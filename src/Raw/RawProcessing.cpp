#include <iiGraphicAdjust/RawProcessing.h>
#include "Internal.h"

namespace iiGraphicAdjust {
namespace {
constexpr std::array<std::array<unsigned,4>,4> colors{{
    {0,1,1,2}, {2,1,1,0}, {1,0,2,1}, {1,2,0,1}
}};

std::size_t tileAt(BayerImageView image,std::size_t x,std::size_t y) noexcept {
    return ((x&1)^image.phaseX)+2*((y&1)^image.phaseY);
}
unsigned colorAt(BayerImageView image,std::size_t x,std::size_t y) noexcept {
    return colors[static_cast<std::size_t>(image.pattern)][tileAt(image,x,y)];
}

Result<std::size_t> validateRaw(BayerImageView image,const RawDevelopmentOptions& options) {
    const auto count=detail::pixelCount(image.width,image.height);
    if(!count) return std::unexpected(count.error());
    if(image.width<2||image.height<2)
        return std::unexpected(Error{ErrorCode::InvalidDimensions,"Bayer reconstruction requires at least 2x2 samples"});
    if(image.pattern!=BayerPattern::RGGB&&image.pattern!=BayerPattern::BGGR&&
       image.pattern!=BayerPattern::GRBG&&image.pattern!=BayerPattern::GBRG)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Unsupported Bayer pattern"});
    if(image.phaseX>1||image.phaseY>1)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"Bayer phase offsets must be zero or one"});
    if(image.strideSamples<image.width)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"Bayer sample stride must be at least the image width"});
    if(image.height-1>(std::numeric_limits<std::size_t>::max()-image.width)/image.strideSamples)
        return std::unexpected(Error{ErrorCode::InvalidDimensions,"Bayer row stride overflows buffer addressing"});
    const auto required=(image.height-1)*image.strideSamples+image.width;
    if(image.samples.size()<required)
        return std::unexpected(Error{ErrorCode::InvalidBuffer,"Bayer buffer does not contain every addressed sample"});
    const auto& calibration=options.calibration;
    if(!detail::inRange(calibration.whiteLevel,0,65535)||calibration.whiteLevel==0)
        return std::unexpected(Error{ErrorCode::InvalidParameter,"RAW white level must be finite in (0,65535]"});
    for(const auto black:calibration.blackLevels)
        if(!std::isfinite(black)||black<0||black>=calibration.whiteLevel)
            return std::unexpected(Error{ErrorCode::InvalidParameter,"Every RAW black level must be finite nonnegative and below white"});
    for(const auto gain:calibration.whiteBalanceGains)
        if(!std::isfinite(gain)||gain<=0)
            return std::unexpected(Error{ErrorCode::InvalidParameter,"RAW white balance gains must be finite positive"});
    for(const auto coefficient:calibration.cameraToLinearSrgb)
        if(!std::isfinite(coefficient))
            return std::unexpected(Error{ErrorCode::InvalidParameter,"RAW camera color matrix entries must be finite"});
    if(!detail::inRange(options.exposureStops,-16,16))
        return std::unexpected(Error{ErrorCode::InvalidParameter,"RAW exposure must be finite in [-16,16] stops"});
    return *count;
}

// At measured sites use the observed value. Else the matching-color samples in
// the surrounding 3x3 cell give the standard Bayer bilinear stencils: two axial
// neighbors at a green site, four axial green neighbors at R/B, or four diagonal
// opposite-color neighbors. Edges use the available subset without extrapolation.
double interpolate(BayerImageView image,std::span<const double> normalized,
                   std::size_t x,std::size_t y,unsigned target) noexcept {
    if(colorAt(image,x,y)==target) return normalized[y*image.width+x];
    const auto left=x==0?0:x-1,top=y==0?0:y-1;
    const auto right=std::min(x+1,image.width-1),bottom=std::min(y+1,image.height-1);
    double average=0;
    unsigned count=0;
    for(auto row=top;row<=bottom;++row) for(auto column=left;column<=right;++column)
        if(colorAt(image,column,row)==target) {
            // Incremental average avoids overflowing the sum of large samples.
            const auto value=normalized[row*image.width+column];
            ++count;
            average+=(value-average)/count;
        }
    // A supported CFA with width/height >= 2 has every target color in this cell.
    return average;
}
} // namespace

Result<LinearRgbImage> developRaw(BayerImageView image,const RawDevelopmentOptions& options) {
    const auto count=validateRaw(image,options);
    if(!count) return std::unexpected(count.error());
    try {
        std::vector<double> normalized(*count);
        const auto& calibration=options.calibration;
        for(std::size_t y=0;y<image.height;++y) for(std::size_t x=0;x<image.width;++x) {
            const auto black=calibration.blackLevels[tileAt(image,x,y)];
            const auto sample=std::max(0.,double(image.samples[y*image.strideSamples+x])-black)/
                              (calibration.whiteLevel-black);
            if(!std::isfinite(sample))
                return std::unexpected(Error{ErrorCode::InvalidPixel,"RAW sample normalization exceeds finite processing range"});
            normalized[y*image.width+x]=sample;
        }
        LinearRgbImage output{image.width,image.height,std::vector<HdrRgba>(*count)};
        const auto exposure=std::exp2(options.exposureStops);
        for(std::size_t y=0;y<image.height;++y) for(std::size_t x=0;x<image.width;++x) {
            std::array<double,3> camera{};
            for(unsigned channel=0;channel<3;++channel) {
                camera[channel]=interpolate(image,normalized,x,y,channel)*calibration.whiteBalanceGains[channel];
                if(!std::isfinite(camera[channel]))
                    return std::unexpected(Error{ErrorCode::InvalidPixel,"RAW white balance exceeds finite processing range"});
            }
            std::array<float,3> rgb{};
            for(unsigned channel=0;channel<3;++channel) {
                const auto row=3*channel;
                const double transformed=calibration.cameraToLinearSrgb[row]*camera[0]+
                    calibration.cameraToLinearSrgb[row+1]*camera[1]+
                    calibration.cameraToLinearSrgb[row+2]*camera[2];
                const double exposed=std::max(0.,transformed)*exposure;
                if(!std::isfinite(transformed)||!std::isfinite(exposed)||
                   exposed>std::numeric_limits<float>::max())
                    return std::unexpected(Error{ErrorCode::InvalidPixel,"RAW color transform exceeds finite output range"});
                rgb[channel]=static_cast<float>(exposed);
            }
            output.pixels[y*image.width+x]={rgb[0],rgb[1],rgb[2],1};
        }
        return output;
    } catch(const std::bad_alloc&) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"Could not allocate RAW processing storage"});
    } catch(const std::length_error&) {
        return std::unexpected(Error{ErrorCode::AllocationFailure,"RAW processing storage exceeds container limits"});
    }
}

Result<Image> renderRaw(BayerImageView image,const RawDevelopmentOptions& options,
                        const HdrToningOptions& toning) {
    const auto linear=developRaw(image,options);
    if(!linear) return std::unexpected(linear.error());
    return hdrToning(linear->view(),toning);
}
} // namespace iiGraphicAdjust
