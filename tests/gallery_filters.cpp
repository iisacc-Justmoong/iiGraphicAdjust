#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iiGraphicAdjust/GalleryFilters.h>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;
namespace {
std::size_t checks = 0;
void require(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
Image pattern(std::size_t w = 56, std::size_t h = 40) {
    Image image{w, h, {}};
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x) {
            const auto v = float((x * 7 + y * 3) % 23) / 22.f;
            Rgba p = (x / 9 + y / 7) % 2 == 0 ? Rgba{v, .3f, 1 - v, 1} : Rgba{.9f, v * .5f, .2f, 1};
            p.a = (x + y * w) % 17 == 0 ? 0 : ((x + y * w) % 5 == 0 ? .35f : 1.f);
            image.pixels.push_back(p);
        }
    return image;
}
void outputContract(const Image &output, const Image &input) {
    require(output.width == input.width && output.height == input.height, "Dimensions changed");
    require(output.pixels.size() == input.pixels.size(), "Pixel count changed");
    for (std::size_t i = 0; i < input.pixels.size(); ++i) {
        const auto p = output.pixels[i];
        require(std::isfinite(p.r) && p.r >= 0 && p.r <= 1 && std::isfinite(p.g) && p.g >= 0 &&
                    p.g <= 1 && std::isfinite(p.b) && p.b >= 0 && p.b <= 1,
                "Non-finite or unnormalized output");
        require(p.a == input.pixels[i].a, "Alpha changed");
        if (p.a == 0)
            require(p == input.pixels[i], "Hidden RGB changed");
    }
}
template <class F, class O> void defaults(F filter, O options) {
    const auto input = pattern();
    const auto result = filter(input.view(), options);
    require(bool(result), "Default operation failed");
    outputContract(*result, input);
    require(result->pixels != input.pixels, "Default operation changed nothing");
    require(filter(input.view(), options)->pixels == result->pixels,
            "Operation is not deterministic");
    auto alternate = input;
    for (auto &p : alternate.pixels)
        if (p.a == 0) {
            p.r = 1 - p.r;
            p.g = 1 - p.g;
            p.b = 1 - p.b;
        }
    const auto changed = filter(alternate.view(), options);
    require(bool(changed), "Hidden RGB alternate operation failed");
    for (std::size_t i = 0; i < input.pixels.size(); ++i)
        if (input.pixels[i].a > 0)
            require(result->pixels[i] == changed->pixels[i],
                    "Hidden RGB influenced visible output");
    for (const auto &small : {pattern(1, 1), pattern(1, 9), pattern(9, 1)}) {
        const auto out = filter(small.view(), options);
        require(bool(out), "Thin image failed");
        outputContract(*out, small);
    }
    auto hidden = input;
    for (auto &p : hidden.pixels)
        p.a = 0;
    require(filter(hidden.view(), options)->pixels == hidden.pixels, "All-hidden image changed");
    require(!filter(ImageView{}, options), "Empty image accepted");
    auto invalid = input;
    invalid.pixels[0].r = std::numeric_limits<float>::quiet_NaN();
    const auto bad = filter(invalid.view(), options);
    require(!bad && bad.error().code == ErrorCode::InvalidPixel, "Invalid pixel accepted");
}
template <class F, class O> void rejects(F filter, const O &options) {
    const auto input = pattern(7, 5);
    const auto out = filter(input.view(), options);
    require(!out && out.error().code == ErrorCode::InvalidParameter, "Invalid option accepted");
}
template <class F, class O> void extremes(F filter, const O &options) {
    const auto input = pattern();
    const auto out = filter(input.view(), options);
    require(bool(out), "Valid extreme options failed");
    outputContract(*out, input);
}

void test_coloredPencil() {
    const auto fn = coloredPencil;
    defaults(fn, GalleryColoredPencilOptions{});
    {
        GalleryColoredPencilOptions o;
        o.pencilWidth = 0.0;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.pencilWidth = 25.0;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.pencilWidth = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.pencilWidth = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.strokePressure = -1.0;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.strokePressure = 16.0;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.strokePressure = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.strokePressure = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.paperBrightness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.paperBrightness = 51.0;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.paperBrightness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.paperBrightness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.pencilWidth = 1;
        o.strokePressure = 0;
        o.paperBrightness = 0;
        extremes(fn, o);
    }
    {
        GalleryColoredPencilOptions o;
        o.pencilWidth = 24;
        o.strokePressure = 15;
        o.paperBrightness = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryColoredPencilOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryColoredPencilOptions o;
        o.pencilWidth = 1;
        const auto a = fn(input.view(), o);
        o.pencilWidth = 24;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "coloredPencil.pencilWidth slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryColoredPencilOptions o;
        o.strokePressure = 0;
        const auto a = fn(input.view(), o);
        o.strokePressure = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "coloredPencil.strokePressure slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryColoredPencilOptions o;
        o.paperBrightness = 0;
        const auto a = fn(input.view(), o);
        o.paperBrightness = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "coloredPencil.paperBrightness slider had no effect");
    }
}

void test_cutout() {
    const auto fn = cutout;
    defaults(fn, GalleryCutoutOptions{});
    {
        GalleryCutoutOptions o;
        o.numberOfLevels = 1.0;
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.numberOfLevels = 9.0;
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.numberOfLevels = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.numberOfLevels = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeSimplicity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeSimplicity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeSimplicity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeSimplicity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeFidelity = 0.0;
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeFidelity = 4.0;
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeFidelity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.edgeFidelity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.numberOfLevels = 2;
        o.edgeSimplicity = 0;
        o.edgeFidelity = 1;
        extremes(fn, o);
    }
    {
        GalleryCutoutOptions o;
        o.numberOfLevels = 8;
        o.edgeSimplicity = 10;
        o.edgeFidelity = 3;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryCutoutOptions o;
        o.numberOfLevels = 2;
        const auto a = fn(input.view(), o);
        o.numberOfLevels = 8;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "cutout.numberOfLevels slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCutoutOptions o;
        o.edgeSimplicity = 0;
        const auto a = fn(input.view(), o);
        o.edgeSimplicity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "cutout.edgeSimplicity slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCutoutOptions o;
        o.edgeFidelity = 1;
        const auto a = fn(input.view(), o);
        o.edgeFidelity = 3;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "cutout.edgeFidelity slider had no effect");
    }
}

void test_dryBrush() {
    const auto fn = dryBrush;
    defaults(fn, GalleryDryBrushOptions{});
    {
        GalleryDryBrushOptions o;
        o.brushSize = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushSize = 11.0;
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushDetail = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushDetail = 11.0;
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushDetail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushDetail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.texture = 0.0;
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.texture = 4.0;
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.texture = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.texture = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushSize = 0;
        o.brushDetail = 0;
        o.texture = 1;
        extremes(fn, o);
    }
    {
        GalleryDryBrushOptions o;
        o.brushSize = 10;
        o.brushDetail = 10;
        o.texture = 3;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryDryBrushOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryDryBrushOptions o;
        o.brushSize = 0;
        const auto a = fn(input.view(), o);
        o.brushSize = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "dryBrush.brushSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryDryBrushOptions o;
        o.brushDetail = 0;
        const auto a = fn(input.view(), o);
        o.brushDetail = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "dryBrush.brushDetail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryDryBrushOptions o;
        o.texture = 1;
        const auto a = fn(input.view(), o);
        o.texture = 3;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "dryBrush.texture slider had no effect");
    }
}

void test_galleryFilmGrain() {
    const auto fn = galleryFilmGrain;
    defaults(fn, GalleryFilmGrainOptions{});
    {
        GalleryFilmGrainOptions o;
        o.grain = -1.0;
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.grain = 21.0;
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.grain = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.grain = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.highlightArea = -1.0;
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.highlightArea = 21.0;
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.highlightArea = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.highlightArea = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.intensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.intensity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.intensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.intensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.grain = 0;
        o.highlightArea = 0;
        o.intensity = 0;
        extremes(fn, o);
    }
    {
        GalleryFilmGrainOptions o;
        o.grain = 20;
        o.highlightArea = 20;
        o.intensity = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryFilmGrainOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryFilmGrainOptions o;
        o.grain = 0;
        const auto a = fn(input.view(), o);
        o.grain = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "galleryFilmGrain.grain slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryFilmGrainOptions o;
        o.highlightArea = 0;
        const auto a = fn(input.view(), o);
        o.highlightArea = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "galleryFilmGrain.highlightArea slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryFilmGrainOptions o;
        o.highlightArea = 15;
        o.intensity = 0;
        const auto a = fn(input.view(), o);
        o.intensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "galleryFilmGrain.intensity slider had no effect");
    }
}

