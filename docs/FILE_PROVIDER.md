# File creation ownership in iiGraphicAdjust

Native image-export/example file creation delegates to provider streams/directories. Image processing, color operations and codec behavior remain here.

The common [ownership contract](../../iiFileProvider/docs/FILE_OWNERSHIP.md) defines stream, staging, control-file and
installed-package rules. Source regression tests remain with this module; the
workspace creation audit also checks these boundaries. Build/deployment and target
runtime validation are separate evidence.

Changed boundaries:


- `examples/adjust.cpp`
- `examples/develop.cpp`
- `examples/local_corrections.cpp`
