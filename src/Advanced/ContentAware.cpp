#include <iiGraphicAdjust/ContentAware.h>
#include "Internal.h"
#include <cstdint>
#include <numeric>

namespace iiGraphicAdjust::objects {
namespace {
// Validate all values before allocating or changing any result.
Result<void> maskValid(MaskView mask, std::size_t width, std::size_t height) {
    const auto count = detail::pixelCount(mask.width,mask.height);
    if (!count) return std::unexpected(count.error());
    if (mask.width!=width || mask.height!=height)
        return std::unexpected(Error{ErrorCode::SizeMismatch,"Mask dimensions must match image"});
    if (mask.coverage.size()!=*count)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"Mask buffer must match dimensions"});
    for (float value : mask.coverage)
        if (!detail::inRange(value,0,1))
            return std::unexpected(Error{ErrorCode::InvalidParameter,"Mask coverage must be normalized"});
    return {};
}

// Interpolate premultiplied channels so transparent source colors do not bleed.
Rgba interpolate(Rgba a, Rgba b, double t) {
    if (t<=0) return a;
    if (t>=1) return b;
    const double alpha=a.a*(1-t)+b.a*t;
    if (alpha==0) return {0,0,0,0};
    const auto channel=[&](float x,float y) {
        return static_cast<float>((x*a.a*(1-t)+y*b.a*t)/alpha);
    };
    return {channel(a.r,b.r),channel(a.g,b.g),channel(a.b,b.b),static_cast<float>(alpha)};
}

// Find a known original sample whose available neighbourhood best matches the
// already reconstructed context. Stable scan order makes the result repeatable.
std::size_t bestSample(const Image& working, const std::vector<unsigned char>& known,
                       const std::vector<unsigned char>& originalKnown,
                       const std::vector<std::size_t>& candidates, std::size_t target,
                       unsigned radius, std::size_t maxCandidates) {
    const auto w=working.width,h=working.height;
    const auto tx=target%w,ty=target/w;
    std::size_t best=candidates.front();
    double bestError=std::numeric_limits<double>::infinity();
    double bestDistance=std::numeric_limits<double>::infinity();
    const auto samples=std::min(maxCandidates,candidates.size());
    for (std::size_t n=0;n<samples;++n) {
        const auto offset=n*(candidates.size()/samples)+
            std::size_t((std::uint64_t(n)*(candidates.size()%samples))/samples);
        const auto candidate=candidates[offset];
        const auto cx=candidate%w,cy=candidate/w;
        double error=0; std::size_t used=0;
        for (int dy=-int(radius);dy<=int(radius);++dy)
            for (int dx=-int(radius);dx<=int(radius);++dx) {
                if (!dx && !dy) continue;
                const auto ax=std::int64_t(tx)+dx,ay=std::int64_t(ty)+dy;
                const auto bx=std::int64_t(cx)+dx,by=std::int64_t(cy)+dy;
                if (ax<0 || ay<0 || bx<0 || by<0 || ax>=std::int64_t(w) ||
                    bx>=std::int64_t(w) || ay>=std::int64_t(h) || by>=std::int64_t(h)) continue;
                const auto a=std::size_t(ay)*w+std::size_t(ax),b=std::size_t(by)*w+std::size_t(bx);
                if (!known[a] || !originalKnown[b]) continue;
                const auto p=working.pixels[a],q=working.pixels[b];
                const auto dr=double(p.r)-q.r,dg=double(p.g)-q.g,db=double(p.b)-q.b,da=double(p.a)-q.a;
                error+=dr*dr+dg*dg+db*db+da*da; ++used;
            }
        error=used ? error/double(used) : 5.;
        const double x=double(tx)-double(cx),y=double(ty)-double(cy),distance=x*x+y*y;
        if (error<bestError || (error==bestError && distance<bestDistance)) {
            best=candidate; bestError=error; bestDistance=distance;
        }
    }
    return best;
}

// Transposition is local to this call and carries seam preferences with pixels.
void transpose(Image& image, std::vector<float>& weights) {
    Image other{image.height,image.width,std::vector<Rgba>(image.pixels.size())};
    std::vector<float> transposed(weights.size());
    for (std::size_t y=0;y<image.height;++y)
        for (std::size_t x=0;x<image.width;++x) {
            const auto from=y*image.width+x,to=x*image.height+y;
            other.pixels[to]=image.pixels[from]; transposed[to]=weights[from];
        }
    image=std::move(other); weights=std::move(transposed);
}