void test_fresco() {
    const auto fn = fresco;
    defaults(fn, GalleryFrescoOptions{});
    {
        GalleryFrescoOptions o;
        o.brushSize = -1.0;
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushSize = 11.0;
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushDetail = -1.0;
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushDetail = 11.0;
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushDetail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushDetail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.texture = 0.0;
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.texture = 4.0;
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.texture = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.texture = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushSize = 0;
        o.brushDetail = 0;
        o.texture = 1;
        extremes(fn, o);
    }
    {
        GalleryFrescoOptions o;
        o.brushSize = 10;
        o.brushDetail = 10;
        o.texture = 3;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryFrescoOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryFrescoOptions o;
        o.brushSize = 0;
        const auto a = fn(input.view(), o);
        o.brushSize = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "fresco.brushSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryFrescoOptions o;
        o.brushDetail = 0;
        const auto a = fn(input.view(), o);
        o.brushDetail = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "fresco.brushDetail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryFrescoOptions o;
        o.texture = 1;
        const auto a = fn(input.view(), o);
        o.texture = 3;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "fresco.texture slider had no effect");
    }
}

void test_neonGlow() {
    const auto fn = neonGlow;
    defaults(fn, GalleryNeonGlowOptions{});
    {
        GalleryNeonGlowOptions o;
        o.glowSize = -25.0;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowSize = 25.0;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowBrightness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowBrightness = 51.0;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowBrightness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowBrightness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowColor[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowColor[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowColor[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowSize = -24;
        o.glowBrightness = 0;
        extremes(fn, o);
    }
    {
        GalleryNeonGlowOptions o;
        o.glowSize = 24;
        o.glowBrightness = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryNeonGlowOptions o;
        o.glowSize = -24;
        const auto a = fn(input.view(), o);
        o.glowSize = 24;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "neonGlow.glowSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryNeonGlowOptions o;
        o.glowBrightness = 0;
        const auto a = fn(input.view(), o);
        o.glowBrightness = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "neonGlow.glowBrightness slider had no effect");
    }
}

void test_paintDaubs() {
    const auto fn = paintDaubs;
    defaults(fn, GalleryPaintDaubsOptions{});
    {
        GalleryPaintDaubsOptions o;
        o.brushSize = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.brushSize = 51.0;
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.brushSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.brushSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.sharpness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.sharpness = 41.0;
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.sharpness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.sharpness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.brushType = static_cast<GalleryBrushType>(999);
        rejects(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.brushSize = 1;
        o.sharpness = 0;
        extremes(fn, o);
    }
    {
        GalleryPaintDaubsOptions o;
        o.brushSize = 50;
        o.sharpness = 40;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPaintDaubsOptions o;
        o.brushType = GalleryBrushType::LightRough;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryPaintDaubsOptions o;
        o.brushSize = 1;
        const auto a = fn(input.view(), o);
        o.brushSize = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "paintDaubs.brushSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPaintDaubsOptions o;
        o.sharpness = 0;
        const auto a = fn(input.view(), o);
        o.sharpness = 40;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "paintDaubs.sharpness slider had no effect");
    }
}

void test_paletteKnife() {
    const auto fn = paletteKnife;
    defaults(fn, GalleryPaletteKnifeOptions{});
    {
        GalleryPaletteKnifeOptions o;
        o.strokeSize = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeSize = 51.0;
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeDetail = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeDetail = 4.0;
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeDetail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeDetail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.softness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.softness = 11.0;
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.softness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.softness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeSize = 1;
        o.strokeDetail = 1;
        o.softness = 0;
        extremes(fn, o);
    }
    {
        GalleryPaletteKnifeOptions o;
        o.strokeSize = 50;
        o.strokeDetail = 3;
        o.softness = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPaletteKnifeOptions o;
        o.strokeSize = 1;
        const auto a = fn(input.view(), o);
        o.strokeSize = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "paletteKnife.strokeSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPaletteKnifeOptions o;
        o.strokeDetail = 1;
        const auto a = fn(input.view(), o);
        o.strokeDetail = 3;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "paletteKnife.strokeDetail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPaletteKnifeOptions o;
        o.softness = 0;
        const auto a = fn(input.view(), o);
        o.softness = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "paletteKnife.softness slider had no effect");
    }
}

void test_plasticWrap() {
    const auto fn = plasticWrap;
    defaults(fn, GalleryPlasticWrapOptions{});
    {
        GalleryPlasticWrapOptions o;
        o.highlightStrength = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.highlightStrength = 21.0;
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.highlightStrength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.highlightStrength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.detail = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.detail = 16.0;
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.detail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.detail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.highlightStrength = 0;
        o.detail = 1;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GalleryPlasticWrapOptions o;
        o.highlightStrength = 20;
        o.detail = 15;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPlasticWrapOptions o;
        o.smoothness = 1;
        o.detail = 15;
        o.highlightStrength = 0;
        const auto a = fn(input.view(), o);
        o.highlightStrength = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "plasticWrap.highlightStrength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPlasticWrapOptions o;
        o.smoothness = 1;
        o.detail = 15;
        o.detail = 1;
        const auto a = fn(input.view(), o);
        o.detail = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "plasticWrap.detail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPlasticWrapOptions o;
        o.smoothness = 1;
        o.detail = 15;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "plasticWrap.smoothness slider had no effect");
    }
}

void test_posterEdges() {
    const auto fn = posterEdges;
    defaults(fn, GalleryPosterEdgesOptions{});
    {
        GalleryPosterEdgesOptions o;
        o.edgeThickness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeThickness = 11.0;
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeThickness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeThickness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeIntensity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.posterization = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.posterization = 7.0;
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.posterization = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.posterization = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeThickness = 0;
        o.edgeIntensity = 0;
        o.posterization = 0;
        extremes(fn, o);
    }
    {
        GalleryPosterEdgesOptions o;
        o.edgeThickness = 10;
        o.edgeIntensity = 10;
        o.posterization = 6;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPosterEdgesOptions o;
        o.edgeThickness = 0;
        const auto a = fn(input.view(), o);
        o.edgeThickness = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "posterEdges.edgeThickness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPosterEdgesOptions o;
        o.edgeIntensity = 0;
        const auto a = fn(input.view(), o);
        o.edgeIntensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "posterEdges.edgeIntensity slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPosterEdgesOptions o;
        o.posterization = 0;
        const auto a = fn(input.view(), o);
        o.posterization = 6;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "posterEdges.posterization slider had no effect");
    }
}

void test_roughPastels() {
    const auto fn = roughPastels;
    defaults(fn, GalleryRoughPastelsOptions{});
    {
        GalleryRoughPastelsOptions o;
        o.strokeLength = -1.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeLength = 41.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeDetail = 0.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeDetail = 21.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeDetail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeDetail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.texture = static_cast<GalleryTexture>(999);
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.scaling = 49.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.scaling = 201.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.scaling = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.scaling = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.relief = -1.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.relief = 51.0;
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.relief = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.relief = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.light = static_cast<GalleryLightDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeLength = 0;
        o.strokeDetail = 1;
        o.scaling = 50;
        o.relief = 0;
        extremes(fn, o);
    }
    {
        GalleryRoughPastelsOptions o;
        o.strokeLength = 40;
        o.strokeDetail = 20;
        o.scaling = 200;
        o.relief = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryRoughPastelsOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryRoughPastelsOptions o;
        o.strokeLength = 0;
        const auto a = fn(input.view(), o);
        o.strokeLength = 40;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "roughPastels.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryRoughPastelsOptions o;
        o.strokeDetail = 1;
        const auto a = fn(input.view(), o);
        o.strokeDetail = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "roughPastels.strokeDetail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryRoughPastelsOptions o;
        o.scaling = 50;
        const auto a = fn(input.view(), o);
        o.scaling = 200;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "roughPastels.scaling slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryRoughPastelsOptions o;
        o.relief = 0;
        const auto a = fn(input.view(), o);
        o.relief = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "roughPastels.relief slider had no effect");
    }
}

