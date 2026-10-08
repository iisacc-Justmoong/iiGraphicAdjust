#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iiGraphicAdjust/MeshFilters.h>
#include <iostream>
#include <limits>
#include <string_view>
using namespace iiGraphicAdjust;
namespace {
std::size_t checks = 0;
void require(bool c, const char *message) {
    ++checks;
    if (!c) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
Image image(std::size_t w = 64, std::size_t h = 48) {
    Image out{w, h, {}};
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x)
            out.pixels.push_back({float(x) / w, float(y) / h, float((x * 7 + y * 3) % 23) / 22,
                                  ((x + y * w) % 17 == 0) ? 0.f : 1.f});
    return out;
}
void contract(const Image &out, const Image &in) {
    require(out.width == in.width && out.height == in.height &&
                out.pixels.size() == in.pixels.size(),
            "Mesh output dimensions changed");
    for (auto p : out.pixels)
        require(std::isfinite(p.r) && p.r >= 0 && p.r <= 1 && std::isfinite(p.g) && p.g >= 0 &&
                    p.g <= 1 && std::isfinite(p.b) && p.b >= 0 && p.b <= 1 && std::isfinite(p.a) &&
                    p.a >= 0 && p.a <= 1,
                "Mesh output invalid RGBA");
}
void geometry(const MeshGeometry &mesh) {
    require(mesh.source.size() == mesh.destination.size(), "Geometry counts differ");
    for (auto p : mesh.source)
        require(std::isfinite(p[0]) && std::isfinite(p[1]), "Nonfinite source vertex");
    for (auto p : mesh.destination)
        require(std::isfinite(p[0]) && std::isfinite(p[1]), "Nonfinite destination vertex");
    for (const auto &curve : mesh.constraintCurves)
        for (auto p : curve)
            require(std::isfinite(p[0]) && std::isfinite(p[1]), "Nonfinite constraint point");
    for (auto t : mesh.triangles)
        for (auto v : t)
            require(v < mesh.source.size(), "Invalid triangle index");
    require(std::isfinite(mesh.constraintResidual) && mesh.constraintResidual >= 0,
            "Invalid residual");
}
template <class F, class O> void invalid(F fn, O o) {
    const auto in = image();
    const auto out = fn(in.view(), o);
    require(!out && out.error().code == ErrorCode::InvalidParameter,
            "Invalid mesh option accepted");
}
template <class F, class O> void consistency(F fn, const O &o) {
    const auto in = image();
    const auto a = fn(in.view(), o);
    require(bool(a), "Mesh operation failed");
    contract(*a, in);
    const auto b = fn(in.view(), o);
    require(bool(b) && a->pixels == b->pixels, "Mesh operation nondeterministic");
    auto hidden = in;
    for (auto &p : hidden.pixels)
        if (p.a == 0) {
            p.r = 1 - p.r;
            p.g = 1 - p.g;
            p.b = 1 - p.b;
        }
    const auto alternate = fn(hidden.view(), o);
    require(bool(alternate), "Alternate mesh failed");
    for (std::size_t k = 0; k < a->pixels.size(); ++k)
        if (a->pixels[k].a > 0)
            require(a->pixels[k] == alternate->pixels[k],
                    "Hidden RGB leaked through geometry sampling");
}
void liquifyTests() {
    const auto in = image();
    require(liquify(in.view())->pixels == in.pixels, "Default liquify not identity");
    LiquifyStroke warp;
    warp.size = 30;
    warp.points = {{{20, 24}, 1}, {{35, 24}, 1}};
    LiquifyOptions o;
    o.cell = 1;
    o.strokes = {warp};
    const auto moved = liquify(in.view(), o);
    require(bool(moved) && moved->pixels != in.pixels, "Forward Warp changed nothing");
    consistency(liquify, o);
    for (auto tool : {LiquifyTool::TwirlClockwise, LiquifyTool::TwirlCounterclockwise,
                      LiquifyTool::Pucker, LiquifyTool::Bloat, LiquifyTool::PushLeft}) {
        auto stroke = warp;
        stroke.tool = tool;
        if (tool != LiquifyTool::PushLeft)
            stroke.points = {{{32, 24}, 1}, {{32, 24}, 1}, {{32, 24}, 1}};
        o.strokes = {stroke};
        const auto out = liquify(in.view(), o);
        require(bool(out) && out->pixels != in.pixels, "Liquify deformation tool changed nothing");
        contract(*out, in);
    }
    o.strokes = {warp};
    const auto baseline = liquify(in.view(), o);
    for (auto tool : {LiquifyTool::Smooth, LiquifyTool::Reconstruct}) {
        auto stroke = warp;
        stroke.tool = tool;
        stroke.points = {{{30, 24}, 1}, {{30, 24}, 1}, {{30, 24}, 1}};
        o.strokes = {warp, stroke};
        const auto out = liquify(in.view(), o);
        require(bool(out) && out->pixels != baseline->pixels,
                "Reconstruct/Smooth did not alter existing field");
    }
    LiquifyStroke global;
    global.tool = LiquifyTool::FreezeAll;
    o.strokes = {global, warp};
    require(liquify(in.view(), o)->pixels == in.pixels, "Global freeze failed");
    global.tool = LiquifyTool::ThawAll;
    o.strokes.push_back(global);
    o.strokes.push_back(warp);
    require(liquify(in.view(), o)->pixels != in.pixels, "Global thaw failed");
    global.tool = LiquifyTool::ReconstructAll;
    o.strokes = {warp, global};
    require(liquify(in.view(), o)->pixels == in.pixels, "Full reconstruction not identity");
    global.amount = 0;
    o.strokes = {warp, global};
    require(liquify(in.view(), o)->pixels == baseline->pixels, "Zero reconstruction changed field");
    global.tool = LiquifyTool::FreezeAll;
    global.amount.reset();
    LiquifyStroke invert;
    invert.tool = LiquifyTool::InvertFreeze;
    o.strokes = {global, invert, warp};
    require(liquify(in.view(), o)->pixels == baseline->pixels, "Invert freeze failed");
    LiquifyStroke freeze;
    freeze.tool = LiquifyTool::Freeze;
    freeze.size = 150;
    freeze.density = 0;
    freeze.points = {{{32, 24}, 1}};
    o.strokes = {freeze, warp};
    require(liquify(in.view(), o)->pixels == in.pixels, "Brush freeze failed");
    freeze.tool = LiquifyTool::Thaw;
    o.strokes.insert(o.strokes.begin() + 1, freeze);
    require(liquify(in.view(), o)->pixels == baseline->pixels, "Brush thaw failed");
    LiquifyStroke lasso;
    lasso.tool = LiquifyTool::LassoMask;
    lasso.points = {{{0, 0}, 1}, {{64, 0}, 1}, {{64, 48}, 1}, {{0, 48}, 1}};
    o.strokes = {lasso, warp};
    require(liquify(in.view(), o)->pixels == in.pixels, "Lasso freeze failed");
    lasso.amount = 0;
    o.strokes.insert(o.strokes.begin() + 1, lasso);
    require(liquify(in.view(), o)->pixels == baseline->pixels, "Lasso thaw failed");
    o.strokes = {warp};
    o.strokes[0].pressure = 0;
    require(liquify(in.view(), o)->pixels == in.pixels, "Zero pressure moved pixels");
    o.strokes[0].pressure = 100;
    for (auto &p : o.strokes[0].points)
        p.pressure = 0;
    require(liquify(in.view(), o)->pixels == in.pixels, "Zero recorded pressure moved pixels");
    {
        LiquifyOptions bad;
        bad.cell = .5;
        invalid(liquify, bad);
    }
    {
        LiquifyOptions bad;
        bad.strokes.resize(65);
        invalid(liquify, bad);
    }
    for (const auto v : {-1., 101., std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        auto bad = LiquifyOptions{};
        bad.strokes = {warp};
        bad.strokes[0].density = v;
        invalid(liquify, bad);
        bad.strokes[0].density = 50;
        bad.strokes[0].pressure = v;
        invalid(liquify, bad);
        bad.strokes[0].pressure = 100;
        bad.strokes[0].rate = v;
        invalid(liquify, bad);
    }
    {
        LiquifyOptions bad;
        bad.strokes = {warp};
        bad.strokes[0].tool = static_cast<LiquifyTool>(999);
        invalid(liquify, bad);
    }
    {
        LiquifyOptions bad;
        bad.strokes = {warp};
        bad.strokes[0].points[0].pressure = 2;
        invalid(liquify, bad);
    }
    {
        LiquifyOptions bad;
        bad.strokes = {warp};
        bad.strokes[0].size = 1;
        bad.strokes[0].points = {{{0, 0}, 1}, {{100000, 0}, 1}};
        invalid(liquify, bad);
    }
}
MeshPoint mapGrid(const MeshGeometry &mesh, MeshPoint p) {
    std::size_t stride = 1;
    while (stride < mesh.source.size() && mesh.source[stride][1] == mesh.source[0][1])
        ++stride;
    const auto nx = stride - 1, ny = mesh.source.size() / stride - 1;
    const auto width = mesh.source[stride - 1][0], height = mesh.source.back()[1],
               gx = std::clamp(p[0] / width * nx, 0., double(nx) - 1e-9),
               gy = std::clamp(p[1] / height * ny, 0., double(ny) - 1e-9);
    const auto x = std::size_t(gx), y = std::size_t(gy), a = y * stride + x;
    const auto tx = gx - x, ty = gy - y;
    const std::array<std::size_t, 4> indices{a, a + 1, a + stride, a + stride + 1};
    const std::array<double, 4> weights{(1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty};
    MeshPoint out{};
    for (int k = 0; k < 4; ++k)
        for (int c = 0; c < 2; ++c)
            out[c] += weights[k] * mesh.destination[indices[k]][c];
    return out;
}
void wideTests() {
    const auto in = image(480, 320);
    require(adaptiveWideAngle(in.view())->pixels == in.pixels, "Default wide angle not identity");
    AdaptiveWideAngleOptions o;
    o.model = WideAngleModel::Fisheye;
    o.focalLength = 8;
    o.constraints = {{{60, 70}, {420, 70}, WideAngleOrientation::Horizontal},
                     {{80, 60}, {80, 260}, WideAngleOrientation::Free}};
    const auto solved = solveAdaptiveWideAngle(in.view(), o);
    require(bool(solved), "Wide angle solve failed");
    geometry(*solved);
    require(solved->constraintCurves.size() == 2 && solved->constraintCurves[0].size() == 25,
            "Camera arc not generated");
    double bow = 0, minY = 1e9, maxY = -1e9;
    for (auto p : solved->constraintCurves[0]) {
        bow = std::max(bow, std::abs(p[1] - 70));
        const auto mapped = mapGrid(*solved, p);
        minY = std::min(minY, mapped[1]);
        maxY = std::max(maxY, mapped[1]);
    }
    require(bow > 10, "Fisheye great-circle arc did not bend");
    require(solved->constraintResidual < 2 && maxY - minY < 2,
            "Wide angle constraints did not straighten");
    const auto out = adaptiveWideAngle(in.view(), o);
    require(bool(out) && out->pixels != in.pixels, "Wide angle constraint changed nothing");
    contract(*out, in);
    const auto small = image();
    for (auto model : {WideAngleModel::Auto, WideAngleModel::Fisheye, WideAngleModel::Perspective,
                       WideAngleModel::FullSpherical})
        for (auto orientation : {WideAngleOrientation::Free, WideAngleOrientation::Horizontal,
                                 WideAngleOrientation::Vertical}) {
            AdaptiveWideAngleOptions opts;
            opts.model = model;
            opts.meshCells = 8;
            opts.constraints = {{{10, 9}, {52, 15}, orientation}};
            consistency(adaptiveWideAngle, opts);
        }
    AdaptiveWideAngleOptions scaled;
    scaled.scale = 80;
    auto opaque = small;
    for (auto &p : opaque.pixels)
        p.a = 1;
    const auto shrink = adaptiveWideAngle(opaque.view(), scaled);
    require(shrink->pixels[0].a == 0 && shrink->pixels[24 * 64 + 32].a > .99,
            "Wide angle centered scale footprint wrong");
    {
        auto bad = o;
        bad.constraints[0].b = bad.constraints[0].a;
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.model = static_cast<WideAngleModel>(999);
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.constraints[0].orientation = static_cast<WideAngleOrientation>(999);
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.focalLength = -1;
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.cropFactor = 0;
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.scale = 0;
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.meshCells = 97;
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.solverIterations = 0;
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.interpolation = static_cast<MeshInterpolation>(999);
        invalid(adaptiveWideAngle, bad);
    }
    {
        auto bad = o;
        bad.constraints.resize(33);
        invalid(adaptiveWideAngle, bad);
    }
    {
        AdaptiveWideAngleOptions bad;
        bad.model = WideAngleModel::FullSpherical;
        bad.constraints = {{{0, 24}, {32, 24}, WideAngleOrientation::Free}};
        invalid(adaptiveWideAngle, bad);
    }
}
MeshPoint mapTriangles(const MeshGeometry &mesh, MeshPoint p) {
    for (auto t : mesh.triangles) {
        const auto a = mesh.source[t[0]], b = mesh.source[t[1]], c = mesh.source[t[2]];
        const auto det = (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]);
        if (std::abs(det) < 1e-12)
            continue;
        const auto l1 = ((p[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (p[1] - a[1])) / det;
        const auto l2 = ((b[0] - a[0]) * (p[1] - a[1]) - (p[0] - a[0]) * (b[1] - a[1])) / det;
        const auto l0 = 1 - l1 - l2;
        if (l0 < -1e-9 || l1 < -1e-9 || l2 < -1e-9)
            continue;
        return {l0 * mesh.destination[t[0]][0] + l1 * mesh.destination[t[1]][0] +
                    l2 * mesh.destination[t[2]][0],
                l0 * mesh.destination[t[0]][1] + l1 * mesh.destination[t[1]][1] +
                    l2 * mesh.destination[t[2]][1]};
    }
    require(false, "Pin point not covered by source triangle");
    return {};
}
void puppetTests() {
    auto in = image();
    for (auto &p : in.pixels)
        p.a = 1;
    require(puppetWarp(in.view())->pixels == in.pixels, "Default puppet not identity");
    PuppetWarpOptions o;
    o.expansion = 0;
    o.density = PuppetDensity::Fewer;
    o.pins = {{{8, 8}, {8, 8}, {}, 0}, {{52, 38}, {56, 32}, {}, 1}};
    const auto solved = solvePuppetWarp(in.view(), o);
    require(bool(solved), "Puppet solver failed");
    geometry(*solved);
    require(!solved->triangles.empty(), "Puppet mesh empty");
    const auto nearestPin = [&](MeshPoint source) {
        double best = 1e9;
        std::size_t vertex = 0;
        for (std::size_t k = 0; k < solved->source.size(); ++k) {
            const auto d =
                std::hypot(solved->source[k][0] - source[0], solved->source[k][1] - source[1]);
            if (d < best) {
                best = d;
                vertex = k;
            }
        }
        return solved->destination[vertex];
    };
    const auto fixed = nearestPin({8, 8});
    require(std::hypot(fixed[0] - 8, fixed[1] - 8) < 3, "Puppet fixed pin not held");
    for (const auto &pin : o.pins) {
        const auto constrained = mapTriangles(*solved, pin.source);
        require(std::hypot(constrained[0] - pin.destination[0],
                           constrained[1] - pin.destination[1]) < .01,
                "Puppet barycentric pin constraint not met");
    }
    const auto out = puppetWarp(in.view(), o);
    require(bool(out) && out->pixels != in.pixels, "Puppet moved pin changed nothing");
    contract(*out, in);
    for (auto mode : {PuppetMode::Rigid, PuppetMode::Normal, PuppetMode::Distort})
        for (auto density : {PuppetDensity::Fewer, PuppetDensity::Normal, PuppetDensity::More}) {
            auto opts = o;
            opts.mode = mode;
            opts.density = density;
            opts.iterations = 5;
            opts.pins[1].rotation = 20;
            consistency(puppetWarp, opts);
        }
    auto translation = o;
    translation.pins = {{{16, 16}, {19, 18}, {}, 0}};
    translation.mode = PuppetMode::Rigid;
    const auto translated = solvePuppetWarp(in.view(), translation);
    require(bool(translated), "Puppet translation solve failed");
    for (std::size_t k = 0; k < translated->source.size(); ++k)
        require(std::hypot(translated->destination[k][0] - translated->source[k][0] - 3,
                           translated->destination[k][1] - translated->source[k][1] - 2) < 1e-5,
                "Single pin rigid translation oracle mismatch");
    auto islands = in;
    for (std::size_t y = 0; y < islands.height; ++y)
        for (std::size_t x = 0; x < islands.width; ++x)
            if (x > 20 && x < 44)
                islands.pixels[y * 64 + x].a = 0;
    PuppetWarpOptions oneIsland;
    oneIsland.expansion = 0;
    oneIsland.density = PuppetDensity::More;
    oneIsland.pins = {{{10, 20}, {14, 20}, {}, 0}};
    const auto disconnected = solvePuppetWarp(islands.view(), oneIsland);
    require(bool(disconnected), "Disconnected puppet failed");
    for (std::size_t k = 0; k < disconnected->source.size(); ++k)
        if (disconnected->source[k][0] > 44)
            require(std::hypot(disconnected->source[k][0] - disconnected->destination[k][0],
                               disconnected->source[k][1] - disconnected->destination[k][1]) < 1e-6,
                    "Unpinned component moved");
    for (auto expansion : {-3., 0., 3.}) {
        auto opts = o;
        opts.expansion = expansion;
        const auto mesh = solvePuppetWarp(in.view(), opts);
        require(bool(mesh), "Coverage expansion failed");
        geometry(*mesh);
    }
    {
        auto bad = o;
        bad.pins.resize(65);
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.mode = static_cast<PuppetMode>(999);
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.density = static_cast<PuppetDensity>(999);
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.expansion = 201;
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.iterations = 0;
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.solverIterations = 2001;
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.pins[0].rotation = 361;
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.pins[0].depth = 10001;
        invalid(puppetWarp, bad);
    }
    {
        auto bad = o;
        bad.pins[0].source[0] = NAN;
        invalid(puppetWarp, bad);
    }
}
void perspectiveTests() {
    const auto in = image();
    require(perspectiveWarp(in.view())->pixels == in.pixels, "Empty perspective not identity");
    const MeshQuad source{{{0, 0}, {100, 0}, {100, 100}, {0, 100}}},
        destination{{{20, 0}, {80, 0}, {100, 100}, {0, 100}}};
    PerspectiveWarpOptions o;
    o.planes = {{source, destination}};
    for (int k = 0; k < 4; ++k) {
        const auto mapped = mapPerspectiveWarpPoint(source[k], o);
        require(bool(mapped) && std::hypot((*mapped)[0] - destination[k][0],
                                           (*mapped)[1] - destination[k][1]) < 1e-9,
                "Perspective corner oracle mismatch");
    }
    const auto center = mapPerspectiveWarpPoint({50, 50}, o);
    require(bool(center) && std::abs((*center)[0] - 50) < 1e-9 &&
                std::abs((*center)[1] - 37.5) < 1e-9,
            "Perspective homography center oracle mismatch");
    consistency(perspectiveWarp, o);
    PerspectiveWarpOptions pair;
    pair.planes = {
        {{{{0, 0}, {50, 0}, {50, 100}, {0, 100}}}, {{{0, 10}, {50, 0}, {55, 100}, {0, 90}}}},
        {{{{50.2, 0}, {100, 0}, {100, 100}, {50, 100.2}}},
         {{{50, 0}, {110, 20}, {100, 80}, {55, 100}}}}};
    for (auto t : {.05, .25, .5, .77, .95}) {
        const auto x = 50.1 - .1 * t, y = 100.1 * t;
        const auto left = mapPerspectiveWarpPoint({x - 1e-6, y}, pair),
                   right = mapPerspectiveWarpPoint({x + 1e-6, y}, pair);
        require(bool(left) && bool(right) &&
                    std::hypot((*left)[0] - (*right)[0], (*left)[1] - (*right)[1]) < 1e-4,
                "Linked perspective edge discontinuous");
    }
    PerspectiveWarpOptions straighten;
    straighten.planes = {
        {{{{0, 0}, {100, 0}, {100, 100}, {0, 100}}}, {{{0, 3}, {100, -2}, {96, 100}, {4, 104}}}}};
    straighten.straighten = PerspectiveStraighten::Vertical;
    const auto a = mapPerspectiveWarpPoint({100, 0}, straighten),
               b = mapPerspectiveWarpPoint({100, 100}, straighten);
    require(std::abs((*a)[0] - (*b)[0]) < 1e-9, "Vertical straighten failed");
    straighten.straighten = PerspectiveStraighten::Auto;
    const auto c = mapPerspectiveWarpPoint({0, 0}, straighten),
               d = mapPerspectiveWarpPoint({100, 0}, straighten);
    require(std::abs((*c)[1] - (*d)[1]) < 1e-9, "Auto straighten failed");
    {
        auto bad = o;
        bad.planes[0].source[1] = bad.planes[0].source[0];
        invalid(perspectiveWarp, bad);
    }
    {
        auto bad = o;
        std::swap(bad.planes[0].destination[1], bad.planes[0].destination[2]);
        invalid(perspectiveWarp, bad);
    }
    {
        auto bad = o;
        bad.planes.resize(17);
        invalid(perspectiveWarp, bad);
    }
    {
        auto bad = o;
        bad.cell = 0;
        invalid(perspectiveWarp, bad);
    }
    {
        auto bad = o;
        bad.straighten = static_cast<PerspectiveStraighten>(999);
        invalid(perspectiveWarp, bad);
    }
    require(!mapPerspectiveWarpPoint({NAN, 0}, o), "Nonfinite point accepted");
    // Premultiplied geometric interpolation keeps a zero-alpha red pixel from tinting green.
    Image alpha{2, 1, {{1, 0, 0, 0}, {0, 1, 0, 1}}};
    PerspectiveWarpOptions shift;
    shift.interpolation = MeshInterpolation::Bilinear;
    shift.cell = 1;
    shift.planes = {
        {{{{0, 0}, {2, 0}, {2, 1}, {0, 1}}}, {{{-.5, 0}, {1.5, 0}, {1.5, 1}, {-.5, 1}}}}};
    const auto mixed = perspectiveWarp(alpha.view(), shift);
    require(bool(mixed) && mixed->pixels[0].r == 0 && mixed->pixels[0].g == 1 &&
                std::abs(mixed->pixels[0].a - .5) < 1e-7,
            "Premultiplied geometric alpha oracle mismatch");
}
MeshQuad floorQuad() {
    const auto project = [](std::array<double, 3> p) {
        const auto sx = std::sin(.6), cx = std::cos(.6), sy = std::sin(.4), cy = std::cos(.4);
        const std::array<double, 3> q{p[0], cx * p[1] - sx * p[2], sx * p[1] + cx * p[2]};
        const std::array<double, 3> r{cy * q[0] + sy * q[2], q[1], -sy * q[0] + cy * q[2] + 4};
        return MeshPoint{400 + 500 * r[0] / r[2], 300 + 500 * r[1] / r[2]};
    };
    return {project({-1, -.5, 0}), project({1, -.5, 0}), project({1, .5, 0}), project({-1, .5, 0})};
}
void vanishingTests() {
    const auto in = image();
    require(vanishingPoint(in.view())->pixels == in.pixels, "Empty vanishing point not identity");
    VanishingPointOptions o;
    o.planes = {{{{{0, 0}, {64, 0}, {64, 48}, {0, 48}}}}};
    require(vanishingPoint(in.view(), o)->pixels == in.pixels, "Plane setup changed image");
    const Image red{8, 4, std::vector<Rgba>(32, Rgba{1, 0, 0, 1})};
    o.pastes = {{red.view(), 0, {.25, .25}, .5, 1}};
    const auto pasted = vanishingPoint(in.view(), o);
    require(bool(pasted) && pasted->pixels[24 * 64 + 32].r > .99 &&
                pasted->pixels[24 * 64 + 32].g == 0,
            "Metric paste failed");
    require(pasted->pixels[2 * 64 + 2] == in.pixels[2 * 64 + 2], "Paste modified outside target");
    require(pasted->pixels[24 * 64 + 32].r > .99 &&
                pasted->pixels[31 * 64 + 32] == in.pixels[31 * 64 + 32],
            "Metric paste did not preserve 2:1 source aspect");

    contract(*pasted, in);
    consistency(vanishingPoint, o);
    o.pastes[0].opacity = 0;
    require(vanishingPoint(in.view(), o)->pixels == in.pixels, "Zero paste opacity changed pixels");
    auto base = image(800, 600);
    for (auto &p : base.pixels)
        p = {.2f, .2f, .2f, 1};
    VanishingPointOptions tilted;
    tilted.planes = {{floorQuad()}};
    const auto planes = vanishingPointPlanes(base.view(), tilted);
    require(bool(planes) && planes->size() == 1, "Metric plane setup failed");
    tilted.tearOffs = {{0, VanishingEdge::Top, 90, 1}};
    const auto torn = vanishingPointPlanes(base.view(), tilted);
    require(bool(torn) && torn->size() == 2, "3D plane tear-off failed");
    require(std::hypot((*torn)[1].corners[3][0] - (*torn)[0].corners[0][0],
                       (*torn)[1].corners[3][1] - (*torn)[0].corners[0][1]) < 1e-6,
            "Tear-off lost shared edge");
    VanishingPointOptions clone;
    clone.planes = {{{{{0, 0}, {64, 0}, {64, 48}, {0, 48}}}}};
    auto colors = image();
    for (auto &p : colors.pixels)
        p = {.2f, .2f, .2f, 1};
    for (std::size_t y = 32; y < 42; ++y)
        for (std::size_t x = 26; x < 38; ++x)
            colors.pixels[y * 64 + x] = {1, 0, 0, 1};
    clone.clones = {{{32.5, 37.5}, {{32.5, 13.5}}, 8, .8, 1}};
    const auto cloned = vanishingPoint(colors.view(), clone);
    require(bool(cloned) && cloned->pixels[13 * 64 + 32].r > .99 &&
                cloned->pixels[13 * 64 + 32].g == 0,
            "Perspective clone oracle mismatch");
    clone.clones[0].opacity = 0;
    require(vanishingPoint(colors.view(), clone)->pixels == colors.pixels,
            "Zero clone opacity changed pixels");
    {
        auto bad = o;
        bad.pastes[0].width = 0;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = o;
        bad.pastes[0].plane = 1;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = o;
        bad.planes[0].corners[0] = bad.planes[0].corners[1];
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = o;
        bad.focalLength = -1;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = clone;
        bad.clones[0].hardness = 2;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = clone;
        bad.clones[0].size = 4097;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = tilted;
        bad.tearOffs[0].from = 4;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = tilted;
        bad.tearOffs[0].angle = 180;
        invalid(vanishingPoint, bad);
    }
    {
        auto bad = tilted;
        bad.tearOffs[0].edge = static_cast<VanishingEdge>(999);
        invalid(vanishingPoint, bad);
    }
    auto bad = o;
    bad.pastes[0].image = ImageView{};
    const auto result = vanishingPoint(in.view(), bad);
    require(!result && result.error().code == ErrorCode::InvalidDimensions,
            "Invalid paste pixels accepted");
}
void validationTests() {
    const auto in = image();
    const auto small = image(16, 16);
    for (auto model : {WideAngleModel::Auto, WideAngleModel::Fisheye, WideAngleModel::Perspective,
                       WideAngleModel::FullSpherical})
        for (double focal : {1e-320, std::numeric_limits<double>::denorm_min()})
            for (double crop : {.1, 1., 10.})
                for (bool constrained : {false, true}) {
                    AdaptiveWideAngleOptions extreme;
                    extreme.model = model;
                    extreme.focalLength = focal;
                    extreme.cropFactor = crop;
                    extreme.meshCells = 2;
                    extreme.solverIterations = 1;
                    if (constrained)
                        extreme.constraints = {{{1, 1}, {15, 1}, WideAngleOrientation::Horizontal}};
                    const auto mesh = solveAdaptiveWideAngle(small.view(), extreme);
                    const auto pixels = adaptiveWideAngle(small.view(), extreme);
                    require(bool(mesh) == bool(pixels), "Extreme camera result contracts differ");
                    if (!mesh) {
                        require(constrained && mesh.error().code == ErrorCode::InvalidParameter &&
                                    pixels.error().code == ErrorCode::InvalidParameter,
                                "Extreme projection lacks a structured error");
                    } else {
                        require(!constrained || model != WideAngleModel::Fisheye,
                                "Nonfinite fisheye projection accepted");
                        geometry(*mesh);
                        contract(*pixels, small);
                        if (!constrained)
                            require(pixels->pixels == small.pixels,
                                    "Unconstrained extreme camera lost neutral identity");
                    }
                }
    for (auto fn : {+[](ImageView im) { return liquify(im); },
                    +[](ImageView im) { return adaptiveWideAngle(im); },
                    +[](ImageView im) { return puppetWarp(im); },
                    +[](ImageView im) { return perspectiveWarp(im); },
                    +[](ImageView im) { return vanishingPoint(im); }}) {
        require(!fn(ImageView{}), "Empty mesh image accepted");
        auto bad = in;
        bad.pixels[0].a = std::numeric_limits<float>::infinity();
        const auto out = fn(bad.view());
        require(!out && out.error().code == ErrorCode::InvalidPixel, "Invalid mesh pixel accepted");
        for (const auto &small : {image(1, 1), image(1, 7), image(7, 1)}) {
            const auto out = fn(small.view());
            require(bool(out) && out->pixels == small.pixels, "Neutral thin image changed");
        }
    }
}
} // namespace
int main(int argc, char **argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "all";
    if (mode == "all" || mode == "liquify")
        liquifyTests();
    if (mode == "all" || mode == "wide")
        wideTests();
    if (mode == "all" || mode == "puppet")
        puppetTests();
    if (mode == "all" || mode == "perspective")
        perspectiveTests();
    if (mode == "all" || mode == "vanishing")
        vanishingTests();
    if (mode == "all" || mode == "validation")
        validationTests();
    require(checks > 1, "Unknown mesh test selection");
    std::cout << "Mesh checks passed: " << checks << '\n';
}
