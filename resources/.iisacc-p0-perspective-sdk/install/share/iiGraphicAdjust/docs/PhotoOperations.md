# Photographic orientation, cropping, and histograms

`PhotoOperations.h` adds exact pixel operations and image statistics. The module
is C++23 and has no Qt or codec dependency. All operations accept the existing
`ImageView` contract: nonempty row-major normalized sRGB pixels with straight
alpha. Inputs are validated in full, including hidden RGB at zero alpha.

## EXIF orientation

`applyOrientation(image, orientation)` transforms stored image pixels into the
orientation specified by EXIF tag 274. The enum uses the tag's numeric values.
Every result owns storage, including orientation one. Pixel channels and alpha
are copied exactly; there is no interpolation or quantization.

For a stored image with rows `A B C / D E F`, the displayed result is:

| Tag | Enum | Output rows | Size |
| --- | --- | --- | --- |
| 1 | `TopLeft` | `A B C / D E F` | 3 x 2 |
| 2 | `TopRight` | `C B A / F E D` | 3 x 2 |
| 3 | `BottomRight` | `F E D / C B A` | 3 x 2 |
| 4 | `BottomLeft` | `D E F / A B C` | 3 x 2 |
| 5 | `LeftTop` | `A D / B E / C F` | 2 x 3 |
| 6 | `RightTop` | `D A / E B / F C` | 2 x 3 |
| 7 | `RightBottom` | `F C / E B / D A` | 2 x 3 |
| 8 | `LeftBottom` | `C F / B E / A D` | 2 x 3 |

`Image` stores no EXIF metadata. A caller saving already transformed pixels must
reset its separately managed orientation tag to one. Applying the same tag again
would transform the image twice. Pixel orientation does not automatically rewrite
metadata coordinates, thumbnails, or lens calibration records.

## Integer cropping

`crop(image, PixelRect{x, y, width, height})` copies the exact source rectangle
`[x,x+width) x [y,y+height)` into detached output storage. Coordinates are in the
input pixel grid. To crop displayed coordinates, apply orientation first.

Empty or partly outside rectangles return `InvalidParameter`. Bounds are checked
with subtraction after validating the origin, so a maximum-size integer cannot
wrap into a valid rectangle. There is no padding, clamping, resize, or exposure
change. Alpha and RGB under fully transparent pixels are preserved.

## Histograms and statistics

`imageHistogram(image, options)` returns four channels of weighted counts and
minimum, maximum, and mean values. Red, green, and blue use **encoded sRGB**.
Luminance decodes each sRGB channel to linear light and computes
`Y = 0.2126 R + 0.7152 G + 0.0722 B` using Rec709/sRGB primaries. The histogram is
not an ICC profile transform; callers must convert other color spaces to the
image contract before calling it.

`bins` defaults to 256 and may be in `[2,65536]`. Each bin covers
`[i/bins,(i+1)/bins)`; the final bin also contains exactly one. Bin values are
`double` weights rather than necessarily integer pixel counts. No density
normalization is applied: summing any channel's bins gives `totalWeight`, apart
from floating-point summation rounding. Endpoint occupancy is not proof that the
original scene or sensor was clipped.

| Alpha policy | Pixel weight | Zero-alpha RGB |
| --- | --- | --- |
| `CoverageWeighted` (default) | Alpha coverage | Excluded |
| `VisiblePixels` | One for each positive-alpha pixel | Excluded |
| `AllPixels` | One for every pixel | Included |

All four channels and their means use the same policy. Extrema are measured only
over positive-weight samples. `pixelCount` is the validated input pixel count;
`contributingPixels` counts samples with positive weight. A fully transparent
image with either exclusion policy succeeds with zero weight, all-zero bins,
and zero extrema and means. Those zero statistics indicate no observations;
use `contributingPixels` to distinguish this case from a visible black image.

The count storage is at most four times 65536 `double` values (approximately
2 MiB), independent of input resolution. Analysis scans the source without
changing it. Orientation and cropping allocate one output image. Storage
allocation and container-limit failures return `AllocationFailure`, using the
same static-message error contract as the rest of the library.

```cpp
#include <iiGraphicAdjust/PhotoOperations.h>

auto displayed = iiGraphicAdjust::applyOrientation(
    source.view(), iiGraphicAdjust::ExifOrientation::RightTop);
if (displayed) {
    auto region = iiGraphicAdjust::crop(displayed->view(), {0, 0, 128, 128});
    if (region) {
        auto statistics = iiGraphicAdjust::imageHistogram(region->view());
        // statistics->red.mean uses encoded sRGB, luminance.mean uses linear light.
    }
}
```

Tests in `tests/photo_operations.cpp` specify all eight rectangular orientation
outputs independently, exact crop boundaries and integer overflow cases,
encoded versus linear histogram values, three alpha policies, detached storage,
fully transparent input, invalid enums, and malformed/nonfinite images.
