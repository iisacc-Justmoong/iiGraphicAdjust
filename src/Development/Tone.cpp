#include <iiGraphicAdjust/Tone.h>
#include "Internal.h"
#include <array>

namespace iiGraphicAdjust {
namespace {
Result<Image> parameterError() {
    return std::unexpected(Error{ErrorCode::InvalidParameter,
                                 "Tonal adjustment parameters violate the documented contract"});
}
template<class F> Result<Image> mapVisible(ImageView image, F&& operation) {
    return detail::allocated([&] {
        auto output = detail::copy(image);
        for (auto& p : output.pixels) if (p.a != 0) operation(p);
        return output;
    });
}
bool validLevel(const LevelChannel& c) noexcept {
    return detail::inRange(c.inputBlack,0,1) && detail::inRange(c.inputWhite,0,1) &&
        c.inputBlack < c.inputWhite && detail::inRange(c.gamma,.01,100) &&
        detail::inRange(c.outputBlack,0,1) && detail::inRange(c.outputWhite,0,1);
}
bool identityLevel(const LevelChannel& c) noexcept {
    return c.inputBlack == 0 && c.inputWhite == 1 && c.gamma == 1 &&
           c.outputBlack == 0 && c.outputWhite == 1;
}
double mapLevel(double v, const LevelChannel& c) noexcept {
    if (identityLevel(c)) return v;
    const double normalized = detail::clamp((v-c.inputBlack)/(c.inputWhite-c.inputBlack));
    return c.outputBlack + (c.outputWhite-c.outputBlack)*std::pow(normalized,1/c.gamma);
}
bool validCurve(const CurveChannel& c) noexcept {
    if (c.points.size() < 2 || c.points.size() > 65536) return false;
    double previous = -1;
    for (const auto& point : c.points) {
        if (!detail::inRange(point.input,0,1) || !detail::inRange(point.output,0,1) ||
            point.input <= previous) return false;
        previous = point.input;
    }
    return true;
}
bool identityCurve(const CurveChannel& c) noexcept {
    if (c.points.front().input != 0 || c.points.back().input != 1) return false;
    return std::all_of(c.points.begin(),c.points.end(),[](const auto& p) { return p.input == p.output; });
}
double mapCurve(double value, const CurveChannel& c) noexcept {
    if (value <= c.points.front().input) return c.points.front().output;
    if (value >= c.points.back().input) return c.points.back().output;
    const auto upper = std::upper_bound(c.points.begin(),c.points.end(),value,
        [](double v,const CurvePoint& p) { return v < p.input; });
    const auto& lower = *(upper-1);
    return std::lerp(lower.output,upper->output,(value-lower.input)/(upper->input-lower.input));
}
double luminance(const Rgba& p) noexcept { return .2126*p.r+.7152*p.g+.0722*p.b; }
double smoothWeight(double value) noexcept {
    const double x = detail::clamp(value);
    return x*x*(3-2*x);
}
bool validAuto(const AutoToneOptions& s) noexcept {
    return detail::inRange(s.blackClip,0,1) && detail::inRange(s.whiteClip,0,1) &&
           s.blackClip+s.whiteClip < 1;
}
struct Bounds final { double black = 0, white = 1; bool constant = true; };
Bounds bounds(std::vector<double>& samples, const AutoToneOptions& options) {
    if (samples.empty()) return {};
    std::sort(samples.begin(),samples.end());
    const double last = static_cast<double>(samples.size()-1);
    const auto low = static_cast<std::size_t>(std::floor(options.blackClip*last));
    const auto high = static_cast<std::size_t>(std::ceil((1-options.whiteClip)*last));
    const double black = samples[low], white = samples[high];
    return {black,white,!(black < white)};
}
double stretch(double value,const Bounds& b) noexcept {
    return b.constant ? value : detail::clamp((value-b.black)/(b.white-b.black));
}
enum class AutoKind { Channels, Common, Color };
Result<Image> automatic(ImageView image,const AutoToneOptions& options,AutoKind kind) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!validAuto(options)) return parameterError();
    return detail::allocated([&] {
        std::array<std::vector<double>,3> samples;
        for (const auto& p : image.pixels) if (p.a != 0) {
            if (kind == AutoKind::Common) {
                samples[0].push_back(p.r); samples[0].push_back(p.g); samples[0].push_back(p.b);
            } else {
                samples[0].push_back(p.r); samples[1].push_back(p.g); samples[2].push_back(p.b);
            }
        }
        std::array<Bounds,3> endpoints;
        endpoints[0] = bounds(samples[0],options);
        if (kind == AutoKind::Common) endpoints[1] = endpoints[2] = endpoints[0];
        else { endpoints[1] = bounds(samples[1],options); endpoints[2] = bounds(samples[2],options); }
        std::array<double,3> gamma{1,1,1};
        if (kind == AutoKind::Color) for (std::size_t channel=0;channel<3;++channel) {
            const auto& v = samples[channel];
            if (v.empty() || endpoints[channel].constant) continue;
            const double middle = v.size()%2 ? v[v.size()/2] : (v[v.size()/2-1]+v[v.size()/2])*.5;
            const double median = stretch(middle,endpoints[channel]);
            // Quantile clipping can put the median on an endpoint. Keep that
            // channel's stretch instead of computing an undefined logarithm.
            if (median > 0 && median < 1)
                gamma[channel] = std::clamp(std::log(median)/std::log(.5),.01,100.);
        }
        auto output = detail::copy(image);
        for (auto& p : output.pixels) if (p.a != 0) {
            p.r = static_cast<float>(std::pow(stretch(p.r,endpoints[0]),1/gamma[0]));
            p.g = static_cast<float>(std::pow(stretch(p.g,endpoints[1]),1/gamma[1]));
            p.b = static_cast<float>(std::pow(stretch(p.b,endpoints[2]),1/gamma[2]));
        }
        return output;
    });
}
} // namespace

