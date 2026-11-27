#include <iiGraphicAdjust/ContentAware.h>
#include <iiGraphicAdjust/LayerCompositing.h>
#include <iiGraphicAdjust/Preview.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace iiGraphicAdjust;
using namespace iiGraphicAdjust::objects;

namespace {
void check(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
bool near(float a, float b) { return std::abs(a-b) < 0.015F; }

void content() {
    Image image{3,2,{{1,0,0,1},{0,1,0,1},{0,0,1,1},
                     {1,0,0,1},{0,1,0,1},{0,0,1,1}}};
    Mask selected{3,2,{0,1,0,0,1,0}};
    auto fill = ContentAwareInpaintSolver{}.execute(image.view(), selected.view());
    check(fill && fill->pixels[0]==image.pixels[0] && fill->pixels[1]!=image.pixels[1],
          "inpaint preserves unmasked pixels and fills the selection");
    Mask empty{3,2,{0,0,0,0,0,0}};
    auto identity=ContentAwareInpaintSolver{}.execute(image.view(),empty.view());
    check(identity && identity->pixels==image.pixels,"empty inpaint mask is an exact detached copy");
    Mask all{3,2,{1,1,1,1,1,1}};
    check(!ContentAwareInpaintSolver{}.execute(image.view(),all.view()),"all-masked input rejected");
    auto repeated=ContentAwareInpaintSolver{}.execute(image.view(),selected.view());
    check(repeated && fill->pixels==repeated->pixels,"inpainting is deterministic");
    Mask invalid=selected; invalid.coverage[0]=std::numeric_limits<float>::quiet_NaN();
    check(!ContentAwareInpaintSolver{}.execute(image.view(),invalid.view()),"invalid mask rejected");
    Mask protect{3,2,{0,1,0,0,1,0}}, remove{3,2,{1,0,0,1,0,0}};
    auto constraints=SeamConstraintMask{}.execute(protect.view(),remove.view());
    check(constraints && constraints->weights[0]<0 && constraints->weights[1]>0,
          "constraint signs reflect protect and remove masks");
    auto smaller=ContentAwareRescaleSolver{}.execute(image.view(),2,2,constraints->view());
    check(smaller && smaller->width==2 && smaller->height==2 &&
          smaller->pixels[0]==image.pixels[1] && smaller->pixels[1]==image.pixels[2],
          "seam resize honors protection and removal preference");
    auto larger=ContentAwareRescaleSolver{}.execute(image.view(),4,3,constraints->view());
    check(larger && larger->width==4 && larger->height==3,"seam resize enlarges both axes");
    check(!ContentAwareRescaleSolver{}.execute(image.view(),0,2,constraints->view()),"zero width rejected");
    auto bad=*constraints; bad.weights[0]=2;
    check(!ContentAwareRescaleSolver{}.execute(image.view(),2,2,bad.view()),"invalid seam weight rejected");
}

void layers() {
    Image red{2,1,{{1,0,0,1},{1,0,0,1}}}, blue{2,1,{{0,0,1,1},{0,0,1,1}}};
    std::array<LayerView,2> layers{{{red.view(),{}},{blue.view(),{BlendMode::Normal,.5,BlendSpace::Srgb}}}};
    auto result=LayerStackBlendSolver{}.execute(layers);
    check(result && near(result->pixels[0].r,.5F) && near(result->pixels[0].b,.5F),
          "layer stack is bottom to top");
    Mask sky{2,1,{0,1}};
    auto composite=SkyReplacementCompositor{}.execute(red.view(),blue.view(),sky.view());
    check(composite && composite->pixels[0]==red.pixels[0] && composite->pixels[1]==blue.pixels[1],
          "sky compositor follows supplied mask");
    Image transparent{1,1,{{1,0,0,0}}}, opaque{1,1,{{0,0,1,1}}}; Mask middle{1,1,{.5F}};
    auto mixed=SkyReplacementCompositor{}.execute(transparent.view(),opaque.view(),middle.view());
    check(mixed && near(mixed->pixels[0].a,.5F) && near(mixed->pixels[0].r,0) && near(mixed->pixels[0].b,1),
          "sky interpolation does not leak transparent RGB");
    SubjectEdgeMatteRefiner refiner; refiner.amount=0;
    auto matte=refiner.execute(red.view(),sky.view());
    check(matte && matte->coverage==sky.coverage,"zero refinement preserves matte");
    Image observed{1,1,{{.73535698F,0,.73535698F,1}}};
    Image background{1,1,{{0,0,1,1}}}; Mask half{1,1,{.5F}};
    auto clean=ColorDecontaminationPass{}.execute(observed.view(),background.view(),half.view());
    check(clean && near(clean->pixels[0].r,1) && near(clean->pixels[0].b,0) && clean->pixels[0].a==1,
          "decontamination unmixed background in linear light");
    Image reference{4,2,{{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,1,0,1},
                         {1,0,1,1},{0,1,1,1},{.2F,.3F,.4F,1},{.8F,.2F,.1F,1}}};
    Image moving{4,2,{{0,1,0,1},{0,0,1,1},{1,1,0,1},{0,0,0,0},
                      {0,1,1,1},{.2F,.3F,.4F,1},{.8F,.2F,.1F,1},{0,0,0,0}}};
    LayerRegistrationSolver solver; solver.maxShiftX=2; solver.maxShiftY=0;
    auto registration=solver.execute(reference.view(),moving.view());
    check(registration && registration->dx==1 && registration->dy==0,
          "registration recovers translation in reference coordinates");
    solver.minOverlap=1;
    Image invisible=moving; for (auto& pixel:invisible.pixels) pixel.a=0;
    check(!solver.execute(reference.view(),invisible.view()),"invisible registration input rejected");
}

void previews() {
    Image image{2,2,{{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,1,1,1}}};
    LensCorrectionPreviewState state;
    auto first=state.execute(image.view(),PreviewAction::Refresh);
    LensCorrectionOptions edit; edit.scale=120;
    auto changed=state.execute(image.view(),PreviewAction::Replace,edit);
    check(first && changed && changed->revision>first->revision && changed->canUndo,
          "preview edit renders and exposes revision");
    LensCorrectionOptions invalid; invalid.scale=-1;
    check(!state.execute(image.view(),PreviewAction::Replace,invalid),"invalid edit rejected");
    auto undone=state.execute(image.view(),PreviewAction::Undo);
    check(undone && undone->revision>changed->revision && undone->canRedo &&
          undone->image.pixels==image.pixels,"undo restores previous preview after rejected edit");
    auto redone=state.execute(image.view(),PreviewAction::Redo);
    check(redone && redone->settings.scale==120,"redo restores edited settings");
    auto missingRedo=state.execute(image.view(),PreviewAction::Redo);
    auto afterFailure=state.execute(image.view(),PreviewAction::Refresh);
    check(!missingRedo && afterFailure && afterFailure->revision==redone->revision,
          "failed history movement does not change revision");
    check(bool(PuppetWarpPreviewState{}.execute(image.view(),PreviewAction::Refresh)),"puppet preview");
    check(bool(PerspectiveWarpPreviewState{}.execute(image.view(),PreviewAction::Refresh)),"perspective preview");
    check(bool(AdaptiveWideAnglePreviewState{}.execute(image.view(),PreviewAction::Refresh)),"wide angle preview");
    check(bool(LiquifyPreviewState{}.execute(image.view(),PreviewAction::Refresh)),"liquify preview");
    check(bool(VanishingPointPreviewState{}.execute(image.view(),PreviewAction::Refresh)),"vanishing preview");
    FilterGalleryPipeline pipeline;
    auto frame=pipeline.execute(image.view(),PreviewAction::Refresh);
    check(frame && frame->image.pixels==image.pixels,"empty gallery preserves image");
    FilterPreviewCache cache;
    PreviewCacheRequest request; request.action=PreviewCacheAction::Store;
    request.key={1,1,2,frame->revision}; request.source=image.view(); request.result=frame->image.view();
    check(bool(cache.execute(request)),"cache stores preview");
    request.action=PreviewCacheAction::Lookup;
    check(cache.execute(request)->image.has_value(),"cache hits matching input");
    image.pixels[0].r=.5F;
    request.source=image.view();
    auto stale=cache.execute(request);
    check(stale && !stale->image,"source change invalidates cache without caller revision change");
    request.action=PreviewCacheAction::Store; request.result=frame->image.view();
    check(bool(cache.execute(request)),"cache stores updated source");
    request.action=PreviewCacheAction::Lookup; ++request.key.pipelineRevision;
    auto changedPipeline=cache.execute(request);
    check(changedPipeline && !changedPipeline->hit,"pipeline revision invalidates old cache entries");
    const std::array<HdrRgba,1> hdr{{{2,1,.5F,1}}};
    auto mapped=Float32PreviewToneMapper{}.execute({1,1,hdr});
    check(mapped && mapped->pixels[0].r<=1 && mapped->pixels[0].b>0,"float preview maps HDR");
    const std::array<HdrRgba,1> invalidHdr{{{-1,0,0,1}}};
    check(!Float32PreviewToneMapper{}.execute({1,1,invalidHdr}),"negative HDR input is rejected");
}
} // namespace

int main(int argc,char **argv) {
    if (argc!=2) return 2;
    const std::string_view which=argv[1];
    if (which=="content") content();
    else if (which=="layers") layers();
    else if (which=="previews") previews();
    else return 2;
}
