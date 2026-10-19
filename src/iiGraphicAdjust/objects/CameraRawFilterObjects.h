#pragma once
#include <iiGraphicAdjust/CameraRawFilter.h>
#include <iiGraphicAdjust/Operation.h>
namespace iiGraphicAdjust::objects {
struct CameraRawFilter final {
    inline static constexpr OperationInfo info{"cameraRaw.cameraRawFilter","CameraRawFilter","cameraRaw"};
    CameraRawOptions options;
    [[nodiscard]] Result<Image> apply(ImageView image) const { return cameraRawFilter(image,options); }
};
}