void test_smudgeStick() {
    const auto fn = smudgeStick;
    defaults(fn, GallerySmudgeStickOptions{});
    {
        GallerySmudgeStickOptions o;
        o.strokeLength = -1.0;
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.strokeLength = 11.0;
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.highlightArea = -1.0;
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.highlightArea = 21.0;
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.highlightArea = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.highlightArea = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.intensity = -1.0;
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.intensity = 11.0;
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.intensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.intensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.strokeLength = 0;
        o.highlightArea = 0;
        o.intensity = 0;
        extremes(fn, o);
    }
    {
        GallerySmudgeStickOptions o;
        o.strokeLength = 10;
        o.highlightArea = 20;
        o.intensity = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GallerySmudgeStickOptions o;
        o.strokeLength = 0;
        const auto a = fn(input.view(), o);
        o.strokeLength = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "smudgeStick.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySmudgeStickOptions o;
        o.highlightArea = 0;
        const auto a = fn(input.view(), o);
        o.highlightArea = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "smudgeStick.highlightArea slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySmudgeStickOptions o;
        o.highlightArea = 15;
        o.intensity = 0;
        const auto a = fn(input.view(), o);
        o.intensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "smudgeStick.intensity slider had no effect");
    }
}

void test_sponge() {
    const auto fn = sponge;
    defaults(fn, GallerySpongeOptions{});
    {
        GallerySpongeOptions o;
        o.brushSize = -1.0;
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.brushSize = 11.0;
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.brushSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.brushSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.definition = -1.0;
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.definition = 26.0;
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.definition = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.definition = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.brushSize = 0;
        o.definition = 0;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GallerySpongeOptions o;
        o.brushSize = 10;
        o.definition = 25;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GallerySpongeOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GallerySpongeOptions o;
        o.brushSize = 0;
        const auto a = fn(input.view(), o);
        o.brushSize = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sponge.brushSize slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySpongeOptions o;
        o.definition = 0;
        const auto a = fn(input.view(), o);
        o.definition = 25;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sponge.definition slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySpongeOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sponge.smoothness slider had no effect");
    }
}

void test_underpainting() {
    const auto fn = underpainting;
    defaults(fn, GalleryUnderpaintingOptions{});
    {
        GalleryUnderpaintingOptions o;
        o.brushSize = -1.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.brushSize = 41.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.brushSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.brushSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.textureCoverage = -1.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.textureCoverage = 41.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.textureCoverage = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.textureCoverage = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.texture = static_cast<GalleryTexture>(999);
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.scaling = 49.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.scaling = 201.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.scaling = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.scaling = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.relief = -1.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.relief = 51.0;
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.relief = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.relief = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.light = static_cast<GalleryLightDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.brushSize = 0;
        o.textureCoverage = 0;
        o.scaling = 50;
        o.relief = 0;
        extremes(fn, o);
    }
    {
        GalleryUnderpaintingOptions o;
        o.brushSize = 40;
        o.textureCoverage = 40;
        o.scaling = 200;
        o.relief = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryUnderpaintingOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryUnderpaintingOptions o;
        o.brushSize = 0;
        const auto a = fn(input.view(), o);
        o.brushSize = 40;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "underpainting.brushSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryUnderpaintingOptions o;
        o.textureCoverage = 0;
        const auto a = fn(input.view(), o);
        o.textureCoverage = 40;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "underpainting.textureCoverage slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryUnderpaintingOptions o;
        o.scaling = 50;
        const auto a = fn(input.view(), o);
        o.scaling = 200;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "underpainting.scaling slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryUnderpaintingOptions o;
        o.relief = 0;
        const auto a = fn(input.view(), o);
        o.relief = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "underpainting.relief slider had no effect");
    }
}

void test_watercolor() {
    const auto fn = watercolor;
    defaults(fn, GalleryWatercolorOptions{});
    {
        GalleryWatercolorOptions o;
        o.brushDetail = 0.0;
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.brushDetail = 15.0;
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.brushDetail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.brushDetail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.shadowIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.shadowIntensity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.shadowIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.shadowIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.texture = 0.0;
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.texture = 4.0;
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.texture = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.texture = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.brushDetail = 1;
        o.shadowIntensity = 0;
        o.texture = 1;
        extremes(fn, o);
    }
    {
        GalleryWatercolorOptions o;
        o.brushDetail = 14;
        o.shadowIntensity = 10;
        o.texture = 3;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryWatercolorOptions o;
        o.texture = 2;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryWatercolorOptions o;
        o.brushDetail = 1;
        const auto a = fn(input.view(), o);
        o.brushDetail = 14;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "watercolor.brushDetail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryWatercolorOptions o;
        o.shadowIntensity = 0;
        const auto a = fn(input.view(), o);
        o.shadowIntensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "watercolor.shadowIntensity slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryWatercolorOptions o;
        o.texture = 1;
        const auto a = fn(input.view(), o);
        o.texture = 3;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "watercolor.texture slider had no effect");
    }
}

void test_accentedEdges() {
    const auto fn = accentedEdges;
    defaults(fn, GalleryAccentedEdgesOptions{});
    {
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = 0.0;
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = 15.0;
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeBrightness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeBrightness = 51.0;
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeBrightness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeBrightness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = 1;
        o.edgeBrightness = 0;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = 14;
        o.edgeBrightness = 50;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryAccentedEdgesOptions o;
        o.edgeWidth = 1;
        const auto a = fn(input.view(), o);
        o.edgeWidth = 14;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "accentedEdges.edgeWidth slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryAccentedEdgesOptions o;
        o.edgeBrightness = 0;
        const auto a = fn(input.view(), o);
        o.edgeBrightness = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "accentedEdges.edgeBrightness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryAccentedEdgesOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "accentedEdges.smoothness slider had no effect");
    }
}

void test_angledStrokes() {
    const auto fn = angledStrokes;
    defaults(fn, GalleryAngledStrokesOptions{});
    {
        GalleryAngledStrokesOptions o;
        o.directionBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.directionBalance = 101.0;
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.directionBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.directionBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.strokeLength = 2.0;
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.strokeLength = 51.0;
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.sharpness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.sharpness = 11.0;
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.sharpness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.sharpness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.directionBalance = 0;
        o.strokeLength = 3;
        o.sharpness = 0;
        extremes(fn, o);
    }
    {
        GalleryAngledStrokesOptions o;
        o.directionBalance = 100;
        o.strokeLength = 50;
        o.sharpness = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryAngledStrokesOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryAngledStrokesOptions o;
        o.directionBalance = 0;
        const auto a = fn(input.view(), o);
        o.directionBalance = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "angledStrokes.directionBalance slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryAngledStrokesOptions o;
        o.strokeLength = 3;
        const auto a = fn(input.view(), o);
        o.strokeLength = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "angledStrokes.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryAngledStrokesOptions o;
        o.sharpness = 0;
        const auto a = fn(input.view(), o);
        o.sharpness = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "angledStrokes.sharpness slider had no effect");
    }
}

void test_crosshatch() {
    const auto fn = crosshatch;
    defaults(fn, GalleryCrosshatchOptions{});
    {
        GalleryCrosshatchOptions o;
        o.strokeLength = 2.0;
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strokeLength = 51.0;
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.sharpness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.sharpness = 21.0;
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.sharpness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.sharpness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strength = 0.0;
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strength = 4.0;
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strokeLength = 3;
        o.sharpness = 0;
        o.strength = 1;
        extremes(fn, o);
    }
    {
        GalleryCrosshatchOptions o;
        o.strokeLength = 50;
        o.sharpness = 20;
        o.strength = 3;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryCrosshatchOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryCrosshatchOptions o;
        o.strokeLength = 3;
        const auto a = fn(input.view(), o);
        o.strokeLength = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "crosshatch.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCrosshatchOptions o;
        o.sharpness = 0;
        const auto a = fn(input.view(), o);
        o.sharpness = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "crosshatch.sharpness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCrosshatchOptions o;
        o.strength = 1;
        const auto a = fn(input.view(), o);
        o.strength = 3;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "crosshatch.strength slider had no effect");
    }
}

