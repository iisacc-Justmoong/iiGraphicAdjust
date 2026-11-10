# Build and test resources

The C++23 module and test executables use `build/`. The package stage used by
an existing Dreamscapes cache is retained under `resources/dependencies/stage`;
Unix builds recreate a guarded `build/stage` link when that stage exists.
Existing data, links pointing elsewhere and resources escaping the project are
not replaced.

License and installed-consumer checks use unique `resources/.t` workspaces.
The checks redirect CMake's installation manifest rather than overwriting the
project's manifest and reject unsafe absolute or escaping install paths. The
consumer compiles in a temporary `build/.graphic-consumer-*` directory; normal
completion and test failure remove both owned locations. A forced termination
can leave its workspace for review. `iiGraphicAdjust.objects.installed_build`
checks the installed consumer's compilation without running it; the original
`iiGraphicAdjust.objects.installed_package` also executes its tests when
explicitly selected. Prior user-authored probes and potentially
private data are retained under `resources/.recovery/graphic-history-20261031`.
