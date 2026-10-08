#include <iiGraphicAdjust/DevelopColor.h>
#include <iiGraphicAdjust/DevelopEffects.h>
#include <iiGraphicAdjust/DevelopTone.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

using namespace iiGraphicAdjust;

namespace {
struct Checks final {
    int failures = 0;
    void require(bool ok, const char *reason) {
        if (!ok) {
            if (failures < 20)
                std::cerr << reason << '\n';
            ++failures;
        }
    }
};

// A fixed integer stream avoids differences in standard-library distributions.
struct Values final {
    std::uint64_t state = 0x20261008629e3938ULL;
    double unit() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return double((state * 0x2545f4914f6cdd1dULL) >> 11) / 9007199254740992.;
    }
    double between(double lo, double hi) {
        return lo + (hi - lo) * unit();
    }
};

double linear(double v) {
    return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
}
double encoded(double v) {
    v = std::clamp(v, 0., 1.);
    return v <= .0031308 ? 12.92 * v : 1.055 * std::pow(v, 1. / 2.4) - .055;
}
double chroma(Rgba p) {
    return std::max({p.r, p.g, p.b}) - std::min({p.r, p.g, p.b});
}
bool normalized(Rgba p) {
    for (auto v : {p.r, p.g, p.b, p.a})
        if (!std::isfinite(v) || v < 0 || v > 1)
            return false;
    return true;
}

void preserves(Checks &checks, const Result<Image> &result, const Image &source,
               const char *reason) {
    checks.require(bool(result), reason);
    if (!result)
        return;
    checks.require(result->width == source.width && result->height == source.height &&
                       result->pixels.size() == source.pixels.size(),
                   "development dimensions preserved");
    if (result->pixels.size() != source.pixels.size())
        return;
    for (std::size_t i = 0; i < source.pixels.size(); ++i) {
        checks.require(normalized(result->pixels[i]), "development output finite and normalized");
        checks.require(result->pixels[i].a == source.pixels[i].a, "development alpha unchanged");
        if (source.pixels[i].a == 0)
            checks.require(result->pixels[i] == source.pixels[i], "invisible source RGB preserved");
    }
}

Image fixture() {
    Values values;
    Image image{7, 5, {}};
    for (int i = 0; i < 35; ++i)
        image.pixels.push_back({float(values.unit()), float(values.unit()), float(values.unit()),
                                i % 7 == 0 ? 0.F : float(values.unit())});
    image.pixels[1] = {0, 0, 0, 1};
    image.pixels[2] = {1, 1, 1, 1};
    image.pixels[3] = {.5F, .5F, .5F, .5F};
    image.pixels[4] = {
        std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::denorm_min(),
        std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::denorm_min()};
    return image;
}