void test_darkStrokes() {
    const auto fn = darkStrokes;
    defaults(fn, GalleryDarkStrokesOptions{});
    {
        GalleryDarkStrokesOptions o;
        o.balance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.balance = 11.0;
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.balance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.balance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.blackIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.blackIntensity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.blackIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.blackIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.whiteIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.whiteIntensity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.whiteIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.whiteIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.balance = 0;
        o.blackIntensity = 0;
        o.whiteIntensity = 0;
        extremes(fn, o);
    }
    {
        GalleryDarkStrokesOptions o;
        o.balance = 10;
        o.blackIntensity = 10;
        o.whiteIntensity = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryDarkStrokesOptions o;
        o.balance = 0;
        const auto a = fn(input.view(), o);
        o.balance = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "darkStrokes.balance slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryDarkStrokesOptions o;
        o.blackIntensity = 0;
        const auto a = fn(input.view(), o);
        o.blackIntensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "darkStrokes.blackIntensity slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryDarkStrokesOptions o;
        o.whiteIntensity = 0;
        const auto a = fn(input.view(), o);
        o.whiteIntensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "darkStrokes.whiteIntensity slider had no effect");
    }
}

void test_inkOutlines() {
    const auto fn = inkOutlines;
    defaults(fn, GalleryInkOutlinesOptions{});
    {
        GalleryInkOutlinesOptions o;
        o.strokeLength = 0.0;
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.strokeLength = 51.0;
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.darkIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.darkIntensity = 51.0;
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.darkIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.darkIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.lightIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.lightIntensity = 51.0;
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.lightIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.lightIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.strokeLength = 1;
        o.darkIntensity = 0;
        o.lightIntensity = 0;
        extremes(fn, o);
    }
    {
        GalleryInkOutlinesOptions o;
        o.strokeLength = 50;
        o.darkIntensity = 50;
        o.lightIntensity = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryInkOutlinesOptions o;
        o.strokeLength = 1;
        const auto a = fn(input.view(), o);
        o.strokeLength = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "inkOutlines.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryInkOutlinesOptions o;
        o.darkIntensity = 0;
        const auto a = fn(input.view(), o);
        o.darkIntensity = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "inkOutlines.darkIntensity slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryInkOutlinesOptions o;
        o.lightIntensity = 0;
        const auto a = fn(input.view(), o);
        o.lightIntensity = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "inkOutlines.lightIntensity slider had no effect");
    }
}

void test_spatter() {
    const auto fn = spatter;
    defaults(fn, GallerySpatterOptions{});
    {
        GallerySpatterOptions o;
        o.sprayRadius = -1.0;
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.sprayRadius = 26.0;
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.sprayRadius = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.sprayRadius = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.sprayRadius = 0;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GallerySpatterOptions o;
        o.sprayRadius = 25;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GallerySpatterOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GallerySpatterOptions o;
        o.sprayRadius = 0;
        const auto a = fn(input.view(), o);
        o.sprayRadius = 25;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "spatter.sprayRadius slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySpatterOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "spatter.smoothness slider had no effect");
    }
}

void test_sprayedStrokes() {
    const auto fn = sprayedStrokes;
    defaults(fn, GallerySprayedStrokesOptions{});
    {
        GallerySprayedStrokesOptions o;
        o.strokeLength = -1.0;
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.strokeLength = 21.0;
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.sprayRadius = -1.0;
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.sprayRadius = 26.0;
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.sprayRadius = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.sprayRadius = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.strokeDirection = static_cast<GalleryStrokeDirection>(999);
        rejects(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.strokeLength = 0;
        o.sprayRadius = 0;
        extremes(fn, o);
    }
    {
        GallerySprayedStrokesOptions o;
        o.strokeLength = 20;
        o.sprayRadius = 25;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GallerySprayedStrokesOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GallerySprayedStrokesOptions o;
        o.strokeLength = 0;
        const auto a = fn(input.view(), o);
        o.strokeLength = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sprayedStrokes.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySprayedStrokesOptions o;
        o.sprayRadius = 0;
        const auto a = fn(input.view(), o);
        o.sprayRadius = 25;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sprayedStrokes.sprayRadius slider had no effect");
    }
}

void test_sumiE() {
    const auto fn = sumiE;
    defaults(fn, GallerySumiEOptions{});
    {
        GallerySumiEOptions o;
        o.strokeWidth = 2.0;
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokeWidth = 16.0;
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokeWidth = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokeWidth = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokePressure = -1.0;
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokePressure = 16.0;
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokePressure = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokePressure = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.contrast = -1.0;
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.contrast = 41.0;
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.contrast = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.contrast = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokeWidth = 3;
        o.strokePressure = 0;
        o.contrast = 0;
        extremes(fn, o);
    }
    {
        GallerySumiEOptions o;
        o.strokeWidth = 15;
        o.strokePressure = 15;
        o.contrast = 40;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GallerySumiEOptions o;
        o.strokeWidth = 3;
        const auto a = fn(input.view(), o);
        o.strokeWidth = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sumiE.strokeWidth slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySumiEOptions o;
        o.strokePressure = 0;
        const auto a = fn(input.view(), o);
        o.strokePressure = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sumiE.strokePressure slider had no effect");
    }
    {
        const auto input = pattern();
        GallerySumiEOptions o;
        o.contrast = 0;
        const auto a = fn(input.view(), o);
        o.contrast = 40;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "sumiE.contrast slider had no effect");
    }
}

void test_diffuseGlow() {
    const auto fn = diffuseGlow;
    defaults(fn, GalleryDiffuseGlowOptions{});
    {
        GalleryDiffuseGlowOptions o;
        o.graininess = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.graininess = 11.0;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.graininess = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.graininess = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.glowAmount = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.glowAmount = 21.0;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.glowAmount = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.glowAmount = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.clearAmount = -1.0;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.clearAmount = 21.0;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.clearAmount = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.clearAmount = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.graininess = 0;
        o.glowAmount = 0;
        o.clearAmount = 0;
        extremes(fn, o);
    }
    {
        GalleryDiffuseGlowOptions o;
        o.graininess = 10;
        o.glowAmount = 20;
        o.clearAmount = 20;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryDiffuseGlowOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryDiffuseGlowOptions o;
        o.graininess = 0;
        const auto a = fn(input.view(), o);
        o.graininess = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "diffuseGlow.graininess slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryDiffuseGlowOptions o;
        o.glowAmount = 0;
        const auto a = fn(input.view(), o);
        o.glowAmount = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "diffuseGlow.glowAmount slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryDiffuseGlowOptions o;
        o.clearAmount = 0;
        const auto a = fn(input.view(), o);
        o.clearAmount = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "diffuseGlow.clearAmount slider had no effect");
    }
}

void test_glass() {
    const auto fn = glass;
    defaults(fn, GalleryGlassOptions{});
    {
        GalleryGlassOptions o;
        o.distortion = -1.0;
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.distortion = 21.0;
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.distortion = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.distortion = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.texture = static_cast<GalleryGlassTexture>(999);
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.scaling = 49.0;
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.scaling = 201.0;
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.scaling = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.scaling = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.distortion = 0;
        o.smoothness = 1;
        o.scaling = 50;
        extremes(fn, o);
    }
    {
        GalleryGlassOptions o;
        o.distortion = 20;
        o.smoothness = 15;
        o.scaling = 200;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryGlassOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryGlassOptions o;
        o.distortion = 0;
        const auto a = fn(input.view(), o);
        o.distortion = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "glass.distortion slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryGlassOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "glass.smoothness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryGlassOptions o;
        o.scaling = 50;
        const auto a = fn(input.view(), o);
        o.scaling = 200;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels, "glass.scaling slider had no effect");
    }
}

