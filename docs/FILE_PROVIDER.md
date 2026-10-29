# File creation ownership in iiGraphicAdjust

The separate image-export example applications link iiFileProvider and delegate file creation to its streams. Image processing and color operations use caller-provided data and remain independent of every other SDK module.

The common [ownership contract](../../iiFileProvider/docs/FILE_OWNERSHIP.md) defines stream, staging, control-file and
installed-package rules. Source regression tests remain with this module; the
workspace creation audit also checks these boundaries. Build/deployment and target
runtime validation are separate evidence.

Changed boundaries:


- `examples/adjust.cpp`
- `examples/develop.cpp`
- `examples/local_corrections.cpp`