void oracle(Checks &checks) {
    const Image image{6,
                      1,
                      {{0, 0, 0, 1},
                       {.01F, .01F, .01F, 1},
                       {.1F, .1F, .1F, 1},
                       {.5F, .5F, .5F, .5F},
                       {.9F, .9F, .9F, 1},
                       {.2F, .8F, .1F, 0}}};
    for (double ev : {-5., -1., .5, 1., 5.}) {
        DevelopLightOptions options;
        options.exposure = ev;
        const auto result = developLight(image.view(), options);
        preserves(checks, result, image, "exposure oracle operation succeeds");
        if (result)
            for (std::size_t i = 0; i < 5; ++i)
                checks.require(std::abs(result->pixels[i].r -
                                        encoded(linear(image.pixels[i].r) * std::exp2(ev))) < 2e-6,
                               "neutral-axis exposure follows radiometric gain");
    }
    DevelopCurveOptions curve;
    curve.master.points = {{.25, .25}, {.75, .75}};
    const auto clipped = developCurve(image.view(), curve);
    if (clipped)
        for (std::size_t i = 0; i < 5; ++i)
            checks.require(std::abs(clipped->pixels[i].r -
                                    std::clamp(double(image.pixels[i].r), .25, .75)) < 2e-6,
                           "partial identity point curve clips at endpoints");
    else
        checks.require(false, "partial endpoint curve accepted");
    curve.master.points = {{0, .5}, {1, .5}};
    curve.refineSaturation = 0;
    const auto gray = developCurve(fixture().view(), curve);
    checks.require(bool(gray), "constant point curve accepted");
    if (gray)
        for (auto p : gray->pixels)
            if (p.a > 0)
                checks.require(p.r == .5F && p.g == .5F && p.b == .5F,
                               "Refine Saturation does not amplify neutral-axis rounding");

    Image hueRamp{3600, 1, {}};
    for (int i = 0; i < 3600; ++i) {
        const double hp = i / 600., x = 1 - std::abs(std::fmod(hp, 2) - 1);
        Rgba p{0, 0, 0, 1};
        switch (int(hp) % 6) {
        case 0:
            p.r = 1;
            p.g = float(x);
            break;
        case 1:
            p.r = float(x);
            p.g = 1;
            break;
        case 2:
            p.g = 1;
            p.b = float(x);
            break;
        case 3:
            p.g = float(x);
            p.b = 1;
            break;
        case 4:
            p.r = float(x);
            p.b = 1;
            break;
        default:
            p.r = 1;
            p.b = float(x);
        }
        hueRamp.pixels.push_back(p);
    }
    DevelopColorOptions bandOptions;
    for (auto &band : bandOptions.mixer)
        band.saturation = -50;
    DevelopColorOptions globalOptions;
    globalOptions.saturation = -50;
    const auto band = developColor(hueRamp.view(), bandOptions),
               global = developColor(hueRamp.view(), globalOptions);
    checks.require(band && global, "partition oracle operations succeed");
    if (band && global)
        for (std::size_t i = 0; i < hueRamp.pixels.size(); ++i) {
            const auto a = band->pixels[i], b = global->pixels[i];
            checks.require(std::abs(a.r - b.r) < 2e-6 && std::abs(a.g - b.g) < 2e-6 &&
                               std::abs(a.b - b.b) < 2e-6,
                           "eight color-band weights form partition of unity");
        }
    const auto pointSample = samplePointColor(hueRamp.pixels[0]);
    DevelopColorOptions bw;
    bw.blackAndWhite = true;
    const auto plainBw = developColor(hueRamp.view(), bw);
    if (pointSample) {
        bw.points.push_back(*pointSample);
        bw.points[0].luminanceShift = 100;
        bw.points[0].variance = 100;
        const auto pointBw = developColor(hueRamp.view(), bw);
        checks.require(plainBw && pointBw && plainBw->pixels == pointBw->pixels,
                       "Point Color disabled in black-and-white treatment");
    }
    const Image neutral{1, 1, {{.5F, .5F, .5F, 1}}};
    // Keep synthesized casts away from the inverse estimator's Kelvin boundaries.
    for (double temperature : {5000., 6500., 8000.})
        for (double tint : {-30., 0., 30.}) {
            const auto cast = whiteBalance(neutral.view(), {temperature, tint});
            if (!cast) {
                checks.require(false, "WB cast generation succeeds");
                continue;
            }
            const auto estimate = sampledWhiteBalance(cast->pixels[0]);
            checks.require(bool(estimate), "eyedropper returns valid inverse settings");
            if (estimate) {
                const auto corrected = whiteBalance(cast->view(), *estimate);
                const bool ok = corrected && chroma(corrected->pixels[0]) < .003;
                if (!ok)
                    std::cerr << "WB roundtrip temperature=" << temperature << " tint=" << tint
                              << " estimated=" << estimate->temperature << ',' << estimate->tint
                              << " residual=" << (corrected ? chroma(corrected->pixels[0]) : -1)
                              << '\n';
                checks.require(ok, "WB eyedropper corrects recoverable temperature/tint cast");
            }
        }
}