void test_oceanRipple() {
    const auto fn = oceanRipple;
    defaults(fn, GalleryOceanRippleOptions{});
    {
        GalleryOceanRippleOptions o;
        o.rippleSize = 0.0;
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleSize = 16.0;
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleMagnitude = -1.0;
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleMagnitude = 21.0;
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleMagnitude = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleMagnitude = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleSize = 1;
        o.rippleMagnitude = 0;
        extremes(fn, o);
    }
    {
        GalleryOceanRippleOptions o;
        o.rippleSize = 15;
        o.rippleMagnitude = 20;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryOceanRippleOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryOceanRippleOptions o;
        o.rippleSize = 1;
        const auto a = fn(input.view(), o);
        o.rippleSize = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "oceanRipple.rippleSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryOceanRippleOptions o;
        o.rippleMagnitude = 0;
        const auto a = fn(input.view(), o);
        o.rippleMagnitude = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "oceanRipple.rippleMagnitude slider had no effect");
    }
}

void test_basRelief() {
    const auto fn = basRelief;
    defaults(fn, GalleryBasReliefOptions{});
    {
        GalleryBasReliefOptions o;
        o.detail = 0.0;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.detail = 16.0;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.detail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.detail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.light = static_cast<GalleryLightDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.detail = 1;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GalleryBasReliefOptions o;
        o.detail = 15;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryBasReliefOptions o;
        o.detail = 1;
        const auto a = fn(input.view(), o);
        o.detail = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "basRelief.detail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryBasReliefOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "basRelief.smoothness slider had no effect");
    }
}

void test_chalkCharcoal() {
    const auto fn = chalkCharcoal;
    defaults(fn, GalleryChalkCharcoalOptions{});
    {
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = -1.0;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = 21.0;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.chalkArea = -1.0;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.chalkArea = 21.0;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.chalkArea = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.chalkArea = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.strokePressure = -1.0;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.strokePressure = 6.0;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.strokePressure = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.strokePressure = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = 0;
        o.chalkArea = 0;
        o.strokePressure = 0;
        extremes(fn, o);
    }
    {
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = 20;
        o.chalkArea = 20;
        o.strokePressure = 5;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryChalkCharcoalOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryChalkCharcoalOptions o;
        o.charcoalArea = 0;
        const auto a = fn(input.view(), o);
        o.charcoalArea = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "chalkCharcoal.charcoalArea slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryChalkCharcoalOptions o;
        o.chalkArea = 0;
        const auto a = fn(input.view(), o);
        o.chalkArea = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "chalkCharcoal.chalkArea slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryChalkCharcoalOptions o;
        o.strokePressure = 0;
        const auto a = fn(input.view(), o);
        o.strokePressure = 5;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "chalkCharcoal.strokePressure slider had no effect");
    }
}

void test_charcoal() {
    const auto fn = charcoal;
    defaults(fn, GalleryCharcoalOptions{});
    {
        GalleryCharcoalOptions o;
        o.charcoalThickness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.charcoalThickness = 8.0;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.charcoalThickness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.charcoalThickness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.detail = -1.0;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.detail = 6.0;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.detail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.detail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.lightDarkBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.lightDarkBalance = 101.0;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.lightDarkBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.lightDarkBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.charcoalThickness = 1;
        o.detail = 0;
        o.lightDarkBalance = 0;
        extremes(fn, o);
    }
    {
        GalleryCharcoalOptions o;
        o.charcoalThickness = 7;
        o.detail = 5;
        o.lightDarkBalance = 100;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryCharcoalOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryCharcoalOptions o;
        o.charcoalThickness = 1;
        const auto a = fn(input.view(), o);
        o.charcoalThickness = 7;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "charcoal.charcoalThickness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCharcoalOptions o;
        o.detail = 0;
        const auto a = fn(input.view(), o);
        o.detail = 5;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "charcoal.detail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCharcoalOptions o;
        o.lightDarkBalance = 0;
        const auto a = fn(input.view(), o);
        o.lightDarkBalance = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "charcoal.lightDarkBalance slider had no effect");
    }
}

void test_chrome() {
    const auto fn = chrome;
    defaults(fn, GalleryChromeOptions{});
    {
        GalleryChromeOptions o;
        o.detail = -1.0;
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.detail = 11.0;
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.detail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.detail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.smoothness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.smoothness = 11.0;
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.detail = 0;
        o.smoothness = 0;
        extremes(fn, o);
    }
    {
        GalleryChromeOptions o;
        o.detail = 10;
        o.smoothness = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryChromeOptions o;
        o.detail = 0;
        const auto a = fn(input.view(), o);
        o.detail = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels, "chrome.detail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryChromeOptions o;
        o.smoothness = 0;
        const auto a = fn(input.view(), o);
        o.smoothness = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "chrome.smoothness slider had no effect");
    }
}

void test_conteCrayon() {
    const auto fn = conteCrayon;
    defaults(fn, GalleryConteCrayonOptions{});
    {
        GalleryConteCrayonOptions o;
        o.foregroundLevel = 0.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foregroundLevel = 16.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foregroundLevel = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foregroundLevel = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.backgroundLevel = 0.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.backgroundLevel = 16.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.backgroundLevel = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.backgroundLevel = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.texture = static_cast<GalleryTexture>(999);
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.scaling = 49.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.scaling = 201.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.scaling = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.scaling = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.relief = -1.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.relief = 51.0;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.relief = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.relief = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.light = static_cast<GalleryLightDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foregroundLevel = 1;
        o.backgroundLevel = 1;
        o.scaling = 50;
        o.relief = 0;
        extremes(fn, o);
    }
    {
        GalleryConteCrayonOptions o;
        o.foregroundLevel = 15;
        o.backgroundLevel = 15;
        o.scaling = 200;
        o.relief = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryConteCrayonOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryConteCrayonOptions o;
        o.foregroundLevel = 1;
        const auto a = fn(input.view(), o);
        o.foregroundLevel = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "conteCrayon.foregroundLevel slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryConteCrayonOptions o;
        o.backgroundLevel = 1;
        const auto a = fn(input.view(), o);
        o.backgroundLevel = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "conteCrayon.backgroundLevel slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryConteCrayonOptions o;
        o.scaling = 50;
        const auto a = fn(input.view(), o);
        o.scaling = 200;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "conteCrayon.scaling slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryConteCrayonOptions o;
        o.relief = 0;
        const auto a = fn(input.view(), o);
        o.relief = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "conteCrayon.relief slider had no effect");
    }
}

void test_graphicPen() {
    const auto fn = graphicPen;
    defaults(fn, GalleryGraphicPenOptions{});
    {
        GalleryGraphicPenOptions o;
        o.strokeLength = 0.0;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.strokeLength = 16.0;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.strokeLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.strokeLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.lightDarkBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.lightDarkBalance = 101.0;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.lightDarkBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.lightDarkBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.strokeDirection = static_cast<GalleryStrokeDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.strokeLength = 1;
        o.lightDarkBalance = 0;
        extremes(fn, o);
    }
    {
        GalleryGraphicPenOptions o;
        o.strokeLength = 15;
        o.lightDarkBalance = 100;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryGraphicPenOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryGraphicPenOptions o;
        o.strokeLength = 1;
        const auto a = fn(input.view(), o);
        o.strokeLength = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "graphicPen.strokeLength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryGraphicPenOptions o;
        o.lightDarkBalance = 0;
        const auto a = fn(input.view(), o);
        o.lightDarkBalance = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "graphicPen.lightDarkBalance slider had no effect");
    }
}

void test_halftonePattern() {
    const auto fn = halftonePattern;
    defaults(fn, GalleryHalftonePatternOptions{});
    {
        GalleryHalftonePatternOptions o;
        o.size = 0.0;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.size = 13.0;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.size = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.size = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.contrast = -1.0;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.contrast = 51.0;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.contrast = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.contrast = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.patternType = static_cast<GalleryPatternType>(999);
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.size = 1;
        o.contrast = 0;
        extremes(fn, o);
    }
    {
        GalleryHalftonePatternOptions o;
        o.size = 12;
        o.contrast = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryHalftonePatternOptions o;
        o.size = 1;
        const auto a = fn(input.view(), o);
        o.size = 12;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "halftonePattern.size slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryHalftonePatternOptions o;
        o.contrast = 0;
        const auto a = fn(input.view(), o);
        o.contrast = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "halftonePattern.contrast slider had no effect");
    }
}