double difference(Rgba a,Rgba b) {
    return std::abs(double(a.r)*a.a-double(b.r)*b.a)+
           std::abs(double(a.g)*a.a-double(b.g)*b.a)+
           std::abs(double(a.b)*a.a-double(b.b)*b.a)+std::abs(double(a.a)-b.a);
}

// Dynamic programming finds one minimum-energy vertical seam; parent indices
// are signed offsets so the storage remains bounded to the image size.
std::vector<std::size_t> seam(const Image& image,const std::vector<float>& weights) {
    const auto w=image.width,h=image.height;
    std::vector<double> previous(w),current(w);
    std::vector<std::int8_t> parents(image.pixels.size());
    for (std::size_t y=0;y<h;++y) {
        for (std::size_t x=0;x<w;++x) {
            const auto left=x?x-1:x,right=std::min(w-1,x+1);
            const auto up=y?y-1:y,down=std::min(h-1,y+1);
            const auto energy=difference(image.pixels[y*w+left],image.pixels[y*w+right])+
                difference(image.pixels[up*w+x],image.pixels[down*w+x])+10000.*weights[y*w+x];
            std::size_t parent=x;
            if (y) {
                if (x && previous[x-1]<previous[parent]) parent=x-1;
                if (x+1<w && previous[x+1]<previous[parent]) parent=x+1;
            }
            current[x]=energy+(y?previous[parent]:0);
            parents[y*w+x]=static_cast<std::int8_t>(std::int64_t(parent)-std::int64_t(x));
        }
        previous.swap(current);
    }
    auto at=static_cast<std::size_t>(std::min_element(previous.begin(),previous.end())-previous.begin());
    std::vector<std::size_t> path(h);
    for (std::size_t y=h;y-->0;) {
        path[y]=at;
        if (y) at=static_cast<std::size_t>(std::int64_t(at)+parents[y*w+at]);
    }
    return path;
}

// One pass creates new storage; the source and its constraint map stay aligned.
void changeWidth(Image& image,std::vector<float>& weights,bool insert) {
    const auto path=seam(image,weights);
    const auto w=image.width,h=image.height;
    const auto newWidth=insert?w+1:w-1;
    Image next{newWidth,h,std::vector<Rgba>(newWidth*h)};
    std::vector<float> nextWeights(newWidth*h);
    for (std::size_t y=0;y<h;++y) {
        std::size_t out=0;
        for (std::size_t x=0;x<w;++x) {
            if (!insert && x==path[y]) continue;
            next.pixels[y*newWidth+out]=image.pixels[y*w+x];
            nextWeights[y*newWidth+out++]=weights[y*w+x];
            if (insert && x==path[y]) {
                const auto neighbor=x+1<w?x+1:(x?x-1:x);
                next.pixels[y*newWidth+out]=interpolate(image.pixels[y*w+x],image.pixels[y*w+neighbor],.5);
                nextWeights[y*newWidth+out++]=(weights[y*w+x]+weights[y*w+neighbor])*.5F;
            }
        }
    }
    image=std::move(next); weights=std::move(nextWeights);
}
} // namespace

