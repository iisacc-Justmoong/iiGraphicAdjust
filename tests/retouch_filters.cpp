#include <algorithm>
#include <cmath>
#include <functional>
#include <iiGraphicAdjust/RetouchFilters.h>
#include <iostream>
#include <limits>
#include <string>

using namespace iiGraphicAdjust;
namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}
bool near(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) <= tolerance;
}
double linear(double v) {
    return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
}
double encoded(double v) {
    return v <= .0031308 ? v * 12.92 : 1.055 * std::pow(v, 1 / 2.4) - .055;
}
Image image(std::size_t w, std::size_t h, Rgba p) {
    return {w, h, std::vector<Rgba>(w * h, p)};
}
void bounded(const Result<Image> &result, ImageView original) {
    check(bool(result), "valid retouch request must succeed");
    if (!result)
        return;
    check(result->width == original.width && result->height == original.height,
          "retouch preserves dimensions");
    for (std::size_t i = 0; i < result->pixels.size(); ++i) {
        auto p = result->pixels[i];
        check(std::isfinite(p.r) && std::isfinite(p.g) && std::isfinite(p.b) && p.r >= 0 &&
                  p.r <= 1 && p.g >= 0 && p.g <= 1 && p.b >= 0 && p.b <= 1,
              "retouch output is finite SDR");
        check(p.a == original.pixels[i].a, "retouch preserves original coverage");
    }
}
} // namespace
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    const std::string mode = argv[1];
    if (mode == "spots") {
        auto in = image(100, 100, {.1F, .1F, .1F, 1});
        for (std::size_t y = 0; y < 100; ++y)
            for (std::size_t x = 50; x < 100; ++x)
                in.pixels[y * 100 + x] = {.9F, .9F, .9F, 1};
        SpotOptions s;
        s.points = {{.25, .5}};
        s.size = .05;
        s.feather = 0;
        s.sourceOffset = {{.5, 0}};
        auto cloned = cloneSpot(in.view(), s);
        bounded(cloned, in.view());
        check(cloned && near(cloned->pixels[50 * 100 + 25].r, .9) &&
                  cloned->pixels[50 * 100 + 5] == in.pixels[50 * 100 + 5],
              "clone copies shifted source at pixel centres and leaves exterior exact");
        auto healed = healSpot(in.view(), s), removed = removeSpot(in.view(), s);
        check(healed && near(healed->pixels[50 * 100 + 25].r, .1) && removed &&
                  healed->pixels == removed->pixels,
              "heal normalized surrounding difference preserves target tone; fixed remove equals "
              "heal");
        s.opacity = 50;
        auto half = cloneSpot(in.view(), s);
        double expected = encoded((linear(double(.1F)) + linear(double(.9F))) / 2);
        check(half && near(half->pixels[50 * 100 + 25].r, expected),
              "clone opacity blends in linear light, with an independent numerical oracle");
        for (std::size_t y = 0; y < 100; ++y)
            for (std::size_t x = 50; x < 100; ++x)
                in.pixels[y * 100 + x].a = .5F;
        auto semi = cloneSpot(in.view(), s);
        expected = encoded(linear(double(.1F)) * .75 + linear(double(.9F)) * .25);
        check(semi && near(semi->pixels[50 * 100 + 25].r, expected),
              "source coverage weights the linear clone blend");
        s.opacity = 0;
        auto zero = healSpot(in.view(), s);
        check(zero && zero->pixels == in.pixels, "zero spot opacity exact identity");
        s.points.clear();
        auto empty = cloneSpot(in.view(), s);
        check(empty && empty->pixels == in.pixels, "empty stroke exact identity");
        auto gradient = image(120, 80, {});
        for (std::size_t y = 0; y < 80; ++y)
            for (std::size_t x = 0; x < 120; ++x) {
                float v = .2F + x * .001F;
                gradient.pixels[y * 120 + x] = {v, v, v, 1};
            }
        for (int y = 36; y < 44; ++y)
            for (int x = 56; x < 64; ++x)
                gradient.pixels[y * 120 + x] = {0, 0, 0, 1};
        s = {};
        s.size = 8. / 120;
        s.feather = 30;
        auto clean = removeSpot(gradient.view(), s);
        bounded(clean, gradient.view());
        check(clean && near(clean->pixels[40 * 120 + 60].r, .26, .02),
              "automatic ring-matched heal removes a dark dot from a gentle gradient");
        auto uniform = image(100, 100, {.4F, .4F, .4F, 1});
        auto sources = rankSpotSources(uniform.view());
        check(sources && sources->size() == 48, "all 48 reference ring candidates enumerated");
        if (sources && !sources->empty()) {
            SpotSourceOptions options;
            options.avoidOffset = sources->front().offset;
            auto refreshed = rankSpotSources(uniform.view(), options);
            check(refreshed && refreshed->size() < sources->size(),
                  "refresh omits nearby source offsets");
            if (refreshed)
                for (auto q : *refreshed)
                    check(std::hypot((q.offset[0] - (*options.avoidOffset)[0]) * 100,
                                     (q.offset[1] - (*options.avoidOffset)[1]) * 100) > 2,
                          "refreshed source is more than one spot radius from previous offset");
        }
    } else if (mode == "eyes") {
        auto red = image(100, 100, {.9F, .15F, .15F, 1});
        EyeOptions e;
        e.center = {.505, .505};
        e.radiusX = e.radiusY = .1;
        auto detected = detectPupil(red.view(), e);
        check(detected && detected->found && near(detected->center[0], .505, 1e-12) &&
                  near(detected->radius, .1, 1e-12),
              "fully red ellipse has exact grid centroid and area-derived pupil radius");
        auto out = redEye(red.view(), e);
        bounded(out, red.view());
        double r = linear(double(.9F)), g = linear(double(.15F));
        double green = .069097289358232 * r + (.919540395075459 + .011362315566309) * g;
        double blue = .01639143887515 * r + (.088013307877226 + .895595253247624) * g;
        double expected = encoded(.5 * (green + blue) * .55);
        check(out && near(out->pixels[50 * 100 + 50].r, expected, 2e-6) &&
                  near(out->pixels[50 * 100 + 50].g, expected, 2e-6) &&
                  near(out->pixels[50 * 100 + 50].b, expected, 2e-6) &&
                  out->pixels[0] == red.pixels[0],
              "red gate neutralizes and darkens in Rec2020 linear light; exterior remains exact");
        auto grey = image(100, 100, {.5F, .5F, .5F, .4F});
        auto noRed = redEye(grey.view(), e);
        check(noRed && noRed->pixels == grey.pixels, "red gate preserves a non-red iris exactly");
        auto noPupil = detectPupil(grey.view(), e, EyeKind::Pet);
        check(noPupil && !noPupil->found && near(noPupil->radius, .05),
              "uniform pet eye uses documented half-ellipse fallback");
        auto pet = petEye(grey.view(), e);
        expected = encoded(.04 * (1 - .85 * .5));
        bounded(pet, grey.view());
        check(pet && near(pet->pixels[50 * 100 + 50].r, expected),
              "pet fallback disc applies the independent .04 luminance ceiling and darken oracle");
        e.catchlight = {{0, 0}};
        auto light = petEye(grey.view(), e);
        check(light && near(light->pixels[50 * 100 + 50].r, encoded(.9)),
              "pet catchlight centre is .9 in scene-linear light");
        auto pupilImage = image(400, 200, {.3F, .3F, .3F, 1});
        for (std::size_t y = 0; y < 200; ++y)
            for (std::size_t x = 0; x < 400; ++x)
                if (std::hypot(double(x) - 210, double(y) - 96) <= 14)
                    pupilImage.pixels[y * 400 + x] = {.75F, .9F, .55F, 1};
        e.center = {.52, .5};
        e.radiusX = .1;
        e.radiusY = .09;
        e.catchlight.reset();
        auto found = detectPupil(pupilImage.view(), e, EyeKind::Pet);
        check(found && found->found && near(found->center[0] * 400, 210, 2) &&
                  near(found->center[1] * 200, 96, 2) && near(found->radius * 400, 14, 3),
              "brightness-weighted grid finds a shifted green pet-eye glow and estimates its area");
        auto fixed = petEye(pupilImage.view(), e);
        check(fixed && fixed->pixels[96 * 400 + 210].g < .2 &&
                  fixed->pixels[96 * 400 + 10] == pupilImage.pixels[96 * 400 + 10],
              "pet-eye glow becomes a neutral dark pupil, with surrounding photo preserved");
    } else if (mode == "dust") {
        auto clean = image(256, 128, {.6F, .6F, .6F, 1});
        auto spots = detectDustSpots(clean.view());
        auto view = visualizeSpots(clean.view());
        check(spots && spots->empty(), "constant field has no dust");
        check(view && std::all_of(view->pixels.begin(), view->pixels.end(),
                                  [](auto p) { return p.r == 0 && p.g == 0 && p.b == 0; }),
              "constant field has zero thresholded high pass");
        auto speck = clean;
        speck.pixels[64 * 256 + 128] = {.35F, .35F, .35F, 1};
        auto coarse = visualizeSpots(speck.view(), {0}), fine = visualizeSpots(speck.view(), {100});
        check(coarse && fine && fine->pixels[64 * 256 + 128].r == 1,
              "high-pass marks the dark speck");
        auto whiteness = [](const Image &i) {
            return std::count_if(i.pixels.begin(), i.pixels.end(), [](auto p) { return p.r == 1; });
        };
        check(coarse && fine && whiteness(*fine) >= whiteness(*coarse),
              "increased spot sensitivity monotonically lowers the threshold");
        auto sky = image(600, 400, {});
        const std::array<std::array<double, 3>, 3> targets{
            {{120, 90, 4}, {400, 200, 6}, {500, 330, 3}}};
        for (std::size_t y = 0; y < 400; ++y)
            for (std::size_t x = 0; x < 600; ++x) {
                double v = .55 + .25 * y / 400 + .05 * x / 600;
                for (auto target : targets)
                    v -= .06 * std::exp(-(std::pow(double(x) - target[0], 2) +
                                          std::pow(double(y) - target[1], 2)) /
                                        (2 * target[2] * target[2]));
                sky.pixels[y * 600 + x] = {float(v), float(v), float(v * 1.1), 1};
            }
        auto found = detectDustSpots(sky.view());
        check(found && found->size() == 3, "three soft round sensor-dust spots are all found");
        if (found) {
            for (auto target : targets)
                check(std::any_of(found->begin(), found->end(),
                                  [&](auto q) {
                                      return near(q.center[0] * 600, target[0], 3) &&
                                             near(q.center[1] * 400, target[1], 3);
                                  }),
                      "dust coordinates agree with independent planted centres");
            check(std::is_sorted(found->begin(), found->end(),
                                 [](auto a, auto b) { return a.strength > b.strength; }),
                  "dust detections sorted by descending darkening strength");
        }
        auto limited = detectDustSpots(sky.view(), {50, 1});
        check(limited && limited->size() == 1 && found && limited->front() == found->front(),
              "dust result cap retains strongest detection");
        auto small = detectDustSpots(image(31, 40, {.5F, .5F, .5F, 1}).view());
        check(small && small->empty(),
              "reference dust detector excludes images below 32 on either axis");
    } else if (mode == "validation") {
        auto in = image(48, 32, {.5F, .4F, .3F, 1});
        const double nan = std::numeric_limits<double>::quiet_NaN();
        SpotOptions s;
        s.opacity = 0;
        s.sourceOffset = {{nan, 0}};
        check(!cloneSpot(in.view(), s), "inactive source NaN rejected");
        s = {};
        s.feather = 101;
        check(!healSpot(in.view(), s), "feather range validated");
        s = {};
        s.points = {{1.1, .5}};
        check(!removeSpot(in.view(), s), "stroke coordinate range validated");
        s = {};
        s.points.assign(4097, {.5, .5});
        check(!cloneSpot(in.view(), s), "stroke point count bounded");
        auto medium = image(256, 256, {.5F, .5F, .5F, 1});
        s.points.assign(4096, {.5, .5});
        s.size = .25;
        check(!healSpot(medium.view(), s), "brush distance work estimate bounded");
        SpotSourceOptions search;
        search.avoidOffset = {{0, nan}};
        check(!rankSpotSources(in.view(), search), "source-refresh offset validated");
        EyeOptions eye;
        eye.catchlight = {{nan, 0}};
        check(!redEye(in.view(), eye) && !petEye(in.view(), eye),
              "nested catchlight finite for both eye kinds");
        eye = {};
        check(!detectPupil(in.view(), eye, EyeKind(100)), "eye kind enum validated");
        check(!visualizeSpots(in.view(), {nan}) && !detectDustSpots(in.view(), {101, 60}) &&
                  !detectDustSpots(in.view(), {50, 0}),
              "diagnostic and dust option ranges validated");
        std::vector<std::function<Result<Image>(ImageView)>> functions = {[](ImageView i) { return cloneSpot(i); },
                                                                          [](ImageView i) { return healSpot(i); },
                                                                          [](ImageView i) { return removeSpot(i); },
                                                                          [](ImageView i) { return redEye(i); },
                                                                          [](ImageView i) { return petEye(i); },
                                                                          [](ImageView i) { return visualizeSpots(i); }};
        auto altered = in;
        for (std::size_t i = 0; i < in.pixels.size(); i += 3) {
            in.pixels[i] = {1, 0, 1, 0};
            altered.pixels[i] = {0, 1, 0, 0};
        }
        for (auto fn : functions) {
            auto a = fn(in.view()), b = fn(altered.view());
            bounded(a, in.view());
            bounded(b, altered.view());
            if (a && b)
                for (std::size_t i = 0; i < in.pixels.size(); ++i)
                    if (in.pixels[i].a > 0)
                        check(a->pixels[i] == b->pixels[i],
                              "hidden RGB cannot affect visible retouch result");
            check(!fn({0, 1, {}}), "invalid image dimensions rejected");
            check(!fn({2, 2, in.pixels}), "invalid buffer length rejected");
            auto invalid = image(1, 1, {float(nan), 0, 0, 1});
            check(!fn(invalid.view()), "invalid input pixel rejected");
            for (auto extent : {std::array<std::size_t, 2>{1, 1}, {1, 8}, {8, 1}}) {
                auto thin = image(extent[0], extent[1], {.2F, .4F, .6F, .5F});
                bounded(fn(thin.view()), thin.view());
            }
        }
        auto sa = rankSpotSources(in.view()), sb = rankSpotSources(altered.view());
        check(sa && sb && *sa == *sb, "hidden RGB cannot affect source search");
        auto pa = detectPupil(in.view()), pb = detectPupil(altered.view());
        check(pa && pb && *pa == *pb, "hidden RGB cannot affect pupil detection");
        auto da = detectDustSpots(in.view()), db = detectDustSpots(altered.view());
        check(da && db && *da == *db, "hidden RGB cannot affect dust detection");
    } else
        return 2;
    return failures ? 1 : 0;
}
