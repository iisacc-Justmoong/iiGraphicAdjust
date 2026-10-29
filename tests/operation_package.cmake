foreach(_required SOURCE_ROOT BINARY_ROOT)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "Missing ${_required}")
    endif()
endforeach()
set(_stage "${BINARY_ROOT}/objects-package")
file(REMOVE_RECURSE "${_stage}")
function(run_checked _label)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
    file(WRITE "${BINARY_ROOT}/objects-package-${_label}.log" "${_stdout}\n${_stderr}")
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "${_label} failed (${_result}): ${_stdout}\n${_stderr}")
    endif()
endfunction()
run_checked(install "${CMAKE_COMMAND}" --install "${BINARY_ROOT}" --prefix "${_stage}/install")
foreach(_file include/iiGraphicAdjust/Objects.h include/iiGraphicAdjust/Operation.h
        include/iiGraphicAdjust/CorrectionExtensions.h
        include/iiGraphicAdjust/objects/OwnedResourceObjects.h
        include/iiGraphicAdjust/objects/DevelopFeatureObjects.h
        share/iiGraphicAdjust/docs/OperationObjects.md
        share/iiGraphicAdjust/docs/object-catalog.json
        share/iiGraphicAdjust/docs/adobe-feature-coverage.json)
    if(NOT EXISTS "${_stage}/install/${_file}")
        message(FATAL_ERROR "Missing installed object contract: ${_file}")
    endif()
endforeach()
run_checked(configure "${CMAKE_COMMAND}" -S "${SOURCE_ROOT}/tests/consumer" -B "${_stage}/build"
    "-DCMAKE_PREFIX_PATH=${_stage}/install" "-DCMAKE_DISABLE_FIND_PACKAGE_iiFileProviderNative=TRUE"
    "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}" "-DCMAKE_CXX_FLAGS=${CXX_FLAGS}"
    "-DCMAKE_BUILD_TYPE=Release")
run_checked(build "${CMAKE_COMMAND}" --build "${_stage}/build" --parallel 2)
run_checked(test "${CTEST_COMMAND}" --test-dir "${_stage}/build" --output-on-failure)
message(STATUS "Installed processing and 309-object consumers passed")