void test_notePaper() {
    const auto fn = notePaper;
    defaults(fn, GalleryNotePaperOptions{});
    {
        GalleryNotePaperOptions o;
        o.imageBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.imageBalance = 51.0;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.imageBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.imageBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.graininess = -1.0;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.graininess = 21.0;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.graininess = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.graininess = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.relief = -1.0;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.relief = 26.0;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.relief = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.relief = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.imageBalance = 0;
        o.graininess = 0;
        o.relief = 0;
        extremes(fn, o);
    }
    {
        GalleryNotePaperOptions o;
        o.imageBalance = 50;
        o.graininess = 20;
        o.relief = 25;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryNotePaperOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryNotePaperOptions o;
        o.imageBalance = 0;
        const auto a = fn(input.view(), o);
        o.imageBalance = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "notePaper.imageBalance slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryNotePaperOptions o;
        o.graininess = 0;
        const auto a = fn(input.view(), o);
        o.graininess = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "notePaper.graininess slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryNotePaperOptions o;
        o.relief = 0;
        const auto a = fn(input.view(), o);
        o.relief = 25;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "notePaper.relief slider had no effect");
    }
}

void test_photocopy() {
    const auto fn = photocopy;
    defaults(fn, GalleryPhotocopyOptions{});
    {
        GalleryPhotocopyOptions o;
        o.detail = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.detail = 25.0;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.detail = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.detail = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.darkness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.darkness = 51.0;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.darkness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.darkness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.detail = 1;
        o.darkness = 1;
        extremes(fn, o);
    }
    {
        GalleryPhotocopyOptions o;
        o.detail = 24;
        o.darkness = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPhotocopyOptions o;
        o.detail = 1;
        const auto a = fn(input.view(), o);
        o.detail = 24;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "photocopy.detail slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPhotocopyOptions o;
        o.darkness = 1;
        const auto a = fn(input.view(), o);
        o.darkness = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "photocopy.darkness slider had no effect");
    }
}

void test_plaster() {
    const auto fn = plaster;
    defaults(fn, GalleryPlasterOptions{});
    {
        GalleryPlasterOptions o;
        o.imageBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.imageBalance = 51.0;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.imageBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.imageBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.light = static_cast<GalleryLightDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.imageBalance = 0;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GalleryPlasterOptions o;
        o.imageBalance = 50;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPlasterOptions o;
        o.imageBalance = 0;
        const auto a = fn(input.view(), o);
        o.imageBalance = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "plaster.imageBalance slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPlasterOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "plaster.smoothness slider had no effect");
    }
}

void test_reticulation() {
    const auto fn = reticulation;
    defaults(fn, GalleryReticulationOptions{});
    {
        GalleryReticulationOptions o;
        o.density = -1.0;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.density = 51.0;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.density = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.density = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foregroundLevel = -1.0;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foregroundLevel = 51.0;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foregroundLevel = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foregroundLevel = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.backgroundLevel = -1.0;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.backgroundLevel = 51.0;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.backgroundLevel = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.backgroundLevel = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.density = 0;
        o.foregroundLevel = 0;
        o.backgroundLevel = 0;
        extremes(fn, o);
    }
    {
        GalleryReticulationOptions o;
        o.density = 50;
        o.foregroundLevel = 50;
        o.backgroundLevel = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryReticulationOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryReticulationOptions o;
        o.density = 0;
        const auto a = fn(input.view(), o);
        o.density = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "reticulation.density slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryReticulationOptions o;
        o.foregroundLevel = 0;
        const auto a = fn(input.view(), o);
        o.foregroundLevel = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "reticulation.foregroundLevel slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryReticulationOptions o;
        o.backgroundLevel = 0;
        const auto a = fn(input.view(), o);
        o.backgroundLevel = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "reticulation.backgroundLevel slider had no effect");
    }
}

void test_stamp() {
    const auto fn = stamp;
    defaults(fn, GalleryStampOptions{});
    {
        GalleryStampOptions o;
        o.lightDarkBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.lightDarkBalance = 51.0;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.lightDarkBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.lightDarkBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.smoothness = 51.0;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStampOptions o;
        o.lightDarkBalance = 0;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GalleryStampOptions o;
        o.lightDarkBalance = 50;
        o.smoothness = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryStampOptions o;
        o.lightDarkBalance = 0;
        const auto a = fn(input.view(), o);
        o.lightDarkBalance = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "stamp.lightDarkBalance slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryStampOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "stamp.smoothness slider had no effect");
    }
}

void test_tornEdges() {
    const auto fn = tornEdges;
    defaults(fn, GalleryTornEdgesOptions{});
    {
        GalleryTornEdgesOptions o;
        o.imageBalance = -1.0;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.imageBalance = 51.0;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.imageBalance = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.imageBalance = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.contrast = 0.0;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.contrast = 26.0;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.contrast = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.contrast = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.imageBalance = 0;
        o.smoothness = 1;
        o.contrast = 1;
        extremes(fn, o);
    }
    {
        GalleryTornEdgesOptions o;
        o.imageBalance = 50;
        o.smoothness = 15;
        o.contrast = 25;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryTornEdgesOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryTornEdgesOptions o;
        o.imageBalance = 0;
        const auto a = fn(input.view(), o);
        o.imageBalance = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "tornEdges.imageBalance slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryTornEdgesOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "tornEdges.smoothness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryTornEdgesOptions o;
        o.contrast = 1;
        const auto a = fn(input.view(), o);
        o.contrast = 25;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "tornEdges.contrast slider had no effect");
    }
}

void test_waterPaper() {
    const auto fn = waterPaper;
    defaults(fn, GalleryWaterPaperOptions{});
    {
        GalleryWaterPaperOptions o;
        o.fiberLength = 2.0;
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.fiberLength = 51.0;
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.fiberLength = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.fiberLength = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.brightness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.brightness = 101.0;
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.brightness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.brightness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.contrast = -1.0;
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.contrast = 101.0;
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.contrast = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.contrast = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.fiberLength = 3;
        o.brightness = 0;
        o.contrast = 0;
        extremes(fn, o);
    }
    {
        GalleryWaterPaperOptions o;
        o.fiberLength = 50;
        o.brightness = 100;
        o.contrast = 100;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryWaterPaperOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryWaterPaperOptions o;
        o.fiberLength = 3;
        const auto a = fn(input.view(), o);
        o.fiberLength = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "waterPaper.fiberLength slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryWaterPaperOptions o;
        o.brightness = 0;
        const auto a = fn(input.view(), o);
        o.brightness = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "waterPaper.brightness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryWaterPaperOptions o;
        o.contrast = 0;
        const auto a = fn(input.view(), o);
        o.contrast = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "waterPaper.contrast slider had no effect");
    }
}

void test_glowingEdges() {
    const auto fn = glowingEdges;
    defaults(fn, GalleryGlowingEdgesOptions{});
    {
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = 0.0;
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = 15.0;
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeBrightness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeBrightness = 21.0;
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeBrightness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeBrightness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.smoothness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.smoothness = 16.0;
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.smoothness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.smoothness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = 1;
        o.edgeBrightness = 0;
        o.smoothness = 1;
        extremes(fn, o);
    }
    {
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = 14;
        o.edgeBrightness = 20;
        o.smoothness = 15;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryGlowingEdgesOptions o;
        o.edgeWidth = 1;
        const auto a = fn(input.view(), o);
        o.edgeWidth = 14;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "glowingEdges.edgeWidth slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryGlowingEdgesOptions o;
        o.edgeBrightness = 0;
        const auto a = fn(input.view(), o);
        o.edgeBrightness = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "glowingEdges.edgeBrightness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryGlowingEdgesOptions o;
        o.smoothness = 1;
        const auto a = fn(input.view(), o);
        o.smoothness = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "glowingEdges.smoothness slider had no effect");
    }
}

