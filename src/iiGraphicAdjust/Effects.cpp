#include <iiGraphicAdjust/Effects.h>
#include "Internal.h"
#include <numbers>

namespace iiGraphicAdjust {
namespace {
struct Sample { double r=0,g=0,b=0,a=0; };
std::size_t clampedIndex(std::size_t position,int offset,std::size_t extent) noexcept {
    if (offset<0) return position < std::size_t(-offset) ? 0 : position-std::size_t(-offset);
    return std::min(position+std::size_t(offset),extent-1);
}
Image blurred(ImageView image,double sigma) {
    const int radius=static_cast<int>(std::ceil(3*sigma));
    std::vector<double> kernel(std::size_t(2*radius+1));
    double sum=0;
    for (int k=-radius;k<=radius;++k) {
        const double distance=k/sigma;
        const double value=std::exp(-.5*distance*distance);
        kernel[std::size_t(k+radius)]=value; sum+=value;
    }
    for (auto& value : kernel) value/=sum;
    std::vector<Sample> input(image.pixels.size()),horizontal(input.size());
    for (std::size_t i=0;i<input.size();++i) {
        const auto p=image.pixels[i];
        input[i]={detail::linear(p.r)*p.a,detail::linear(p.g)*p.a,detail::linear(p.b)*p.a,p.a};
    }
    const auto accumulate=[](Sample& target,const Sample& sample,double weight) {
        target.r+=sample.r*weight; target.g+=sample.g*weight;
        target.b+=sample.b*weight; target.a+=sample.a*weight;
    };
    for (std::size_t y=0;y<image.height;++y)
        for (std::size_t x=0;x<image.width;++x)
            for (int k=-radius;k<=radius;++k)
                accumulate(horizontal[y*image.width+x],input[y*image.width+clampedIndex(x,k,image.width)],
                           kernel[std::size_t(k+radius)]);
    auto result=detail::output(image);
    for (std::size_t y=0;y<image.height;++y) {
        for (std::size_t x=0;x<image.width;++x) {
            Sample sample;
            for (int k=-radius;k<=radius;++k)
                accumulate(sample,horizontal[clampedIndex(y,k,image.height)*image.width+x],
                           kernel[std::size_t(k+radius)]);
            result.pixels[y*image.width+x]=sample.a>0 ?
                Rgba{detail::encoded(sample.r/sample.a),detail::encoded(sample.g/sample.a),
                     detail::encoded(sample.b/sample.a),float(detail::clamp(sample.a))} : Rgba{0,0,0,0};
        }
    }
    return result;
}
// Specified integer arithmetic avoids implementation-defined distribution state.
std::uint64_t next(std::uint64_t& state) noexcept {
    state+=0x9e3779b97f4a7c15ULL;
    auto value=state;
    value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
    value=(value^(value>>27))*0x94d049bb133111ebULL;
    return value^(value>>31);
}
double uniform(std::uint64_t& state) noexcept {
    return (double(next(state)>>32)+.5)/4294967296.; // Strictly between zero and one.
}
double random(std::uint64_t& state,NoiseDistribution distribution) noexcept {
    if (distribution==NoiseDistribution::Uniform) return 2*uniform(state)-1;
    const double u=uniform(state),v=uniform(state);
    return std::sqrt(-2*std::log(u))*std::cos(2*std::numbers::pi*v);
}
} // namespace
Result<Image> gaussianBlur(ImageView image,double sigma) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(sigma,0,32))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Gaussian sigma must be finite and within [0,32]"});
    return detail::allocated([&] { return sigma==0 ? detail::copy(image) : blurred(image,sigma); });
}
Result<Image> sharpen(ImageView image,const SharpenOptions& options) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(options.sigma,0,32) || !detail::inRange(options.amount,0,8) ||
        !detail::inRange(options.threshold,0,1))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Sharpen sigma, amount or threshold is out of range"});
    return detail::allocated([&] {
        auto result=detail::copy(image);
        if (options.sigma==0 || options.amount==0) return result;
        const auto soft=blurred(image,options.sigma);
        for (std::size_t i=0;i<result.pixels.size();++i) {
            auto& p=result.pixels[i];
            if (p.a==0) continue;
            const auto b=soft.pixels[i];
            const double r=detail::linear(p.r),g=detail::linear(p.g),blue=detail::linear(p.b);
            const double dr=r-detail::linear(b.r),dg=g-detail::linear(b.g),db=blue-detail::linear(b.b);
            if (std::max({std::abs(dr),std::abs(dg),std::abs(db)})<=options.threshold) continue;
            p.r=detail::encoded(r+options.amount*dr); p.g=detail::encoded(g+options.amount*dg);
            p.b=detail::encoded(blue+options.amount*db);
        }
        return result;
    });
}
Result<Image> addNoise(ImageView image,const NoiseOptions& options) {
    const auto valid=detail::validate(image);
    if (!valid) return std::unexpected(valid.error());
    if (!detail::inRange(options.amount,0,1) ||
        (options.distribution!=NoiseDistribution::Uniform && options.distribution!=NoiseDistribution::Gaussian))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Invalid noise amount or distribution"});
    return detail::allocated([&] {
        auto result=detail::copy(image);
        if (options.amount==0) return result;
        for (std::size_t i=0;i<result.pixels.size();++i) {
            auto& p=result.pixels[i];
            if (p.a==0) continue;
            auto state=options.seed+std::uint64_t(i)*0x9e3779b97f4a7c15ULL;
            state=next(state); // Hash each pixel before drawing its independent channel sequence.
            const double nr=options.amount*random(state,options.distribution);
            const double ng=options.monochromatic ? nr : options.amount*random(state,options.distribution);
            const double nb=options.monochromatic ? nr : options.amount*random(state,options.distribution);
            p.r=float(detail::clamp(p.r+nr)); p.g=float(detail::clamp(p.g+ng)); p.b=float(detail::clamp(p.b+nb));
        }
        return result;
    });
}
} // namespace iiGraphicAdjust