Result<Image> brightnessContrast(ImageView image,const BrightnessContrastOptions& s) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(s.brightness,-1,1) || !detail::inRange(s.contrast,-1,1)) return parameterError();
    return mapVisible(image,[&](Rgba& p) {
        if (s.brightness == 0 && s.contrast == 0) return;
        auto transform = [&](double v) { return static_cast<float>(detail::clamp((v+s.brightness-.5)*(1+s.contrast)+.5)); };
        p.r=transform(p.r); p.g=transform(p.g); p.b=transform(p.b);
    });
}
Result<Image> levels(ImageView image,const LevelsOptions& s) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!validLevel(s.master) || !validLevel(s.red) || !validLevel(s.green) || !validLevel(s.blue)) return parameterError();
    return mapVisible(image,[&](Rgba& p) {
        p.r=static_cast<float>(mapLevel(mapLevel(p.r,s.master),s.red));
        p.g=static_cast<float>(mapLevel(mapLevel(p.g,s.master),s.green));
        p.b=static_cast<float>(mapLevel(mapLevel(p.b,s.master),s.blue));
    });
}
Result<Image> curves(ImageView image,const CurvesOptions& s) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!validCurve(s.master) || !validCurve(s.red) || !validCurve(s.green) || !validCurve(s.blue)) return parameterError();
    const bool masterIdentity=identityCurve(s.master), redIdentity=identityCurve(s.red);
    const bool greenIdentity=identityCurve(s.green), blueIdentity=identityCurve(s.blue);
    return mapVisible(image,[&](Rgba& p) {
        auto transform = [&](double value,const CurveChannel& channel,bool identity) {
            const double master=masterIdentity ? value : mapCurve(value,s.master);
            return static_cast<float>(identity ? master : mapCurve(master,channel));
        };
        p.r=transform(p.r,s.red,redIdentity);
        p.g=transform(p.g,s.green,greenIdentity);
        p.b=transform(p.b,s.blue,blueIdentity);
    });
}
Result<Image> exposure(ImageView image,const ExposureOptions& s) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(s.stops,-16,16) || !detail::inRange(s.offset,-1,1) || !detail::inRange(s.gamma,.01,100)) return parameterError();
    return mapVisible(image,[&](Rgba& p) {
        if (s.stops == 0 && s.offset == 0 && s.gamma == 1) return;
        auto transform = [&](double v) { return detail::encoded(std::pow(detail::clamp(detail::linear(v)*std::exp2(s.stops)+s.offset),1/s.gamma)); };
        p.r=transform(p.r); p.g=transform(p.g); p.b=transform(p.b);
    });
}
Result<Image> shadowsHighlights(ImageView image,const ShadowsHighlightsOptions& s) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(s.shadows,0,1) || !detail::inRange(s.highlights,0,1) ||
        !detail::inRange(s.shadowWidth,0,1) || !detail::inRange(s.highlightWidth,0,1) ||
        s.shadowWidth == 0 || s.highlightWidth == 0) return parameterError();
    return mapVisible(image,[&](Rgba& p) {
        if (s.shadows == 0 && s.highlights == 0) return;
        const double y=luminance(p);
        const double lift=s.shadows*smoothWeight(1-y/s.shadowWidth);
        const double dark=s.highlights*smoothWeight(1-(1-y)/s.highlightWidth);
        auto transform = [&](double v) { return static_cast<float>(detail::clamp((v+lift*(1-v))*(1-dark))); };
        p.r=transform(p.r); p.g=transform(p.g); p.b=transform(p.b);
    });
}
Result<Image> invert(ImageView image) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    return mapVisible(image,[](Rgba& p) { p.r=1-p.r; p.g=1-p.g; p.b=1-p.b; });
}
Result<Image> posterize(ImageView image,unsigned count) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (count < 2 || count > 65536) return parameterError();
    return mapVisible(image,[&](Rgba& p) {
        const double steps=count-1;
        auto transform = [&](double v) { return static_cast<float>(std::round(v*steps)/steps); };
        p.r=transform(p.r); p.g=transform(p.g); p.b=transform(p.b);
    });
}
Result<Image> threshold(ImageView image,double level) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(level,0,1)) return parameterError();
    return mapVisible(image,[&](Rgba& p) { p.r=p.g=p.b=luminance(p) >= level ? 1.F : 0.F; });
}
Result<Image> equalize(ImageView image) {
    const auto valid = detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        // Double counts avoid overflow from pooling three channels per pixel.
        std::array<double,256> histogram{};
        auto bin=[](float v) { return static_cast<std::size_t>(std::lround(double(v)*255)); };
        for (const auto& p : image.pixels) if (p.a != 0) {
            ++histogram[bin(p.r)]; ++histogram[bin(p.g)]; ++histogram[bin(p.b)];
        }
        const auto first=std::find_if(histogram.begin(),histogram.end(),[](double count) { return count != 0; });
        auto output=detail::copy(image);
        if (first == histogram.end()) return output;
        double total=0;
        for (double count : histogram) total+=count;
        const double firstCount=*first;
        if (total == firstCount) return output;
        std::array<float,256> table{};
        double cumulative=0;
        for (std::size_t i=0;i<table.size();++i) {
            cumulative+=histogram[i];
            table[i]=static_cast<float>(detail::clamp((cumulative-firstCount)/(total-firstCount)));
        }
        for (auto& p : output.pixels) if (p.a != 0) {
            p.r=table[bin(p.r)]; p.g=table[bin(p.g)]; p.b=table[bin(p.b)];
        }
        return output;
    });
}
Result<Image> autoLevels(ImageView image,const AutoToneOptions& options) {
    return automatic(image,options,AutoKind::Channels);
}
Result<Image> autoContrast(ImageView image,const AutoToneOptions& options) {
    return automatic(image,options,AutoKind::Common);
}
Result<Image> autoColor(ImageView image,const AutoToneOptions& options) {
    return automatic(image,options,AutoKind::Color);
}
} // namespace iiGraphicAdjust
