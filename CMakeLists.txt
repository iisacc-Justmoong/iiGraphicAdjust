cmake_minimum_required(VERSION 3.24)
project(iiGraphicAdjust VERSION 0.1.0 LANGUAGES CXX)

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
    src/iiGraphicAdjust/Adjustments.cpp
    src/iiGraphicAdjust/Effects.cpp
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
    src/iiGraphicAdjust/Adjustments.h src/iiGraphicAdjust/Effects.h src/iiGraphicAdjust/Blend.h
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

if(IIGRAPHICADJUST_BUILD_EXAMPLES)
    add_executable(iiGraphicAdjustExample examples/adjust.cpp)
    target_link_libraries(iiGraphicAdjustExample PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    set_target_properties(iiGraphicAdjustExample PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(BUILD_TESTING)
    add_executable(iiGraphicAdjustTests tests/processing.cpp)
    target_link_libraries(iiGraphicAdjustTests PRIVATE iiGraphicAdjust::iiGraphicAdjust)
    set_target_properties(iiGraphicAdjustTests PROPERTIES CXX_EXTENSIONS OFF)
    foreach(_case validation conversion adjustment blend blur sharpen noise invariants)
        add_test(NAME iiGraphicAdjust.${_case} COMMAND iiGraphicAdjustTests ${_case})
        set_tests_properties(iiGraphicAdjust.${_case} PROPERTIES TIMEOUT 20)
    endforeach()
endif()
