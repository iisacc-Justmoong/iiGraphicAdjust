#include <iiGraphicAdjust/Mapping.h>
#include "Internal.h"
#include <charconv>

namespace iiGraphicAdjust {
namespace {
Error invalid(std::string_view message) { return {ErrorCode::InvalidParameter,message}; }
Result<std::size_t> rowCount(LutKind kind,std::size_t size) {
    if (kind==LutKind::OneDimensional && size>=2 && size<=65536) return size;
    if (kind==LutKind::ThreeDimensional && size>=2 && size<=129) return size*size*size;
    return std::unexpected(invalid("LUT kind or size is unsupported"));
}
Result<void> validateLut(const ColorLut& lut) {
    const auto rows=rowCount(lut.kind,lut.size);
    if (!rows) return std::unexpected(rows.error());
    if (lut.values.size()!=*rows) return std::unexpected(invalid("LUT row count does not match its size"));
    for (int c=0;c<3;++c)
        if (!std::isfinite(lut.domainMin[c]) || !std::isfinite(lut.domainMax[c]) ||
            !std::isfinite(lut.domainMax[c]-lut.domainMin[c]) || lut.domainMin[c]>=lut.domainMax[c])
            return std::unexpected(invalid("LUT domain must have finite increasing endpoints"));
    for (const auto& row : lut.values) for (auto v : row)
        if (!std::isfinite(v)) return std::unexpected(invalid("LUT values must be finite"));
    return {};
}
double dither(std::size_t x,std::size_t y) {
    constexpr std::array<int,16> bayer{0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
    return ((bayer[(y%4)*4+x%4]+.5)/16.-.5)/255.;
}
std::array<double,3> sample(const ColorLut& lut,Rgba p,LutInterpolation interpolation) {
    std::array<double,3> positions;
    const std::array<double,3> input{p.r,p.g,p.b};
    std::array<std::size_t,3> indices;
    std::array<double,3> fractions;
    for (int c=0;c<3;++c) {
        positions[c]=detail::clamp((input[c]-lut.domainMin[c])/(lut.domainMax[c]-lut.domainMin[c]))*(lut.size-1);
        indices[c]=std::min(std::size_t(positions[c]),lut.size-2);
        fractions[c]=positions[c]-indices[c];
    }
    std::array<double,3> result{};
    if (lut.kind==LutKind::OneDimensional) {
        for (int c=0;c<3;++c)
            result[c]=std::lerp(double(lut.values[indices[c]][c]),double(lut.values[indices[c]+1][c]),fractions[c]);
        return result;
    }
    const auto at=[&](const std::array<std::size_t,3>& corner) -> const std::array<float,3>& {
        return lut.values[((indices[2]+corner[2])*lut.size+indices[1]+corner[1])*lut.size+indices[0]+corner[0]];
    };
    const auto add=[&](const std::array<std::size_t,3>& corner,double weight) {
        const auto& row=at(corner);
        for (int c=0;c<3;++c) result[c]+=double(row[c])*weight;
    };
    if (interpolation==LutInterpolation::Trilinear) {
        for (std::size_t b=0;b<2;++b) for (std::size_t g=0;g<2;++g) for (std::size_t r=0;r<2;++r)
            add({r,g,b},(r?fractions[0]:1-fractions[0])*(g?fractions[1]:1-fractions[1])*(b?fractions[2]:1-fractions[2]));
    } else {
        std::array<int,3> axes{0,1,2};
        std::sort(axes.begin(),axes.end(),[&](int a,int b) { return fractions[a]>fractions[b]; });
        std::array<std::size_t,3> corner{};
        add(corner,1-fractions[axes[0]]);
        corner[axes[0]]=1; add(corner,fractions[axes[0]]-fractions[axes[1]]);
        corner[axes[1]]=1; add(corner,fractions[axes[1]]-fractions[axes[2]]);
        add({1,1,1},fractions[axes[2]]);
    }
    return result;
}
std::string_view trim(std::string_view s) {
    constexpr std::string_view space=" \t\r\n";
    const auto start=s.find_first_not_of(space);
    if (start==s.npos) return {};
    return s.substr(start,s.find_last_not_of(space)-start+1);
}
std::string_view token(std::string_view& line) {
    line=trim(line);
    const auto end=line.find_first_of(" \t\r\n");
    const auto value=line.substr(0,end);
    line=end==line.npos ? std::string_view{} : line.substr(end);
    return value;
}
template<class T> bool number(std::string_view s,T& value) {
    if (s.empty()) return false;
    if (s.front()=='+') {
        s.remove_prefix(1);
        if (s.empty() || s.front()=='+' || s.front()=='-') return false;
    }
    if (s.empty()) return false;
    const auto parsed=std::from_chars(s.data(),s.data()+s.size(),value);
    return parsed.ec==std::errc{} && parsed.ptr==s.data()+s.size();
}
bool triple(std::string_view line,std::array<double,3>& values) {
    for (auto& v : values)
        if (!number(token(line),v) || !std::isfinite(v)) return false;
    return trim(line).empty();
}
} // namespace
Result<Image> gradientMap(ImageView image,const GradientMapOptions& options) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(options.amount,0,1) || options.stops.size()<2 || options.stops.size()>256)
        return std::unexpected(invalid("Gradient requires 2..256 stops and amount in [0,1]"));
    double previous=-1;
    for (const auto& stop : options.stops) {
        if (!detail::inRange(stop.position,0,1) || stop.position<=previous)
            return std::unexpected(invalid("Gradient positions must be strictly increasing within [0,1]"));
        for (auto v : stop.color) if (!detail::inRange(v,0,1))
            return std::unexpected(invalid("Gradient colors must be finite normalized RGB"));
        previous=stop.position;
    }
    return detail::allocated([&] {
        auto result=detail::copy(image);
        if (options.amount==0) return result;
        for (std::size_t i=0;i<result.pixels.size();++i) {
            auto& p=result.pixels[i];
            if (p.a==0) continue;
            double tone=.2126*p.r+.7152*p.g+.0722*p.b;
            if (options.reverse) tone=1-tone;
            const auto next=std::upper_bound(options.stops.begin(),options.stops.end(),tone,
                [](double value,const GradientStop& stop) { return value<stop.position; });
            const auto& right=next==options.stops.end() ? options.stops.back() : *next;
            const auto& left=next==options.stops.begin() ? options.stops.front() : *(next-1);
            const double fraction=&left==&right ? 0 : detail::clamp((tone-left.position)/(right.position-left.position));
            const double noise=options.dither ? dither(i%image.width,i/image.width) : 0;
            const std::array<double,3> input{p.r,p.g,p.b};
            std::array<float,3> output;
            for (int c=0;c<3;++c)
                output[c]=float(detail::clamp(std::lerp(input[c],detail::clamp(std::lerp(left.color[c],right.color[c],fraction)+noise),options.amount)));
            p.r=output[0]; p.g=output[1]; p.b=output[2];
        }
        return result;
    });
}
Result<ColorLut> identityLut(std::size_t size) {
    const auto rows=rowCount(LutKind::ThreeDimensional,size);
    if (!rows) return std::unexpected(rows.error());
    return detail::allocated([&] {
        ColorLut result; result.size=size; result.values.reserve(*rows);
        for (std::size_t b=0;b<size;++b) for (std::size_t g=0;g<size;++g) for (std::size_t r=0;r<size;++r)
            result.values.push_back({float(r)/float(size-1),float(g)/float(size-1),float(b)/float(size-1)});
        return result;
    });
}
Result<ColorLut> parseCubeLut(std::string_view text) {
    // Parse into local storage; all errors discard the partial LUT.
    if (text.size()>128*1024*1024) return std::unexpected(invalid("Cube text exceeds 128 MiB limit"));
    auto parsed=detail::allocated([&]() -> Result<ColorLut> {
        ColorLut lut;
        bool hasSize=false,hasMin=false,hasMax=false,hasTitle=false,dataStarted=false;
        std::size_t expected=0;
        while (!text.empty()) {
            const auto end=text.find('\n');
            auto line=trim(text.substr(0,end));
            text=end==text.npos ? std::string_view{} : text.substr(end+1);
            if (line.empty() || line.front()=='#') continue;
            // TITLE may contain '#'; other directives/data accept trailing comments.
            auto remainder=line;
            const auto key=token(remainder);
            if (key=="TITLE") {
                remainder=trim(remainder);
                if (dataStarted || hasTitle || remainder.size()<2 || remainder.front()!='"')
                    return std::unexpected(invalid("Malformed or misplaced Cube TITLE"));
                const auto quote=remainder.find('"',1);
                if (quote==remainder.npos) return std::unexpected(invalid("Cube TITLE must be quoted"));
                const auto trailing=trim(remainder.substr(quote+1));
                if (!trailing.empty() && trailing.front()!='#')
                    return std::unexpected(invalid("Unexpected tokens after Cube TITLE"));
                hasTitle=true; continue;
            }
            line=trim(line.substr(0,line.find('#')));
            remainder=line; const auto directive=token(remainder);
            if (directive=="LUT_1D_SIZE" || directive=="LUT_3D_SIZE") {
                if (dataStarted || hasSize || !number(token(remainder),lut.size) || !trim(remainder).empty())
                    return std::unexpected(invalid("Cube must declare exactly one valid LUT size"));
                lut.kind=directive=="LUT_1D_SIZE" ? LutKind::OneDimensional : LutKind::ThreeDimensional;
                const auto rows=rowCount(lut.kind,lut.size);
                if (!rows) return std::unexpected(rows.error());
                expected=*rows; lut.values.reserve(expected); hasSize=true;
            } else if (directive=="DOMAIN_MIN" || directive=="DOMAIN_MAX") {
                auto& seen=directive=="DOMAIN_MIN" ? hasMin : hasMax;
                auto& domain=directive=="DOMAIN_MIN" ? lut.domainMin : lut.domainMax;
                if (seen || dataStarted || !triple(remainder,domain))
                    return std::unexpected(invalid("Malformed or duplicate Cube domain directive"));
                seen=true;
            } else {
                std::array<double,3> row;
                if (!hasSize || !triple(line,row) || lut.values.size()>=expected)
                    return std::unexpected(invalid("Unsupported Cube directive or incorrect RGB row count"));
                std::array<float,3> values;
                for (int c=0;c<3;++c) {
                    if (std::abs(row[c])>std::numeric_limits<float>::max())
                        return std::unexpected(invalid("Cube RGB value exceeds floating-point storage"));
                    values[c]=float(row[c]);
                }
                lut.values.push_back(values); dataStarted=true;
            }
        }
        const auto valid=validateLut(lut);
        if (!valid) return std::unexpected(valid.error());
        return lut;
    });
    if (!parsed) return std::unexpected(parsed.error());
    return std::move(*parsed);
}
Result<Image> colorLookup(ImageView image,const ColorLut& lut,const ColorLookupOptions& options) {
    auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    valid=validateLut(lut);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(options.amount,0,1) ||
        (options.interpolation!=LutInterpolation::Trilinear && options.interpolation!=LutInterpolation::Tetrahedral))
        return std::unexpected(invalid("Invalid LUT interpolation or lookup amount"));
    return detail::allocated([&] {
        auto result=detail::copy(image);
        if (options.amount==0) return result;
        for (std::size_t i=0;i<result.pixels.size();++i) {
            auto& p=result.pixels[i];
            if (p.a==0) continue;
            const auto mapped=sample(lut,p,options.interpolation);
            const auto noise=options.dither ? dither(i%image.width,i/image.width) : 0;
            p.r=float(detail::clamp(std::lerp(double(p.r),detail::clamp(mapped[0]+noise),options.amount)));
            p.g=float(detail::clamp(std::lerp(double(p.g),detail::clamp(mapped[1]+noise),options.amount)));
            p.b=float(detail::clamp(std::lerp(double(p.b),detail::clamp(mapped[2]+noise),options.amount)));
        }
        return result;
    });
}
} // namespace iiGraphicAdjust