void test_craquelure() {
    const auto fn = craquelure;
    defaults(fn, GalleryCraquelureOptions{});
    {
        GalleryCraquelureOptions o;
        o.crackSpacing = 1.0;
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackSpacing = 101.0;
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackSpacing = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackSpacing = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackDepth = -1.0;
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackDepth = 11.0;
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackDepth = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackDepth = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackBrightness = -1.0;
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackBrightness = 11.0;
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackBrightness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackBrightness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackSpacing = 2;
        o.crackDepth = 0;
        o.crackBrightness = 0;
        extremes(fn, o);
    }
    {
        GalleryCraquelureOptions o;
        o.crackSpacing = 100;
        o.crackDepth = 10;
        o.crackBrightness = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryCraquelureOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryCraquelureOptions o;
        o.crackSpacing = 2;
        const auto a = fn(input.view(), o);
        o.crackSpacing = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "craquelure.crackSpacing slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCraquelureOptions o;
        o.crackDepth = 0;
        const auto a = fn(input.view(), o);
        o.crackDepth = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "craquelure.crackDepth slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryCraquelureOptions o;
        o.crackBrightness = 0;
        const auto a = fn(input.view(), o);
        o.crackBrightness = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "craquelure.crackBrightness slider had no effect");
    }
}

void test_grain() {
    const auto fn = grain;
    defaults(fn, GalleryGrainOptions{});
    {
        GalleryGrainOptions o;
        o.intensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.intensity = 101.0;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.intensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.intensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.contrast = -1.0;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.contrast = 101.0;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.contrast = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.contrast = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.grainType = static_cast<GalleryGrainType>(999);
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.background[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.background[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.background[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.intensity = 0;
        o.contrast = 0;
        extremes(fn, o);
    }
    {
        GalleryGrainOptions o;
        o.intensity = 100;
        o.contrast = 100;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryGrainOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryGrainOptions o;
        o.intensity = 0;
        const auto a = fn(input.view(), o);
        o.intensity = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "grain.intensity slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryGrainOptions o;
        o.contrast = 0;
        const auto a = fn(input.view(), o);
        o.contrast = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "grain.contrast slider had no effect");
    }
}

void test_mosaicTiles() {
    const auto fn = mosaicTiles;
    defaults(fn, GalleryMosaicTilesOptions{});
    {
        GalleryMosaicTilesOptions o;
        o.tileSize = 1.0;
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.tileSize = 101.0;
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.tileSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.tileSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.groutWidth = 0.0;
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.groutWidth = 16.0;
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.groutWidth = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.groutWidth = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.lightenGrout = -1.0;
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.lightenGrout = 11.0;
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.lightenGrout = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.lightenGrout = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.tileSize = 2;
        o.groutWidth = 1;
        o.lightenGrout = 0;
        extremes(fn, o);
    }
    {
        GalleryMosaicTilesOptions o;
        o.tileSize = 100;
        o.groutWidth = 15;
        o.lightenGrout = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryMosaicTilesOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryMosaicTilesOptions o;
        o.tileSize = 2;
        const auto a = fn(input.view(), o);
        o.tileSize = 100;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "mosaicTiles.tileSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryMosaicTilesOptions o;
        o.groutWidth = 1;
        const auto a = fn(input.view(), o);
        o.groutWidth = 15;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "mosaicTiles.groutWidth slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryMosaicTilesOptions o;
        o.lightenGrout = 0;
        const auto a = fn(input.view(), o);
        o.lightenGrout = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "mosaicTiles.lightenGrout slider had no effect");
    }
}

void test_patchwork() {
    const auto fn = patchwork;
    defaults(fn, GalleryPatchworkOptions{});
    {
        GalleryPatchworkOptions o;
        o.squareSize = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.squareSize = 11.0;
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.squareSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.squareSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.relief = -1.0;
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.relief = 26.0;
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.relief = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.relief = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.squareSize = 0;
        o.relief = 0;
        extremes(fn, o);
    }
    {
        GalleryPatchworkOptions o;
        o.squareSize = 10;
        o.relief = 25;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryPatchworkOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryPatchworkOptions o;
        o.squareSize = 0;
        const auto a = fn(input.view(), o);
        o.squareSize = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "patchwork.squareSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryPatchworkOptions o;
        o.relief = 0;
        const auto a = fn(input.view(), o);
        o.relief = 25;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "patchwork.relief slider had no effect");
    }
}

void test_stainedGlass() {
    const auto fn = stainedGlass;
    defaults(fn, GalleryStainedGlassOptions{});
    {
        GalleryStainedGlassOptions o;
        o.cellSize = 1.0;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.cellSize = 51.0;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.cellSize = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.cellSize = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.borderThickness = 0.0;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.borderThickness = 21.0;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.borderThickness = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.borderThickness = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.lightIntensity = -1.0;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.lightIntensity = 11.0;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.lightIntensity = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.lightIntensity = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.foreground[1] = -0.1;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.foreground[1] = 1.1;
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.foreground[1] = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.cellSize = 2;
        o.borderThickness = 1;
        o.lightIntensity = 0;
        extremes(fn, o);
    }
    {
        GalleryStainedGlassOptions o;
        o.cellSize = 50;
        o.borderThickness = 20;
        o.lightIntensity = 10;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryStainedGlassOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryStainedGlassOptions o;
        o.cellSize = 2;
        const auto a = fn(input.view(), o);
        o.cellSize = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "stainedGlass.cellSize slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryStainedGlassOptions o;
        o.borderThickness = 1;
        const auto a = fn(input.view(), o);
        o.borderThickness = 20;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "stainedGlass.borderThickness slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryStainedGlassOptions o;
        o.lightIntensity = 0;
        const auto a = fn(input.view(), o);
        o.lightIntensity = 10;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "stainedGlass.lightIntensity slider had no effect");
    }
}

void test_texturizer() {
    const auto fn = texturizer;
    defaults(fn, GalleryTexturizerOptions{});
    {
        GalleryTexturizerOptions o;
        o.texture = static_cast<GalleryTexture>(999);
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.scaling = 49.0;
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.scaling = 201.0;
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.scaling = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.scaling = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.relief = -1.0;
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.relief = 51.0;
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.relief = std::numeric_limits<double>::quiet_NaN();
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.relief = std::numeric_limits<double>::infinity();
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.light = static_cast<GalleryLightDirection>(999);
        rejects(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.scaling = 50;
        o.relief = 0;
        extremes(fn, o);
    }
    {
        GalleryTexturizerOptions o;
        o.scaling = 200;
        o.relief = 50;
        extremes(fn, o);
    }
    {
        const auto input = pattern();
        GalleryTexturizerOptions o;
        const auto a = fn(input.view(), o);
        o.seed = 314159;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "Seed did not affect procedural component");
    }
    {
        const auto input = pattern();
        GalleryTexturizerOptions o;
        o.scaling = 50;
        const auto a = fn(input.view(), o);
        o.scaling = 200;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "texturizer.scaling slider had no effect");
    }
    {
        const auto input = pattern();
        GalleryTexturizerOptions o;
        o.relief = 0;
        const auto a = fn(input.view(), o);
        o.relief = 50;
        const auto b = fn(input.view(), o);
        require(bool(a) && bool(b) && a->pixels != b->pixels,
                "texturizer.relief slider had no effect");
    }
}