void invariants(Checks &checks) {
    const auto image = fixture();
    Values values;
    preserves(checks, whiteBalance(image.view()), image, "neutral WB");
    preserves(checks, developLight(image.view()), image, "neutral Light");
    preserves(checks, developCurve(image.view()), image, "neutral Curve");
    preserves(checks, developColor(image.view()), image, "neutral Color");
    preserves(checks, calibrate(image.view()), image, "neutral Calibration");
    preserves(checks, developEffects(image.view()), image, "neutral Effects");
    preserves(checks, developDetail(image.view()), image, "neutral Detail");
    preserves(checks, postCropVignette(image.view()), image, "neutral Vignette");
    preserves(checks, filmGrain(image.view()), image, "neutral Grain");
    for (const auto &neutral : std::array{
             whiteBalance(image.view()), developLight(image.view()), developCurve(image.view()),
             developColor(image.view()), calibrate(image.view()), developEffects(image.view()),
             developDetail(image.view()), postCropVignette(image.view()), filmGrain(image.view())})
        checks.require(neutral && neutral->pixels == image.pixels,
                       "neutral development operation is exact copy");
    for (int iteration = 0; iteration < 256; ++iteration) {
        const auto signedValue = [&]() { return values.between(-100, 100); };
        WhiteBalanceOptions wb{values.between(2000, 50000), values.between(-150, 150),
                               values.between(2000, 50000), values.between(-150, 150)};
        preserves(checks, whiteBalance(image.view(), wb), image, "random WB");
        DevelopLightOptions light{values.between(-5, 5), signedValue(), signedValue(),
                                  signedValue(),         signedValue(), signedValue()};
        preserves(checks, developLight(image.view(), light), image, "random Light");
        DevelopCurveOptions curve;
        curve.shadows = signedValue();
        curve.darks = signedValue();
        curve.lights = signedValue();
        curve.highlights = signedValue();
        curve.refineSaturation = values.between(0, 100);
        curve.master.points = {{0, values.unit()},
                               {.25, values.unit()},
                               {.5, values.unit()},
                               {.75, values.unit()},
                               {1, values.unit()}};
        preserves(checks, developCurve(image.view(), curve), image, "random Curve");
        DevelopColorOptions color;
        color.vibrance = signedValue();
        color.saturation = signedValue();
        color.blackAndWhite = iteration % 3 == 0;
        for (auto &b : color.mixer)
            b = {signedValue(), signedValue(), signedValue()};
        for (auto &b : color.blackAndWhiteMix)
            b = signedValue();
        for (auto *w : {&color.grading.shadows, &color.grading.midtones, &color.grading.highlights,
                        &color.grading.global})
            *w = {values.between(0, 360), values.between(0, 100), signedValue()};
        color.grading.blending = values.between(0, 100);
        color.grading.balance = signedValue();
        for (int j = 0; j < iteration % 9; ++j) {
            PointColorOptions p;
            p.lightness = values.unit();
            p.chroma = values.between(0, .5);
            p.hue = values.between(-180, 360);
            p.hueShift = signedValue();
            p.saturationShift = signedValue();
            p.luminanceShift = signedValue();
            p.variance = signedValue();
            p.range = values.between(0, 100);
            p.hueRange = values.between(0, 100);
            p.saturationRange = values.between(0, 100);
            p.luminanceRange = values.between(0, 100);
            color.points.push_back(p);
        }
        preserves(checks, developColor(image.view(), color), image, "random Color");
        CalibrationOptions calibration;
        calibration.shadowsTint = signedValue();
        for (int j = 0; j < 3; ++j) {
            calibration.primaryHue[j] = signedValue();
            calibration.primarySaturation[j] = signedValue();
        }
        const auto calibrated = calibrate(image.view(), calibration);
        if (calibrated)
            preserves(checks, calibrated, image, "random Calibration");
        else
            checks.require(calibrated.error().code == ErrorCode::InvalidParameter,
                           "degenerate calibration returns structured error");
        preserves(checks,
                  developEffects(image.view(), {signedValue(), signedValue(), signedValue()}),
                  image, "random Effects");
        DevelopDetailOptions detail{values.between(0, 150), values.between(0, 100),
                                    values.between(0, 100), values.between(0, 100),
                                    values.between(0, 100), values.between(0, 100),
                                    values.between(0, 100)};
        preserves(checks, developDetail(image.view(), detail), image, "random Detail");
        PostCropVignetteOptions vignette{
            signedValue(),          values.between(0, 100),
            signedValue(),          values.between(0, 100),
            values.between(0, 100), static_cast<VignetteStyle>(iteration % 3)};
        preserves(checks, postCropVignette(image.view(), vignette), image, "random Vignette");
        preserves(checks,
                  filmGrain(image.view(),
                            {values.between(0, 100), values.between(0, 100), values.between(0, 100),
                             static_cast<std::uint32_t>(iteration)}),
                  image, "random Grain");
    }
    auto changedHidden = image;
    for (auto &p : changedHidden.pixels)
        if (p.a == 0)
            p = {1 - p.r, 1 - p.g, 1 - p.b, 0};
    const auto isolated = [&](const Result<Image> &a, const Result<Image> &b) {
        checks.require(a && b, "alpha isolation operations succeed");
        if (a && b)
            for (std::size_t i = 0; i < image.pixels.size(); ++i)
                if (image.pixels[i].a > 0)
                    checks.require(a->pixels[i] == b->pixels[i],
                                   "hidden RGB cannot affect visible spatial processing");
    };
    isolated(developLight(image.view(), {0, 0, -70, 70}),
             developLight(changedHidden.view(), {0, 0, -70, 70}));
    isolated(developEffects(image.view(), {70, 70, 70}),
             developEffects(changedHidden.view(), {70, 70, 70}));
    isolated(developDetail(image.view(), {100, 40, 70, 50, 70, 50, 50}),
             developDetail(changedHidden.view(), {100, 40, 70, 50, 70, 50, 50}));
    const auto autoWb = autoWhiteBalance(image.view()),
               otherWb = autoWhiteBalance(changedHidden.view());
    checks.require(autoWb && otherWb && autoWb->temperature == otherWb->temperature &&
                       autoWb->tint == otherWb->tint,
                   "Auto WB statistics ignore hidden RGB");
    const auto autoBw = autoBlackAndWhiteMix(image.view()),
               otherBw = autoBlackAndWhiteMix(changedHidden.view());
    checks.require(autoBw && otherBw && *autoBw == *otherBw,
                   "Auto B&W statistics ignore hidden RGB");
    const auto autoLight = autoDevelopLight(image.view()),
               otherLight = autoDevelopLight(changedHidden.view());
    checks.require(autoLight && otherLight && autoLight->exposure == otherLight->exposure &&
                       autoLight->contrast == otherLight->contrast &&
                       autoLight->highlights == otherLight->highlights &&
                       autoLight->shadows == otherLight->shadows &&
                       autoLight->whites == otherLight->whites &&
                       autoLight->blacks == otherLight->blacks,
                   "Auto Light statistics ignore hidden RGB");
    for (auto width : {std::size_t{1}, std::size_t{64}}) {
        Image thin{width, 1, std::vector<Rgba>(width, {.4F, .4F, .4F, 1})};
        const auto detail = developDetail(thin.view(), {150, 0, 100, 0, 100, 0, 100});
        preserves(checks, detail, thin, "thin image spatial processing");
        if (detail)
            for (auto p : detail->pixels)
                checks.require(std::abs(p.r - .4) < 2e-6 && chroma(p) < 2e-6,
                               "constant image remains constant through spatial detail");
    }
    Image gray{65, 1, {}};
    for (int i = 0; i <= 64; ++i) {
        float v = float(i) / 64;
        gray.pixels.push_back({v, v, v, 1});
    }
    DevelopColorOptions color;
    color.vibrance = 100;
    for (auto &band : color.mixer)
        band = {100, 100, 100};
    const auto neutral = developColor(gray.view(), color);
    if (neutral)
        for (auto p : neutral->pixels)
            checks.require(chroma(p) < 2e-6, "gray axis unaffected by mixer and vibrance");
    const auto point = samplePointColor({.5F, .5F, .5F, 1});
    if (point) {
        color = {};
        color.points.push_back(*point);
        color.points[0].variance = -100;
        color.points[0].saturationShift = 100;
        const auto selected = developColor(gray.view(), color);
        if (selected)
            for (auto p : selected->pixels)
                checks.require(chroma(p) < 2e-6,
                               "sampled gray Point Color does not amplify rounding");
    }
}