Result<Image> ContentAwareInpaintSolver::execute(ImageView image,MaskView mask) const {
    if (auto valid=detail::validate(image);!valid) return std::unexpected(valid.error());
    if (auto valid=maskValid(mask,image.width,image.height);!valid) return std::unexpected(valid.error());
    if (patchRadius<1 || patchRadius>4 || !maxCandidates || maxCandidates>65536)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid inpaint search settings"});
    const bool hasSelection=std::any_of(mask.coverage.begin(),mask.coverage.end(),[](float value) { return value>0; });
    if (!hasSelection) return detail::allocated([&] { return detail::copy(image); });
    if (std::none_of(mask.coverage.begin(),mask.coverage.end(),[](float value) { return value==0; }))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Inpaint requires an unmasked source pixel"});
    auto guarded=detail::allocated([&]() -> Result<Image> {
        auto work=detail::copy(image);
        std::vector<unsigned char> known(image.pixels.size()),originalKnown(image.pixels.size());
        std::vector<std::size_t> candidates; candidates.reserve(image.pixels.size());
        for (std::size_t i=0;i<known.size();++i)
            if (mask.coverage[i]==0) { known[i]=originalKnown[i]=1; candidates.push_back(i); }
        if (candidates.empty())
            return std::unexpected(Error{ErrorCode::InvalidParameter,"Inpaint requires an unmasked source pixel"});
        std::size_t left=known.size()-candidates.size();
        while (left) {
            bool progress=false;
            for (std::size_t i=0;i<known.size();++i) {
                if (known[i]) continue;
                const auto x=i%image.width,y=i/image.width;
                const bool adjacent=(x && known[i-1]) || (x+1<image.width && known[i+1]) ||
                    (y && known[i-image.width]) || (y+1<image.height && known[i+image.width]);
                if (!adjacent) continue;
                work.pixels[i]=image.pixels[bestSample(work,known,originalKnown,candidates,i,patchRadius,maxCandidates)];
                known[i]=1; --left; progress=true;
            }
            if (!progress) return std::unexpected(Error{ErrorCode::InvalidParameter,"Inpaint cannot reach selected pixels"});
        }
        for (std::size_t i=0;i<known.size();++i)
            if (mask.coverage[i]>0) work.pixels[i]=interpolate(image.pixels[i],work.pixels[i],mask.coverage[i]);
        return work;
    });
    if (!guarded) return std::unexpected(guarded.error());
    return std::move(*guarded);
}

Result<SeamConstraints> SeamConstraintMask::execute(MaskView protect,MaskView remove) const {
    if (auto valid=maskValid(protect,protect.width,protect.height);!valid) return std::unexpected(valid.error());
    if (auto valid=maskValid(remove,protect.width,protect.height);!valid) return std::unexpected(valid.error());
    return detail::allocated([&] {
        SeamConstraints out{protect.width,protect.height,std::vector<float>(protect.coverage.size())};
        for (std::size_t i=0;i<out.weights.size();++i) out.weights[i]=protect.coverage[i]-remove.coverage[i];
        return out;
    });
}

Result<Image> ContentAwareRescaleSolver::execute(ImageView image,std::size_t width,std::size_t height,
                                                SeamConstraintView constraints) const {
    if (auto valid=detail::validate(image);!valid) return std::unexpected(valid.error());
    const auto count=detail::pixelCount(width,height);
    if (!count) return std::unexpected(count.error());
    const auto intermediate=detail::pixelCount(width,image.height);
    if (!intermediate) return std::unexpected(intermediate.error());
    if (!maxPixels || *count>maxPixels || *intermediate>maxPixels || image.pixels.size()>maxPixels || !maxSeams ||
        (width>image.width?width-image.width:image.width-width)>maxSeams ||
        (height>image.height?height-image.height:image.height-height)>maxSeams)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Seam resize exceeds configured work limits"});
    const bool supplied=constraints.width || constraints.height || !constraints.weights.empty();
    if (supplied) {
        if (constraints.width!=image.width || constraints.height!=image.height)
            return std::unexpected(Error{ErrorCode::SizeMismatch,"Seam constraints must match image"});
        if (constraints.weights.size()!=image.pixels.size())
            return std::unexpected(Error{ErrorCode::InvalidBuffer,"Seam constraint buffer size is invalid"});
        for (float value:constraints.weights)
            if (!detail::inRange(value,-1,1))
                return std::unexpected(Error{ErrorCode::InvalidParameter,"Seam weights must be in [-1,1]"});
    }
    return detail::allocated([&] {
        auto out=detail::copy(image);
        std::vector<float> weights(image.pixels.size());
        if (supplied) std::copy(constraints.weights.begin(),constraints.weights.end(),weights.begin());
        while (out.width!=width) changeWidth(out,weights,out.width<width);
        if (out.height!=height) {
            transpose(out,weights);
            while (out.width!=height) changeWidth(out,weights,out.width<height);
            transpose(out,weights);
        }
        return out;
    });
}
} // namespace iiGraphicAdjust::objects
