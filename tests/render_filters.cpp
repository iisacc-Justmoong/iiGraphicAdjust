#include <cmath>
#include <functional>
#include <iiGraphicAdjust/RenderFilters.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <string>
using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool pass, const char *why) {
    if (!pass) {
        std::cerr << why << '\n';
        ++failures;
    }
}
bool near(double a, double b, double e = 2e-6) {
    return std::abs(a - b) < e;
}
void bounded(const Result<Image> &r) {
    check(bool(r), "renderer succeeds");
    if (!r)
        return;
    for (auto p : r->pixels)
        check(std::isfinite(p.r) && p.r >= 0 && p.r <= 1 && std::isfinite(p.g) && p.g >= 0 &&
                  p.g <= 1 && std::isfinite(p.b) && p.b >= 0 && p.b <= 1 && p.a >= 0 && p.a <= 1,
              "finite normalized result");
}
double coverage(const Image &i) {
    double sum = 0;
    for (auto p : i.pixels)
        sum += p.a;
    return sum;
}
} // namespace
int main(int argc, char **argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    Image flat{48, 32, std::vector<Rgba>(1536, {.2F, .3F, .4F, 1})};
    if (mode == "noise") {
        CloudsOptions o;
        o.seed = 42;
        auto a = clouds(flat.view(), o), b = clouds(flat.view(), o);
        o.seed = 43;
        auto c = clouds(flat.view(), o);
        bounded(a);
        check(a && b && c && a->pixels == b->pixels && a->pixels != c->pixels,
              "clouds deterministic and seeded");
        o.seed = 42;
        auto d = differenceClouds(flat.view(), o);
        bounded(d);
        if (a && d)
            for (std::size_t i = 0; i < flat.pixels.size(); ++i)
                check(near(d->pixels[i].r, std::abs(.2F - a->pixels[i].r)) && d->pixels[i].a == 1,
                      "difference clouds absolute-channel oracle");
        o.foreground = o.background = {.25F, .5F, .75F, .3F};
        auto solid = clouds(flat.view(), o);
        check(solid && solid->pixels[0] == o.foreground, "equal cloud endpoints constant RGBA");
        FibersOptions f;
        f.foreground = f.background = o.foreground;
        auto fibSolid = fibers(flat.view(), f);
        check(fibSolid && fibSolid->pixels[0] == o.foreground,
              "equal fiber endpoints constant RGBA");
        f = {};
        f.seed = 4;
        auto fa = fibers(flat.view(), f), fb = fibers(flat.view(), f);
        f.strength = 64;
        auto fc = fibers(flat.view(), f);
        check(fa && fb && fc && fa->pixels == fb->pixels && fa->pixels != fc->pixels,
              "fiber directional scale effective and seeded");
        CloudsOptions tiny;
        tiny.foreground = {1, 0, 0, std::numeric_limits<float>::denorm_min()};
        tiny.background = {0, 1, 0, 0};
        auto faint = clouds(flat.view(), tiny);
        bool canonical = bool(faint);
        if (faint)
            for (auto p : faint->pixels)
                canonical &= p.a > 0 || p == Rgba{0, 0, 0, 0};
        check(canonical, "generated cloud alpha rounded to zero has canonical RGB");
    } else if (mode == "light") {
        LightingEffectsOptions o;
        o.lights.clear();
        o.ambience = 100;
        o.gloss = -100;
        auto ambient = lightingEffects(flat.view(), o);
        bounded(ambient);
        check(ambient && near(ambient->pixels[0].r, .1) && near(ambient->pixels[0].g, .15) &&
                  near(ambient->pixels[0].b, .2),
              "lighting ambience independent scalar oracle");
        o.exposure = 50;
        auto exposure = lightingEffects(flat.view(), o);
        check(exposure && near(exposure->pixels[0].r, .2) && near(exposure->pixels[0].g, .3),
              "lighting exposure doubles light");
        o = {};
        o.gloss = -100;
        o.ambience = 0;
        o.lights[0].kind = LightKind::Infinite;
        o.lights[0].intensity = 50;
        o.lights[0].elevation = 90;
        auto top = lightingEffects(flat.view(), o);
        check(top && near(top->pixels[0].r, .2), "normal incidence Lambertian oracle");
        o.lights[0].elevation = 0;
        auto side = lightingEffects(flat.view(), o);
        check(side && side->pixels[0].r < 1e-6, "grazing illumination of flat texture");
        RelightOptions r;
        r.intensity = 0;
        auto id = relight(flat.view(), r);
        check(id && id->pixels == flat.pixels, "zero relight exact identity");
        r = {0, 60, 80, 10, 0, 25};
        auto lit = relight(flat.view(), r);
        double luminance = .2126 * .2F + .7152 * .3F + .0722 * .4F,
               shade = .1 + .8 * std::sin(std::numbers::pi / 3);
        check(lit && near(lit->pixels[700].r, .2F / luminance * shade),
              "flat relight shading oracle");
        auto faint = flat;
        for (auto &p : faint.pixels)
            p.a = 1e-20F;
        r.intensity = 10;
        r.ambient = 0;
        auto faintLight = relight(faint.view(), r), opaqueLight = relight(flat.view(), r);
        check(faintLight && opaqueLight &&
                  near(faintLight->pixels[700].r, opaqueLight->pixels[700].r) &&
                  near(faintLight->pixels[700].g, opaqueLight->pixels[700].g) &&
                  faintLight->pixels[700].a == 1e-20F,
              "alpha-normalized relight is invariant to uniform representable tiny coverage");
        r = {0, 60, 80, 10, 0, 25};
        r.warmth = 80;
        auto warm = relight(flat.view(), r);
        r.warmth = -80;
        auto cool = relight(flat.view(), r);
        check(warm && cool && warm->pixels[0].r > cool->pixels[0].r &&
                  warm->pixels[0].b < cool->pixels[0].b,
              "relight warmth direction");
        LensFlareOptions flare;
        flare.brightness = 0;
        auto noflare = lensFlare(flat.view(), flare);
        check(noflare && noflare->pixels == flat.pixels, "zero flare exact identity");
        flare.brightness = 100;
        std::vector<Rgba> prior;
        for (int k = 0; k < 4; ++k) {
            flare.lens = FlareLens(k);
            auto out = lensFlare(flat.view(), flare);
            bounded(out);
            check(out && out->pixels[16 * 48 + 24].r > .2, "flare center brightened");
            if (out) {
                if (k)
                    check(out->pixels != prior, "flare lens elements vary");
                prior = out->pixels;
            }
        }
    } else if (mode == "procedural") {
        Image clear{64, 48, std::vector<Rgba>(3072, {0, 0, 0, 0})};
        FlameOptions f;
        f.width = 8;
        f.length = 24;
        f.flameLines = 6;
        f.quality = 0;
        f.paths = {{{8, 40}, {56, 32}}};
        f.interval = 16;
        f.seed = 42;
        for (int k = 0; k < 6; ++k) {
            f.type = FlameType(k);
            auto a = flame(clear.view(), f), b = flame(clear.view(), f);
            bounded(a);
            check(a && b && a->pixels == b->pixels && coverage(*a) > 0,
                  "each flame placement renders deterministically");
        }
        f.type = FlameType::CandleLight;
        f.opacity = 0;
        auto empty = flame(clear.view(), f);
        check(empty && empty->pixels == clear.pixels, "flame opacity zero exact identity");
        f.opacity = 75;
        auto fa = flame(clear.view(), f);
        f.seed = 43;
        auto fb = flame(clear.view(), f);
        check(fa && fb && fa->pixels != fb->pixels, "flame seed changes strokes");
        f.color = Rgba{.1F, .8F, .4F, 0};
        for (int k = 0; k < 6; ++k) {
            f.type = FlameType(k);
            auto transparent = flame(clear.view(), f);
            check(transparent && transparent->pixels == clear.pixels,
                  "custom transparent flame color suppresses strands, glow and candle core");
        }
        for (unsigned k = 1; k <= 34; ++k) {
            TreeOptions t;
            t.baseTreeType = k;
            t.seed = 43;
            auto a = tree(clear.view(), t);
            bounded(a);
            check(a && coverage(*a) > 0, "all 34 tree types paint real geometry");
        }
        TreeOptions t;
        t.baseTreeType = 7;
        auto ta = tree(clear.view(), t), tb = tree(clear.view(), t);
        t.leavesAmount = 0;
        auto bare = tree(clear.view(), t);
        check(ta && tb && bare && ta->pixels == tb->pixels && coverage(*ta) > coverage(*bare),
              "tree repeat and leaves amount");
        for (int k = 0; k < 16; ++k) {
            PictureFrameOptions o;
            o.style = PictureFrameStyle(k);
            auto a = pictureFrame(clear.view(), o), b = pictureFrame(clear.view(), o);
            bounded(a);
            check(a && b && a->pixels == b->pixels && coverage(*a) > 0,
                  "all 16 frame designs render deterministically");
        }
        PictureFrameOptions frame;
        frame.style = PictureFrameStyle::VineWithFlowers;
        auto pa = pictureFrame(clear.view(), frame);
        frame.fade = 100;
        auto pb = pictureFrame(clear.view(), frame);
        check(pa && pb && coverage(*pa) > coverage(*pb),
              "frame inner ornament fading reduces coverage");
        Image hiddenClear{64, 48, std::vector<Rgba>(3072, {.2F, .3F, .4F, 0})};
        frame = {};
        frame.style = PictureFrameStyle::Dots;
        frame.flowerColor = {1, 0, 0, std::numeric_limits<float>::denorm_min()};
        auto faintFrame = pictureFrame(hiddenClear.view(), frame);
        bool cleanUnderflow = bool(faintFrame), roundedPaint = false;
        if (faintFrame)
            for (std::size_t i = 0; i < hiddenClear.pixels.size(); ++i) {
                auto q = faintFrame->pixels[i];
                if (q.a == 0) {
                    cleanUnderflow &= q == hiddenClear.pixels[i] || q == Rgba{0, 0, 0, 0};
                    roundedPaint |= q == Rgba{0, 0, 0, 0};
                }
            }
        check(cleanUnderflow && roundedPaint, "composite canonicalizes painted alpha underflow and "
                                              "preserves untouched hidden pixels");
    } else if (mode == "validation") {
        double nan = std::numeric_limits<double>::quiet_NaN();
        CloudsOptions co;
        co.foreground.r = 2;
        check(!clouds(flat.view(), co) && !differenceClouds(flat.view(), co),
              "cloud colors validated");
        FibersOptions fi;
        fi.strength = nan;
        check(!fibers(flat.view(), fi), "fiber NaN rejected");
        check(!lensFlare(flat.view(), {100, .5, .5, FlareLens(50)}), "flare enum validated");
        LightingEffectsOptions l;
        l.lights[0].color[2] = nan;
        check(!lightingEffects(flat.view(), l), "nested light fields validated");
        RelightOptions r;
        r.intensity = 0;
        r.warmth = nan;
        check(!relight(flat.view(), r), "relight inactive fields validated");
        FlameOptions f;
        f.paths = {{{1, nan}}};
        check(!flame(flat.view(), f), "flame paths validated");
        f = FlameOptions{};
        f.flameLines = 100;
        f.paths.assign(256, {{0, 0}, {1, 1}});
        check(!flame(flat.view(), f), "one-along-path aggregate primitive budget validated");
        f.type = FlameType::CandleLight;
        check(!flame(flat.view(), f), "candle aggregate primitive budget validated");
        TreeOptions t;
        t.baseTreeType = 35;
        check(!tree(flat.view(), t), "tree type validated");
        PictureFrameOptions p;
        p.style = PictureFrameStyle(40);
        check(!pictureFrame(flat.view(), p), "frame style validated");
        Image strip{1024, 1, std::vector<Rgba>(1024, {0, 0, 0, 0})};
        p = {};
        p.style = PictureFrameStyle::Snowflakes;
        p.margin = 0;
        p.size = 1;
        p.arrangement = 100;
        auto denseFrame = pictureFrame(strip.view(), p);
        check(!denseFrame && denseFrame.error().code == ErrorCode::InvalidParameter,
              "frame primitive budget rejects dense thin canvas before geometry generation");
        p.fade = 100;
        p.vineColor.a = p.flowerColor.a = p.leafColor.a = 0;
        auto fadedFrame = pictureFrame(strip.view(), p);
        check(!fadedFrame && fadedFrame.error().code == ErrorCode::InvalidParameter,
              "maximum frame fade still validates primitive work");
        p = {};
        p.style = PictureFrameStyle::Dots;
        p.margin = 0;
        p.size = 1;
        p.arrangement = 100;
        bounded(pictureFrame(strip.view(), p));
        Image longStrip{262144, 1, std::vector<Rgba>(262144, {0, 0, 0, 0})};
        p.lines = 5;
        for (int style = 0; style < 16; ++style) {
            p.style = PictureFrameStyle(style);
            auto rejected = pictureFrame(longStrip.view(), p);
            check(!rejected && rejected.error().code == ErrorCode::InvalidParameter,
                  "every frame recipe checks its primitive count before generation");
        }
        p.style = PictureFrameStyle::Dots;
        p.fade = 100;
        p.vineColor.a = p.flowerColor.a = p.leafColor.a = 0;
        auto transparentFrame = pictureFrame(longStrip.view(), p);
        check(!transparentFrame && transparentFrame.error().code == ErrorCode::InvalidParameter,
              "fully transparent frame paint still validates primitive work");
        std::vector<std::function<Result<Image>(ImageView)>> all = {[](ImageView i) {return clouds(i); },
                                                                    [](ImageView i) {return differenceClouds(i); },
                                                                    [](ImageView i) {return fibers(i); },
                                                                    [](ImageView i) {return lensFlare(i); },
                                                                    [](ImageView i) {return lightingEffects(i); },
                                                                    [](ImageView i) {return relight(i); },
                                                                    [](ImageView i) {return flame(i); },
                                                                    [](ImageView i) {return tree(i); },
                                                                    [](ImageView i) {return pictureFrame(i); }};
        auto altered = flat;
        for (std::size_t i = 0; i < flat.pixels.size(); i += 5) {
            flat.pixels[i] = {1, 0, 1, 0};
            altered.pixels[i] = {0, 1, 0, 0};
        }
        for (auto fn : all) {
            auto a = fn(flat.view()), b = fn(altered.view());
            bounded(a);
            bounded(b);
            if (a && b)
                for (std::size_t i = 0; i < a->pixels.size(); ++i)
                    if (a->pixels[i].a > 0 || b->pixels[i].a > 0)
                        check(a->pixels[i] == b->pixels[i], "rendering excludes hidden RGB");
            check(!fn({0, 1, {}}), "renderer image validation");
            Image thin{1, 1, {{.2F, .3F, .4F, 1}}};
            bounded(fn(thin.view()));
        }
    } else
        return 2;
    return failures ? 1 : 0;
}
