if(NOT SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()
if(EXISTS "${SOURCE_ROOT}/src/iiGraphicAdjust")
    message(FATAL_ERROR "The source tree must not repeat the project name")
endif()
foreach(_file Core/Image.h Core/Image.cpp Core/Operation.h Core/Operation.cpp
        Core/Objects.h Core/ObjectCatalog.cpp Core/objects/DevelopObjects.h
        Development/Develop.h Development/Develop.cpp Development/Tone.h Development/Tone.cpp
        Masks/Masks.h Masks/Masks.cpp Raw/RawProcessing.h Raw/RawProcessing.cpp
        Filters/BlurFilters.h Filters/BlurFilters.cpp Filters/RetouchFilters.h Filters/RetouchFilters.cpp)
    if(NOT EXISTS "${SOURCE_ROOT}/src/${_file}")
        message(FATAL_ERROR "Missing source in its responsibility directory: ${_file}")
    endif()
endforeach()
