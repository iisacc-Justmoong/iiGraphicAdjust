"""Run package checks in an isolated resource workspace."""
import argparse
import hashlib
import os
from pathlib import Path, PureWindowsPath
import re
import shutil
import subprocess
import ii_file_provider as provider


def redirect_manifest(content, build, workspace):
    found = 0
    for name in ("install_local_manifest.txt", "${CMAKE_INSTALL_MANIFEST}"):
        old = build.as_posix() + "/" + name
        count = content.count(old)
        if count > 1:
            raise ValueError("Cannot safely redirect the installation manifest")
        if count == 1:
            content = content.replace(old, workspace.as_posix() + "/" + name)
            found += 1
    if found == 0:
        raise ValueError("Cannot safely redirect the installation manifest")
    return content


def reserve_build(build, token):
    if not re.fullmatch(r"[0-9a-f]{4}", token):
        raise ValueError("Unexpected resource workspace name")
    path = build / (".graphic-consumer-" + token)
    provider.mkdir(path, exist_ok=False)
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "build", "workspace", "cmake", "config"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--mode", choices=("licenses", "consumer-build", "consumer"), required=True)
    parser.add_argument("--compiler", default="")
    parser.add_argument("--cxx-flags", default="")
    parser.add_argument("--ctest", default="")
    args = parser.parse_args()
    source = Path(args.source).resolve(strict=True)
    raw_build, raw_workspace = Path(args.build), Path(args.workspace)
    state = source / "resources/.t"
    if raw_build.is_symlink() or raw_workspace.is_symlink() or state.is_symlink():
        raise ValueError("Build and resource workspaces must be direct directories")
    build, workspace, state = (p.resolve(strict=True) for p in (raw_build, raw_workspace, state))
    if build != (source / "build").resolve() or workspace.parent != state:
        raise ValueError("Package test must use this project's build and resource workspace")
    cache = (build / "CMakeCache.txt").read_text()
    for name in ("BINDIR", "DATADIR", "DATAROOTDIR", "INCLUDEDIR", "LIBDIR"):
        match = re.search(r"^CMAKE_INSTALL_" + name + r":[^=]*=(.*)$", cache, re.M)
        if match and match[1] and (Path(match[1]).is_absolute() or PureWindowsPath(match[1]).is_absolute()
                                   or ".." in Path(match[1]).parts or ".." in PureWindowsPath(match[1]).parts):
            raise ValueError("Unsafe install directory: " + name)
    script = redirect_manifest((build / "cmake_install.cmake").read_text(), build, workspace)
    private_script = workspace / "cmake_install.cmake"
    with provider.open(private_script, "x", encoding="utf-8") as output:
        output.write(script)
    prefix = workspace / "install"
    env = {k: v for k, v in os.environ.items() if k.upper() != "DESTDIR"}
    manifest = build / "install_manifest.txt"
    before = manifest.read_bytes() if manifest.is_file() else None
    subprocess.run([args.cmake, "-DCMAKE_INSTALL_LOCAL_ONLY=ON", "-DCMAKE_INSTALL_PREFIX=" + str(prefix),
                    "-DCMAKE_INSTALL_CONFIG_NAME=" + args.config, "-P", str(private_script)], env=env, check=True)
    license_text = (source / "LICENSE").read_text(encoding="utf-8")
    if "Apache License" not in license_text or "Version 2.0, January 2004" not in license_text:
        raise RuntimeError("LICENSE is not the Apache License, Version 2.0")
    notice_text = (source / "NOTICE").read_text(encoding="utf-8")
    if "Copyright (c) 2026 iisacc" not in notice_text:
        raise RuntimeError("NOTICE is missing Copyright (c) 2026 iisacc")
    licenses = (("LICENSE", "LICENSE"), ("NOTICE", "NOTICE"),
                ("third_party/photocraft/LICENSE-MIT", "photocraft/LICENSE-MIT"),
                ("third_party/lightcraft/LICENSE-MIT", "lightcraft/LICENSE-MIT"))
    for original, installed in licenses:
        actual = prefix / "share/licenses/iiGraphicAdjust" / installed
        if not actual.is_file() or hashlib.sha256(actual.read_bytes()).digest() != hashlib.sha256((source / original).read_bytes()).digest():
            raise RuntimeError("Installed license missing or different: " + installed)
    libdir = "lib"
    lib_match = re.search(r"^CMAKE_INSTALL_LIBDIR:[^=]*=(.*)$", cache, re.M)
    if lib_match and lib_match[1]:
        libdir = lib_match[1]
    package_config = prefix / libdir / "cmake/iiGraphicAdjust/iiGraphicAdjustConfig.cmake"
    if 'set(iiGraphicAdjust_SPDX_LICENSE "Apache-2.0")' not in package_config.read_text(encoding="utf-8"):
        raise RuntimeError("Installed package config does not export iiGraphicAdjust_SPDX_LICENSE=Apache-2.0")
    if args.mode in ("consumer-build", "consumer"):
        for name in ("include/iiGraphicAdjust/Objects.h", "include/iiGraphicAdjust/Operation.h",
                     "include/iiGraphicAdjust/CorrectionExtensions.h", "include/iiGraphicAdjust/objects/OwnedResourceObjects.h",
                     "include/iiGraphicAdjust/objects/DevelopFeatureObjects.h", "share/iiGraphicAdjust/docs/OperationObjects.md",
                     "share/iiGraphicAdjust/docs/object-catalog.json", "share/iiGraphicAdjust/docs/adobe-feature-coverage.json"):
            if not (prefix / name).is_file():
                raise RuntimeError("Missing installed object contract: " + name)
        if not args.compiler or (args.mode == "consumer" and not args.ctest):
            raise ValueError("Consumer mode requires its compiler and, for execution, ctest")
        consumer = reserve_build(build, workspace.name)
        try:
            subprocess.run([args.cmake, "-S", str(source / "tests/consumer"), "-B", str(consumer),
                            "-DCMAKE_PREFIX_PATH=" + str(prefix), "-DCMAKE_DISABLE_FIND_PACKAGE_iiFileProviderNative=TRUE",
                            "-DCMAKE_CXX_COMPILER=" + args.compiler, "-DCMAKE_CXX_FLAGS=" + args.cxx_flags,
                            "-DCMAKE_BUILD_TYPE=Release"], env=env, check=True)
            subprocess.run([args.cmake, "--build", str(consumer), "--parallel", "2"], env=env, check=True)
            if args.mode == "consumer":
                subprocess.run([args.ctest, "--test-dir", str(consumer), "--output-on-failure"], env=env, check=True)
        finally:
            shutil.rmtree(consumer)
    after = manifest.read_bytes() if manifest.is_file() else None
    if before != after:
        raise RuntimeError("The project installation manifest changed during the test")


if __name__ == "__main__":
    main()