void validation(Checks &checks) {
    const auto image = fixture();
    for (double bad :
         {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
          -std::numeric_limits<double>::infinity(), 1e10, -1e10}) {
        WhiteBalanceOptions wb;
        wb.temperature = bad;
        checks.require(!whiteBalance(image.view(), wb), "WB invalid temperature rejected");
        wb = {};
        wb.asShotTint = bad;
        checks.require(!autoWhiteBalance(image.view(), wb), "Auto WB invalid source rejected");
        DevelopLightOptions light;
        light.shadows = bad;
        checks.require(!developLight(image.view(), light), "Light invalid slider rejected");
        DevelopCurveOptions curve;
        curve.splitMid = bad;
        checks.require(!developCurve(image.view(), curve), "Curve invalid split rejected");
        curve = {};
        curve.red.points[1].output = bad;
        checks.require(!developCurve(image.view(), curve), "Curve invalid point rejected");
        DevelopColorOptions color;
        color.grading.global.hue = bad;
        checks.require(!developColor(image.view(), color), "Color invalid wheel rejected");
        color = {};
        color.points.resize(1);
        color.points[0].variance = bad;
        checks.require(!developColor(image.view(), color), "Point Color invalid variance rejected");
        CalibrationOptions calibration;
        calibration.primaryHue[1] = bad;
        checks.require(!calibrate(image.view(), calibration),
                       "Calibration invalid primary rejected");
        checks.require(!developEffects(image.view(), {0, 0, bad}),
                       "Effects invalid dehaze rejected");
        DevelopDetailOptions detail;
        detail.colorSmoothness = bad;
        checks.require(!developDetail(image.view(), detail), "Detail invalid smoothness rejected");
        PostCropVignetteOptions vignette;
        vignette.feather = bad;
        checks.require(!postCropVignette(image.view(), vignette),
                       "Vignette invalid feather rejected");
        checks.require(!filmGrain(image.view(), {0, 0, bad}), "Grain invalid roughness rejected");
    }
    Image bad = image;
    bad.pixels[0].r = 2;
    checks.require(
        !whiteBalance(bad.view()) && !developLight(bad.view()) && !developCurve(bad.view()) &&
            !developColor(bad.view()) && !calibrate(bad.view()) && !developEffects(bad.view()) &&
            !developDetail(bad.view()) && !postCropVignette(bad.view()) && !filmGrain(bad.view()),
        "rendered-image APIs reject HDR channels");
    const Image empty;
    checks.require(!autoDevelopLight(empty.view()) && !autoBlackAndWhiteMix(empty.view()),
                   "empty estimates rejected");
    Image invisible{1, 1, {{.5F, .5F, .5F, 0}}};
    checks.require(!autoDevelopLight(invisible.view()) && !autoBlackAndWhiteMix(invisible.view()) &&
                       !samplePointColor(invisible.pixels[0]) &&
                       !sampledWhiteBalance(invisible.pixels[0]),
                   "invisible statistical samples rejected");
    DevelopColorOptions color;
    color.points.resize(9);
    checks.require(!developColor(image.view(), color), "Point Color sample limit rejected");
    PostCropVignetteOptions vignette;
    vignette.style = static_cast<VignetteStyle>(99);
    checks.require(!postCropVignette(image.view(), vignette), "unknown vignette style rejected");
}
} // namespace

int main(int argc, char **argv) {
    Checks checks;
    const std::string_view name = argc > 1 ? argv[1] : "all";
    if (name == "all" || name == "oracle")
        oracle(checks);
    if (name == "all" || name == "invariants")
        invariants(checks);
    if (name == "all" || name == "validation")
        validation(checks);
    if (name != "all" && name != "oracle" && name != "invariants" && name != "validation")
        return 2;
    std::cout << "Develop " << name << " failures=" << checks.failures << '\n';
    return checks.failures ? 1 : 0;
}
