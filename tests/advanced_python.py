"""Exercise the public Python surface against the native editing contracts."""
import unittest
import sys
import os

if len(sys.argv) > 1:
    sys.path[:0] = sys.argv[1:]
    del sys.argv[1:]

if os.name == "nt" and "IIGRAPHICADJUST_TEST_DLL_DIR" in os.environ:
    with os.add_dll_directory(os.environ["IIGRAPHICADJUST_TEST_DLL_DIR"]):
        import iigraphicadjust as g
else:
    import iigraphicadjust as g


def image(colors):
    return g.Image(len(colors), 1, [g.Rgba(*color, 1) for color in colors])


class AdvancedPythonTests(unittest.TestCase):
    def test_content_and_layers(self):
        source = image([(1, 0, 0), (0, 1, 0), (0, 0, 1)])
        protect = g.Mask(3, 1, [0, 1, 0])
        remove = g.Mask(3, 1, [1, 0, 0])
        selection = g.Mask(3, 1, [0, 1, 0])
        self.assertNotEqual(g.ContentAwareInpaintSolver().execute(source, selection).pixels[1].g, 1)
        constraints = g.SeamConstraintMask().execute(protect, remove)
        smaller = g.ContentAwareRescaleSolver().execute(source, 2, 1, constraints)
        self.assertEqual((smaller.width, smaller.height), (2, 1))
        self.assertEqual(smaller.pixels[0].g, 1)
        first = g.Layer(); first.image = source
        second = g.Layer(); second.image = source
        self.assertEqual(g.LayerStackBlendSolver().execute([first, second]).width, 3)
        self.assertEqual(g.LayerRegistrationSolver().execute(source, source).dx, 0)
        self.assertEqual(g.SkyReplacementCompositor().execute(source, source, selection).width, 3)
        refiner = g.SubjectEdgeMatteRefiner(); refiner.amount = 0
        self.assertEqual(refiner.execute(source, protect).coverage, [0, 1, 0])
        self.assertEqual(g.ColorDecontaminationPass().execute(source, source, protect).width, 3)

    def test_preview_history_and_cache(self):
        source = image([(1, 0, 0), (0, 1, 0)])
        options = g.LensCorrectionOptions(); options.scale = 120
        state = g.LensCorrectionPreviewState()
        edited = state.execute(source, g.PreviewAction.REPLACE, options)
        self.assertTrue(edited["can_undo"])
        self.assertEqual(state.execute(source, g.PreviewAction.UNDO)["settings"].scale, 100)
        self.assertEqual(state.execute(source, g.PreviewAction.REDO)["settings"].scale, 120)
        for cls in (g.PuppetWarpPreviewState, g.PerspectiveWarpPreviewState,
                    g.AdaptiveWideAnglePreviewState, g.LiquifyPreviewState,
                    g.VanishingPointPreviewState):
            self.assertEqual(cls().execute(source)["image"].width, 2)
        pipeline = g.FilterGalleryPipeline()
        effect = g.GalleryCutoutOptions(); effect.number_of_levels = 5
        frame = pipeline.execute(source, g.PreviewAction.REPLACE, [effect])
        self.assertEqual(frame["settings"][0].number_of_levels, 5)
        key = g.PreviewCacheKey(); key.source_id = 1; key.pipeline_id = 2
        key.pipeline_revision = frame["revision"]
        cache = g.FilterPreviewCache()
        cache.execute(g.PreviewCacheAction.STORE, key, source, frame["image"])
        self.assertTrue(cache.execute(g.PreviewCacheAction.LOOKUP, key, source)["hit"])
        changed = image([(.5, 0, 0), (0, 1, 0)])
        self.assertFalse(cache.execute(g.PreviewCacheAction.LOOKUP, key, changed)["hit"])
        tone = g.Float32PreviewToneMapper().execute(1, 1, [g.HdrRgba(2, 1, .5, 1)])
        self.assertLessEqual(tone.pixels[0].r, 1)


if __name__ == "__main__":
    unittest.main()