// Independent single-pixel oracles do not call implementation helpers.
double step(double a, double b, double x) {
    const auto t = std::clamp((x - a) / (b - a), 0., 1.);
    return t * t * (3 - 2 * t);
}
void oracles() {
    Image image{1, 1, {{.2f, .4f, .6f, .4f}}};
    GalleryCutoutOptions c;
    c.numberOfLevels = 4;
    c.edgeSimplicity = 0;
    const auto cut = cutout(image.view(), c);
    require(bool(cut), "Cutout oracle failed");
    const auto luminance = .299 * .2f + .587 * .4f + .114 * .6f;
    const auto k = (std::round(luminance * 3) / 3) / luminance;
    const auto expected = [&](double v) { return std::round(std::clamp(v * k, 0., 1.) * 7) / 7; };
    require(std::abs(cut->pixels[0].r - expected(.2f)) < 1e-7 &&
                std::abs(cut->pixels[0].g - expected(.4f)) < 1e-7 &&
                std::abs(cut->pixels[0].b - expected(.6f)) < 1e-7,
            "Cutout radiometric oracle mismatch");
    GalleryFilmGrainOptions film;
    film.grain = 0;
    film.highlightArea = 10;
    film.intensity = 3;
    const auto grain = galleryFilmGrain(image.view(), film);
    const auto weight = step(1 - 10. / 20 * .75, 1.02, luminance) * (1 - 3. / 10 * .6);
    require(std::abs(grain->pixels[0].g - (.4f + (1 - .4f) * weight)) < 1e-7,
            "Film highlight oracle mismatch");
    GalleryNeonGlowOptions neon;
    neon.glowBrightness = 0;
    neon.foreground = {.1, .2, .3};
    neon.background = {.9, .8, .7};
    const auto glow = neonGlow(image.view(), neon);
    require(std::abs(glow->pixels[0].r - (.1 + .8 * luminance)) < 1e-7,
            "Neon base color oracle mismatch");
    GalleryChromeOptions metal;
    const auto chromeOut = chrome(image.view(), metal);
    const auto v =
        .5 + .5 * std::sin(luminance * (2.5 + metal.detail / 10 * 3) * 2 * std::acos(-1));
    require(std::abs(chromeOut->pixels[0].r - step(.05, .95, v)) < 1e-7,
            "Chrome wave oracle mismatch");
    GalleryStampOptions stampOptions;
    const auto stampOut = stamp(image.view(), stampOptions);
    const auto mask = step(.5 + .02, .5 - .02, luminance);
    require(std::abs(stampOut->pixels[0].r - (1 - mask)) < 1e-7, "Stamp threshold oracle mismatch");
    GalleryTexturizerOptions tx;
    tx.relief = 0;
    require(texturizer(image.view(), tx)->pixels == image.pixels, "Zero relief changed pixels");
    GalleryGlassOptions glassOptions;
    glassOptions.distortion = 0;
    require(glass(image.view(), glassOptions)->pixels == image.pixels,
            "Zero glass displacement changed pixels");
    GalleryOceanRippleOptions ripple;
    ripple.rippleMagnitude = 0;
    require(oceanRipple(image.view(), ripple)->pixels == image.pixels,
            "Zero ripple displacement changed pixels");
    GallerySpatterOptions spat;
    spat.sprayRadius = 0;
    require(spatter(image.view(), spat)->pixels == image.pixels,
            "Zero spatter displacement changed pixels");
}

void stack() {
    const auto input = pattern();
    const std::array<GalleryEffect, 2> effects{GalleryCutoutOptions{}, GalleryPosterEdgesOptions{}};
    const auto stacked = filterGallery(input.view(), effects);
    const auto first = cutout(input.view());
    const auto sequential = posterEdges(first->view());
    require(bool(stacked) && stacked->pixels == sequential->pixels,
            "Gallery stack order differs from sequential operations");
    const std::array<GalleryEffect, 2> reversed{GalleryPosterEdgesOptions{},
                                                GalleryCutoutOptions{}};
    require(filterGallery(input.view(), reversed)->pixels != stacked->pixels,
            "Gallery ordering is not observable");
    const auto empty = filterGallery(input.view(), {});
    require(bool(empty) && empty->pixels == input.pixels &&
                empty->pixels.data() != input.pixels.data(),
            "Empty gallery stack does not return detached storage");
    require(!filterGallery(ImageView{}, {}), "Invalid empty gallery input accepted");
    std::array<GalleryEffect, 65> tooMany;
    require(!filterGallery(input.view(), tooMany), "Over-limit gallery stack accepted");
    GalleryGlassOptions invalid;
    invalid.smoothness = 0;
    const std::array<GalleryEffect, 3> bad{GalleryCutoutOptions{}, invalid, GalleryStampOptions{}};
    const auto rejected = filterGallery(input.view(), bad);
    require(!rejected && rejected.error().code == ErrorCode::InvalidParameter,
            "Invalid gallery effect did not fail stack");
    require(input.pixels == pattern().pixels, "Failed gallery stack mutated source");
    std::array<GalleryEffect, 64> limit;
    for (auto &effect : limit) {
        GalleryTexturizerOptions zero;
        zero.relief = 0;
        effect = zero;
    }
    const auto accepted = filterGallery(input.view(), limit);
    require(bool(accepted) && accepted->pixels == input.pixels, "Maximum gallery stack failed");
}
} // namespace
int main(int argc, char **argv) {
    const std::string_view selected = argc > 1 ? argv[1] : "all";
    if (selected == "all" || selected == "coloredPencil")
        test_coloredPencil();
    if (selected == "all" || selected == "cutout")
        test_cutout();
    if (selected == "all" || selected == "dryBrush")
        test_dryBrush();
    if (selected == "all" || selected == "galleryFilmGrain")
        test_galleryFilmGrain();
    if (selected == "all" || selected == "fresco")
        test_fresco();
    if (selected == "all" || selected == "neonGlow")
        test_neonGlow();
    if (selected == "all" || selected == "paintDaubs")
        test_paintDaubs();
    if (selected == "all" || selected == "paletteKnife")
        test_paletteKnife();
    if (selected == "all" || selected == "plasticWrap")
        test_plasticWrap();
    if (selected == "all" || selected == "posterEdges")
        test_posterEdges();
    if (selected == "all" || selected == "roughPastels")
        test_roughPastels();
    if (selected == "all" || selected == "smudgeStick")
        test_smudgeStick();
    if (selected == "all" || selected == "sponge")
        test_sponge();
    if (selected == "all" || selected == "underpainting")
        test_underpainting();
    if (selected == "all" || selected == "watercolor")
        test_watercolor();
    if (selected == "all" || selected == "accentedEdges")
        test_accentedEdges();
    if (selected == "all" || selected == "angledStrokes")
        test_angledStrokes();
    if (selected == "all" || selected == "crosshatch")
        test_crosshatch();
    if (selected == "all" || selected == "darkStrokes")
        test_darkStrokes();
    if (selected == "all" || selected == "inkOutlines")
        test_inkOutlines();
    if (selected == "all" || selected == "spatter")
        test_spatter();
    if (selected == "all" || selected == "sprayedStrokes")
        test_sprayedStrokes();
    if (selected == "all" || selected == "sumiE")
        test_sumiE();
    if (selected == "all" || selected == "diffuseGlow")
        test_diffuseGlow();
    if (selected == "all" || selected == "glass")
        test_glass();
    if (selected == "all" || selected == "oceanRipple")
        test_oceanRipple();
    if (selected == "all" || selected == "basRelief")
        test_basRelief();
    if (selected == "all" || selected == "chalkCharcoal")
        test_chalkCharcoal();
    if (selected == "all" || selected == "charcoal")
        test_charcoal();
    if (selected == "all" || selected == "chrome")
        test_chrome();
    if (selected == "all" || selected == "conteCrayon")
        test_conteCrayon();
    if (selected == "all" || selected == "graphicPen")
        test_graphicPen();
    if (selected == "all" || selected == "halftonePattern")
        test_halftonePattern();
    if (selected == "all" || selected == "notePaper")
        test_notePaper();
    if (selected == "all" || selected == "photocopy")
        test_photocopy();
    if (selected == "all" || selected == "plaster")
        test_plaster();
    if (selected == "all" || selected == "reticulation")
        test_reticulation();
    if (selected == "all" || selected == "stamp")
        test_stamp();
    if (selected == "all" || selected == "tornEdges")
        test_tornEdges();
    if (selected == "all" || selected == "waterPaper")
        test_waterPaper();
    if (selected == "all" || selected == "glowingEdges")
        test_glowingEdges();
    if (selected == "all" || selected == "craquelure")
        test_craquelure();
    if (selected == "all" || selected == "grain")
        test_grain();
    if (selected == "all" || selected == "mosaicTiles")
        test_mosaicTiles();
    if (selected == "all" || selected == "patchwork")
        test_patchwork();
    if (selected == "all" || selected == "stainedGlass")
        test_stainedGlass();
    if (selected == "all" || selected == "texturizer")
        test_texturizer();
    if (selected == "all" || selected == "stack")
        stack();
    if (selected == "all" || selected == "oracles")
        oracles();
    require(checks > 1, "Unknown gallery test selection");
    std::cout << "Gallery checks passed: " << checks << '\n';
}
