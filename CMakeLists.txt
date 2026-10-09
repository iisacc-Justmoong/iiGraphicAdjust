cmake_minimum_required(VERSION 3.24)
project(iiGraphicAdjust VERSION 0.5.0 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local/SDK/iiGraphicAdjust" CACHE PATH "Install prefix" FORCE)
    elseif(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
        set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/.local/SDK/iiGraphicAdjust" CACHE PATH "Install prefix" FORCE)
    else()
        message(FATAL_ERROR "Set CMAKE_INSTALL_PREFIX explicitly")
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(CTest)
option(BUILD_SHARED_LIBS "Build iiGraphicAdjust as a shared library" ON)
option(IIGRAPHICADJUST_BUILD_EXAMPLES "Build the standalone image adjustment example" ON)

add_library(iiGraphicAdjust
    src/iiGraphicAdjust/Image.cpp
    src/iiGraphicAdjust/RawProcessing.cpp
    src/iiGraphicAdjust/PhotoOperations.cpp
    src/iiGraphicAdjust/Adjustments.cpp
    src/iiGraphicAdjust/Effects.cpp
    src/iiGraphicAdjust/Mapping.cpp
    src/iiGraphicAdjust/Hdr.cpp
    src/iiGraphicAdjust/Tone.cpp
    src/iiGraphicAdjust/Color.cpp
    src/iiGraphicAdjust/DevelopTone.cpp
    src/iiGraphicAdjust/DevelopColor.cpp
    src/iiGraphicAdjust/DevelopEffects.cpp
    src/iiGraphicAdjust/Optics.cpp
    src/iiGraphicAdjust/Masks.cpp
    src/iiGraphicAdjust/Develop.cpp
    src/iiGraphicAdjust/BlurFilters.cpp
    src/iiGraphicAdjust/DistortionFilters.cpp
    src/iiGraphicAdjust/UtilityFilters.cpp
    src/iiGraphicAdjust/GalleryFilters.cpp
    src/iiGraphicAdjust/LightcraftFilters.cpp
    src/iiGraphicAdjust/LensFilter.cpp
    src/iiGraphicAdjust/PixelFilters.cpp
    src/iiGraphicAdjust/RenderFilters.cpp
    src/iiGraphicAdjust/MeshFilters.cpp
    src/iiGraphicAdjust/CameraRawFilter.cpp
    src/iiGraphicAdjust/RetouchFilters.cpp
    src/iiGraphicAdjust/DiagnosticFilters.cpp
    src/iiGraphicAdjust/Blend.cpp)
add_library(iiGraphicAdjust::iiGraphicAdjust ALIAS iiGraphicAdjust)
generate_export_header(iiGraphicAdjust BASE_NAME IIGRAPHICADJUST
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/iiGraphicAdjust/Export.h")
target_compile_features(iiGraphicAdjust PUBLIC cxx_std_23)
target_include_directories(iiGraphicAdjust PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(iiGraphicAdjust PUBLIC IIGRAPHICADJUST_STATIC_DEFINE)
endif()
set_target_properties(iiGraphicAdjust PROPERTIES
    CXX_EXTENSIONS OFF CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(MSVC)
    target_compile_options(iiGraphicAdjust PRIVATE /W4 /permissive-)
else()
    target_compile_options(iiGraphicAdjust PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/iiGraphicAdjust")
install(TARGETS iiGraphicAdjust EXPORT iiGraphicAdjustTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
    src/iiGraphicAdjust/iiGraphicAdjust.h src/iiGraphicAdjust/Image.h
    src/iiGraphicAdjust/RawProcessing.h src/iiGraphicAdjust/PhotoOperations.h
    src/iiGraphicAdjust/Adjustments.h src/iiGraphicAdjust/Effects.h src/iiGraphicAdjust/Blend.h
    src/iiGraphicAdjust/Mapping.h src/iiGraphicAdjust/Hdr.h
    src/iiGraphicAdjust/Tone.h src/iiGraphicAdjust/Color.h
    src/iiGraphicAdjust/DevelopTone.h
    src/iiGraphicAdjust/DevelopColor.h
    src/iiGraphicAdjust/DevelopEffects.h
    src/iiGraphicAdjust/Optics.h
    src/iiGraphicAdjust/Masks.h
    src/iiGraphicAdjust/Develop.h
    src/iiGraphicAdjust/BlurFilters.h
    src/iiGraphicAdjust/DistortionFilters.h
    src/iiGraphicAdjust/UtilityFilters.h
    src/iiGraphicAdjust/GalleryFilters.h
    src/iiGraphicAdjust/LightcraftFilters.h
    src/iiGraphicAdjust/LensFilter.h
    src/iiGraphicAdjust/PixelFilters.h
    src/iiGraphicAdjust/RenderFilters.h
    src/iiGraphicAdjust/MeshFilters.h
    src/iiGraphicAdjust/CameraRawFilter.h
    src/iiGraphicAdjust/RetouchFilters.h
    src/iiGraphicAdjust/DiagnosticFilters.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/iiGraphicAdjust/Export.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiGraphicAdjust)
install(EXPORT iiGraphicAdjustTargets NAMESPACE iiGraphicAdjust:: DESTINATION "${_package_dir}")
configure_package_config_file(cmake/iiGraphicAdjustConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiGraphicAdjustConfig.cmake" INSTALL_DESTINATION "${_package_dir}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiGraphicAdjustConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiGraphicAdjustConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiGraphicAdjustConfigVersion.cmake" DESTINATION "${_package_dir}")
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DATADIR}/iiGraphicAdjust)
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DATADIR}/iiGraphicAdjust/docs)
install(DIRECTORY third_party/ DESTINATION ${CMAKE_INSTALL_DATADIR}/iiGraphicAdjust/third_party)

if(IIGRAPHICADJUST_BUILD_EXAMPLES)
    add_executable(iiGraphicAdjustDevelopExample examples/develop.cpp)
    target_link_libraries(iiGraphicAdjustDevelopExample PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    add_executable(iiGraphicAdjustExample examples/adjust.cpp)
    target_link_libraries(iiGraphicAdjustExample PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    set_target_properties(iiGraphicAdjustExample PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(BUILD_TESTING)
    add_executable(iiGraphicAdjustRawTests tests/raw_processing.cpp)
    target_link_libraries(iiGraphicAdjustRawTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case patterns interpolation calibration rendering layout validation)
        add_test(NAME iiGraphicAdjust.raw.${_case} COMMAND iiGraphicAdjustRawTests ${_case})
        set_tests_properties(iiGraphicAdjust.raw.${_case} PROPERTIES TIMEOUT 30)
    endforeach()
    add_executable(iiGraphicAdjustPhotoTests tests/photo_operations.cpp)
    target_link_libraries(iiGraphicAdjustPhotoTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case orientation crop histogram validation)
        add_test(NAME iiGraphicAdjust.photo.${_case} COMMAND iiGraphicAdjustPhotoTests ${_case})
        set_tests_properties(iiGraphicAdjust.photo.${_case} PROPERTIES TIMEOUT 30)
    endforeach()
    add_executable(iiGraphicAdjustDiagnosticFiltersTests tests/diagnostic_filters.cpp)
    target_link_libraries(iiGraphicAdjustDiagnosticFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case point masks gamut validation)
        add_test(NAME iiGraphicAdjust.diagnostic.${_case} COMMAND iiGraphicAdjustDiagnosticFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.diagnostic.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustRetouchFiltersTests tests/retouch_filters.cpp)
    target_link_libraries(iiGraphicAdjustRetouchFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case spots eyes dust validation)
        add_test(NAME iiGraphicAdjust.retouch.${_case} COMMAND iiGraphicAdjustRetouchFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.retouch.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustMeshFiltersTests tests/mesh_filters.cpp)
    target_link_libraries(iiGraphicAdjustMeshFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case liquify wide puppet perspective vanishing validation)
        add_test(NAME iiGraphicAdjust.mesh.${_case} COMMAND iiGraphicAdjustMeshFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.mesh.${_case} PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(iiGraphicAdjustCameraRawFilterTests tests/camera_raw_filter.cpp)
    target_link_libraries(iiGraphicAdjustCameraRawFilterTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case oracle basic color detail effects validation invariants)
        add_test(NAME iiGraphicAdjust.cameraRaw.${_case} COMMAND iiGraphicAdjustCameraRawFilterTests ${_case})
        set_tests_properties(iiGraphicAdjust.cameraRaw.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustLensFilterTests tests/lens_filter.cpp)
    target_link_libraries(iiGraphicAdjustLensFilterTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    add_test(NAME iiGraphicAdjust.filters.lens COMMAND iiGraphicAdjustLensFilterTests)
    set_tests_properties(iiGraphicAdjust.filters.lens PROPERTIES TIMEOUT 60)
    add_executable(iiGraphicAdjustGalleryFiltersTests tests/gallery_filters.cpp)
    target_link_libraries(iiGraphicAdjustGalleryFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case coloredPencil cutout dryBrush galleryFilmGrain fresco neonGlow paintDaubs paletteKnife
            plasticWrap posterEdges roughPastels smudgeStick sponge underpainting watercolor accentedEdges
            angledStrokes crosshatch darkStrokes inkOutlines spatter sprayedStrokes sumiE diffuseGlow glass
            oceanRipple basRelief chalkCharcoal charcoal chrome conteCrayon graphicPen halftonePattern
            notePaper photocopy plaster reticulation stamp tornEdges waterPaper glowingEdges craquelure
            grain mosaicTiles patchwork stainedGlass texturizer oracles stack)
        add_test(NAME iiGraphicAdjust.gallery.${_case} COMMAND iiGraphicAdjustGalleryFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.gallery.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustLightcraftFiltersTests tests/lightcraft_filters.cpp)
    target_link_libraries(iiGraphicAdjustLightcraftFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case looks presets resize invalid)
        add_test(NAME iiGraphicAdjust.lightcraft.${_case} COMMAND iiGraphicAdjustLightcraftFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.lightcraft.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustPixelFiltersTests tests/pixel_filters.cpp)
    target_link_libraries(iiGraphicAdjustPixelFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case pixelate stylize validation)
        add_test(NAME iiGraphicAdjust.pixel.${_case} COMMAND iiGraphicAdjustPixelFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.pixel.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustRenderFiltersTests tests/render_filters.cpp)
    target_link_libraries(iiGraphicAdjustRenderFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case noise light procedural validation)
        add_test(NAME iiGraphicAdjust.render.${_case} COMMAND iiGraphicAdjustRenderFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.render.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustCoreFiltersTests tests/core_filters.cpp)
    target_link_libraries(iiGraphicAdjustCoreFiltersTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case blur distortion utility isolation validation)
        add_test(NAME iiGraphicAdjust.filters.${_case} COMMAND iiGraphicAdjustCoreFiltersTests ${_case})
        set_tests_properties(iiGraphicAdjust.filters.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustMasksInvariantsTests tests/masks_invariants.cpp)
    target_link_libraries(iiGraphicAdjustMasksInvariantsTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    add_test(NAME iiGraphicAdjust.develop.mask-invariants COMMAND iiGraphicAdjustMasksInvariantsTests)
    set_tests_properties(iiGraphicAdjust.develop.mask-invariants PROPERTIES TIMEOUT 60)
    add_executable(iiGraphicAdjustMasksTests tests/masks.cpp)
    target_link_libraries(iiGraphicAdjustMasksTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case shapes combine local pipeline)
        add_test(NAME iiGraphicAdjust.develop.${_case} COMMAND iiGraphicAdjustMasksTests ${_case})
        set_tests_properties(iiGraphicAdjust.develop.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustDevelopInvariantsTests tests/develop_invariants.cpp)
    target_link_libraries(iiGraphicAdjustDevelopInvariantsTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case oracle invariants validation)
        add_test(NAME iiGraphicAdjust.develop.${_case} COMMAND iiGraphicAdjustDevelopInvariantsTests ${_case})
        set_tests_properties(iiGraphicAdjust.develop.${_case} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(iiGraphicAdjustOpticsTests tests/optics.cpp)
    target_link_libraries(iiGraphicAdjustOpticsTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    add_test(NAME iiGraphicAdjust.develop.optics COMMAND iiGraphicAdjustOpticsTests)
    set_tests_properties(iiGraphicAdjust.develop.optics PROPERTIES TIMEOUT 30)
    add_executable(iiGraphicAdjustDevelopEffectsTests tests/develop_effects.cpp)
    target_link_libraries(iiGraphicAdjustDevelopEffectsTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case effects vignette grain detail)
        add_test(NAME iiGraphicAdjust.develop.${_case} COMMAND iiGraphicAdjustDevelopEffectsTests ${_case})
        set_tests_properties(iiGraphicAdjust.develop.${_case} PROPERTIES TIMEOUT 30)
    endforeach()
    add_executable(iiGraphicAdjustDevelopColorTests tests/develop_color.cpp)
    target_link_libraries(iiGraphicAdjustDevelopColorTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case mixer point grading calibration)
        add_test(NAME iiGraphicAdjust.develop.${_case} COMMAND iiGraphicAdjustDevelopColorTests ${_case})
        set_tests_properties(iiGraphicAdjust.develop.${_case} PROPERTIES TIMEOUT 30)
    endforeach()
    add_executable(iiGraphicAdjustDevelopToneTests tests/develop_tone.cpp)
    target_link_libraries(iiGraphicAdjustDevelopToneTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case wb light curve)
        add_test(NAME iiGraphicAdjust.develop.${_case} COMMAND iiGraphicAdjustDevelopToneTests ${_case})
        set_tests_properties(iiGraphicAdjust.develop.${_case} PROPERTIES TIMEOUT 30)
    endforeach()
    add_executable(iiGraphicAdjustTests tests/processing.cpp)
    target_link_libraries(iiGraphicAdjustTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    set_target_properties(iiGraphicAdjustTests PROPERTIES CXX_EXTENSIONS OFF)
    foreach(_case validation conversion adjustment blend blur sharpen noise invariants)
        add_test(NAME iiGraphicAdjust.${_case} COMMAND iiGraphicAdjustTests ${_case})
        set_tests_properties(iiGraphicAdjust.${_case} PROPERTIES TIMEOUT 20)
    endforeach()
    add_executable(iiGraphicAdjustMappingTests tests/mapping.cpp)
    target_link_libraries(iiGraphicAdjustMappingTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case gradient lookup parser)
        add_test(NAME iiGraphicAdjust.mapping.${_case} COMMAND iiGraphicAdjustMappingTests ${_case})
    endforeach()
    add_executable(iiGraphicAdjustHdrTests tests/hdr.cpp)
    target_link_libraries(iiGraphicAdjustHdrTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    add_test(NAME iiGraphicAdjust.hdr COMMAND iiGraphicAdjustHdrTests)
    add_executable(iiGraphicAdjustToneTests tests/tone.cpp)
    target_link_libraries(iiGraphicAdjustToneTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case brightness levels curves exposure shadows discrete histogram automatic validation invariants)
        add_test(NAME iiGraphicAdjust.tone.${_case} COMMAND iiGraphicAdjustToneTests ${_case})
        set_tests_properties(iiGraphicAdjust.tone.${_case} PROPERTIES TIMEOUT 20)
    endforeach()
    add_executable(iiGraphicAdjustColorTests tests/color.cpp)
    target_link_libraries(iiGraphicAdjustColorTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    foreach(_case hue vibrance balance black-white filter mixer selective replace match validation invariants)
        add_test(NAME iiGraphicAdjust.color.${_case} COMMAND iiGraphicAdjustColorTests ${_case})
        set_tests_properties(iiGraphicAdjust.color.${_case} PROPERTIES TIMEOUT 20)
    endforeach()
    set_target_properties(iiGraphicAdjustMappingTests iiGraphicAdjustHdrTests
        iiGraphicAdjustToneTests iiGraphicAdjustColorTests PROPERTIES CXX_EXTENSIONS OFF)
    set_tests_properties(iiGraphicAdjust.mapping.gradient iiGraphicAdjust.mapping.lookup
        iiGraphicAdjust.mapping.parser iiGraphicAdjust.hdr PROPERTIES TIMEOUT 20)
endif()
