#include <cmath>
#include <functional>
#include <iiGraphicAdjust/PixelFilters.h>
#include <iostream>
#include <limits>
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
    check(bool(r), "filter succeeds");
    if (!r)
        return;
    for (auto p : r->pixels)
        check(std::isfinite(p.r) && p.r >= 0 && p.r <= 1 && std::isfinite(p.g) && p.g >= 0 &&
                  p.g <= 1 && std::isfinite(p.b) && p.b >= 0 && p.b <= 1 && p.a >= 0 && p.a <= 1,
              "finite normalized result");
}
Image pattern(std::size_t w, std::size_t h) {
    Image in{w, h, std::vector<Rgba>(w * h)};
    for (std::size_t i = 0; i < in.pixels.size(); ++i)
        in.pixels[i] = {float((i * 17 % 73) / 72.), float((i * 31 % 71) / 70.),
                        float((i * 13 % 67) / 66.), 1};
    return in;
}
std::vector<std::function<Result<Image>(ImageView)>> filters() {
    return {
        [](ImageView i) {return colorHalftone(i); }, [](ImageView i) {return crystallize(i); },
        [](ImageView i) {return facet(i); },         [](ImageView i) {return fragment(i); },
        [](ImageView i) {return mezzotint(i); },     [](ImageView i) {return pointillize(i); },
        [](ImageView i) {return mosaic(i); },        [](ImageView i) {return emboss(i); },
        [](ImageView i) {return findEdges(i); },     [](ImageView i) {return solarize(i); },
        [](ImageView i) {return diffuse(i); },       [](ImageView i) {return extrude(i); },
        [](ImageView i) {return oilPaint(i); },      [](ImageView i) {return tiles(i); },
        [](ImageView i) {return traceContour(i); },  [](ImageView i) {return wind(i); }};
}
} // namespace
int main(int argc, char **argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "pixelate") {
        Image in{2, 2, {{1, 0, 0, 1}, {0, 1, 0, .5F}, {0, 0, 1, 0}, {1, 1, 1, .5F}}};
        auto m = mosaic(in.view(), 2);
        bounded(m);
        if (m)
            for (auto p : m->pixels)
                check(near(p.r, .75) && near(p.g, .5) && near(p.b, .25) && near(p.a, .5),
                      "mosaic independently computed premultiplied cell mean");
        Image subnormal{2,
                        2,
                        {{1, 0, 0, std::numeric_limits<float>::denorm_min()},
                         {0, 1, 0, 0},
                         {0, 0, 1, 0},
                         {1, 1, 1, 0}}};
        auto rounded = mosaic(subnormal.view(), 2);
        check(rounded && rounded->pixels == std::vector<Rgba>(4, {0, 0, 0, 0}),
              "transported average canonicalizes alpha rounded to zero");
        auto p = pattern(9, 9);
        auto f = fragment(p.view());
        bounded(f);
        if (f) {
            auto got = f->pixels[40];
            auto a = p.pixels[36], b = p.pixels[44], c = p.pixels[4], d = p.pixels[76];
            check(near(got.r, (a.r + b.r + c.r + d.r) / 4.) &&
                      near(got.g, (a.g + b.g + c.g + d.g) / 4.),
                  "fragment four fixed offsets oracle");
        }
        auto cr = crystallize(p.view(), {4, 42});
        bounded(cr);
        if (cr) {
            double sr = 0, sg = 0, sb = 0, tr = 0, tg = 0, tb = 0;
            for (auto q : p.pixels) {
                sr += q.r;
                sg += q.g;
                sb += q.b;
            }
            for (auto q : cr->pixels) {
                tr += q.r;
                tg += q.g;
                tb += q.b;
            }
            check(near(sr, tr, 2e-5) && near(sg, tg, 2e-5) && near(sb, tb, 2e-5),
                  "Voronoi cell means conserve integrated RGB");
        }
        Image white{13, 7, std::vector<Rgba>(91, {1, 1, 1, 1})},
            black{13, 7, std::vector<Rgba>(91, {0, 0, 0, 1})};
        auto hw = colorHalftone(white.view()), hb = colorHalftone(black.view());
        bounded(hw);
        bounded(hb);
        if (hw && hb)
            for (std::size_t i = 0; i < 91; ++i)
                check(hw->pixels[i].r == 1 && hb->pixels[i].r <= .5,
                      "halftone zero ink white and full ink overlapping dark discs");
        for (int k = 0; k < 10; ++k) {
            MezzotintOptions o{MezzotintPattern(k), 42};
            auto a = mezzotint(p.view(), o), b = mezzotint(p.view(), o);
            bounded(a);
            check(a && b && a->pixels == b->pixels, "all mezzotint patterns deterministic");
            if (a)
                for (auto q : a->pixels)
                    check((q.r == 0 || q.r == 1) && (q.g == 0 || q.g == 1) &&
                              (q.b == 0 || q.b == 1),
                          "mezzotint channels binary");
        }
        auto a = pointillize(p.view(), {4, 42, {1, 1, 1, 1}}),
             b = pointillize(p.view(), {4, 42, {1, 1, 1, 1}}),
             c = pointillize(p.view(), {4, 43, {1, 1, 1, 1}});
        bounded(a);
        check(a && b && c && a->pixels == b->pixels && a->pixels != c->pixels,
              "pointillize seed controls dot layout");
    } else if (mode == "stylize") {
        Image flat{16, 12, std::vector<Rgba>(192, {.3F, .4F, .5F, .7F})};
        auto fac = facet(flat.view());
        check(fac && fac->pixels == flat.pixels, "facet constant identity and alpha preservation");
        auto em = emboss(flat.view());
        bounded(em);
        if (em)
            for (auto p : em->pixels)
                check(p.r == .5F && p.g == .5F && p.b == .5F && p.a == .7F,
                      "emboss constant midgray relief");
        Image line{5, 1, {{0, 0, 0, 1}, {0, 0, 0, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}};
        auto ed = findEdges(line.view());
        check(ed && ed->pixels[0].r == 1 && ed->pixels[1].r == 0 && ed->pixels[2].r == 0 &&
                  ed->pixels[4].r == 1,
              "Sobel step independently expected dark boundary");
        auto lo = traceContour(line.view(), {128, false}),
             hi = traceContour(line.view(), {128, true});
        check(lo && hi && lo->pixels[1].r == 0 && lo->pixels[2].r == 1 && hi->pixels[1].r == 1 &&
                  hi->pixels[2].r == 0,
              "contour traces chosen side of threshold");
        Image solar{1, 1, {{.5F, .75F, .25F, .5F}}};
        auto s = solarize(solar.view());
        check(s && s->pixels[0] == Rgba{.5F, .25F, .25F, .5F}, "solarize channel threshold oracle");
        auto p = pattern(20, 16);
        TilesOptions t;
        t.maximumOffset = 0;
        auto ts = tiles(p.view(), t);
        check(ts && ts->pixels == p.pixels, "zero tile displacement identity");
        OilPaintOptions o;
        o.lighting = false;
        auto paint = oilPaint(flat.view(), o);
        bounded(paint);
        if (paint)
            for (auto q : paint->pixels)
                check(near(q.r, .3) && near(q.g, .4) && near(q.b, .5) && q.a == .7F,
                      "LIC oil constant identity without lighting");
        Image faint{2, 1, {{1, 0, 0, 1e-20F}, {0, 1, 0, 1e-20F}}};
        auto faintPaint = oilPaint(faint.view(), o);
        check(faintPaint && faintPaint->pixels[0].g > .1 && faintPaint->pixels[1].r > .1 &&
                  faintPaint->pixels[0].a == 1e-20F && faintPaint->pixels[1].a == 1e-20F,
              "oil painting still smooths colors at representable tiny alpha");
        Image isolated{3, 3, std::vector<Rgba>(9, {0, 1, 0, 0})};
        isolated.pixels[4] = {1, 0, 0, std::numeric_limits<float>::denorm_min()};
        auto faintFacet = facet(isolated.view());
        auto faintDiffuse = diffuse(isolated.view(), {DiffuseMode::Anisotropic, 0});
        check(faintFacet && faintFacet->pixels[4] == isolated.pixels[4],
              "facet retains double color ratios when source alpha is preserved");
        check(faintDiffuse && faintDiffuse->pixels[4] == isolated.pixels[4],
              "anisotropic diffuse retains double color ratios when source alpha is preserved");
        o.lighting = true;
        o.shine = 10;
        auto oilA = oilPaint(p.view(), o);
        o.angle = 120;
        auto oilB = oilPaint(p.view(), o);
        check(oilA && oilB && oilA->pixels != oilB->pixels,
              "oil lighting direction modifies relief");
        for (int k = 0; k < 4; ++k) {
            auto a = diffuse(p.view(), {DiffuseMode(k), 31}),
                 b = diffuse(p.view(), {DiffuseMode(k), 31});
            bounded(a);
            check(a && b && a->pixels == b->pixels, "diffuse variants deterministic");
            if (a && (k == 1 || k == 2))
                for (std::size_t i = 0; i < p.pixels.size(); ++i) {
                    auto lum = [](Rgba q) { return .299 * q.r + .587 * q.g + .114 * q.b; };
                    check(k == 1 ? lum(a->pixels[i]) <= lum(p.pixels[i]) + 1e-7
                                 : lum(a->pixels[i]) >= lum(p.pixels[i]) - 1e-7,
                          "directional diffuse acceptance");
                }
        }
        for (int k = 0; k < 2; ++k) {
            ExtrudeOptions e;
            e.shape = ExtrudeShape(k);
            e.size = 5;
            e.depth = 100;
            auto a = extrude(p.view(), e);
            bounded(a);
            check(a && a->pixels != p.pixels, "block and pyramid extrusion execute");
        }
        for (int k = 0; k < 3; ++k) {
            WindOptions w{WindMethod(k), true, 31};
            auto a = wind(p.view(), w);
            w.fromRight = false;
            auto b = wind(p.view(), w);
            bounded(a);
            bounded(b);
            check(a && b && a->pixels != b->pixels, "all wind methods respect direction");
        }
    } else if (mode == "validation") {
        auto p = pattern(7, 5);
        auto hidden = p;
        for (std::size_t i = 0; i < p.pixels.size(); i += 3) {
            p.pixels[i] = {1, 0, 1, 0};
            hidden.pixels[i] = {0, 1, 0, 0};
        }
        for (auto f : filters()) {
            auto a = f(p.view()), b = f(hidden.view());
            bounded(a);
            bounded(b);
            if (a && b)
                for (std::size_t i = 0; i < p.pixels.size(); ++i)
                    if (a->pixels[i].a > 0 || b->pixels[i].a > 0)
                        check(a->pixels[i] == b->pixels[i],
                              "hidden RGB never enters spatial visible color");
            check(!f({0, 1, {}}), "all filters reject invalid dimensions");
        }
        for (auto mode : {DiffuseMode::DarkenOnly, DiffuseMode::LightenOnly}) {
            Image a{7, 5, std::vector<Rgba>(35, {0, 0, 0, 0})};
            a.pixels[17] = {.3F, .3F, .3F, 1};
            auto b = a;
            for (auto &q : b.pixels)
                if (q.a == 0)
                    q = {1, 1, 1, 0};
            auto da = diffuse(a.view(), {mode, 31}), db = diffuse(b.view(), {mode, 31});
            bool sameVisible = bool(da) && bool(db);
            if (da && db)
                for (std::size_t i = 0; i < a.pixels.size(); ++i)
                    if (da->pixels[i].a > 0 || db->pixels[i].a > 0)
                        sameVisible &= da->pixels[i] == db->pixels[i];
            check(sameVisible, "directional diffuse acceptance excludes hidden center RGB");
        }
        for (auto [w, h] :
             std::array<std::pair<std::size_t, std::size_t>, 3>{{{1, 1}, {1, 5}, {7, 1}}}) {
            auto in = pattern(w, h);
            for (auto f : filters())
                bounded(f(in.view()));
        }
        double nan = std::numeric_limits<double>::quiet_NaN();
        check(!colorHalftone(p.view(), {nan, {108, 162, 90}}), "halftone NaN rejected");
        check(!crystallize(p.view(), {2, 0}), "crystallize range");
        check(!mosaic(p.view(), 201), "mosaic range");
        check(!mezzotint(p.view(), {MezzotintPattern(100), 0}), "mezzotint enum");
        check(!pointillize(p.view(), {4, 0, {2, 0, 0, 1}}), "pointillize color");
        check(!emboss(p.view(), {0, 0, 100}), "emboss height");
        check(!diffuse(p.view(), {DiffuseMode(100), 0}), "diffuse enum");
        check(!extrude(p.view(), {ExtrudeShape(100), 30, 30}), "extrude enum");
        OilPaintOptions oil;
        oil.angle = nan;
        check(!oilPaint(p.view(), oil), "oil inactive fields validated");
        TilesOptions tilesO;
        tilesO.fill = TileFill(100);
        check(!tiles(p.view(), tilesO), "tile enum");
        check(!traceContour(p.view(), {256, false}), "contour range");
        check(!wind(p.view(), {WindMethod(100), true, 0}), "wind enum");
    } else
        return 2;
    return failures ? 1 : 0;
}
