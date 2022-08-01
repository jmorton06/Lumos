
cmake_minimum_required(VERSION 3.10)

project(msdf-atlas-gen VERSION 1.2 LANGUAGES CXX)
option(MSDF_ATLAS_GEN_BUILD_STANDALONE "Build the msdf-atlas-gen standalone executable" ON)
set(MSDFGEN_BUILD_MSDFGEN_STANDALONE OFF CACHE BOOL "Build the msdfgen standalone executable")
set(MSDFGEN_USE_OPENMP OFF CACHE INTERNAL "Build with OpenMP support for multithreaded code (disabled for atlas gen)" FORCE)
set(MSDFGEN_USE_CPP11 ON CACHE INTERNAL "Build with C++11 enabled (always enabled for atlas gen)" FORCE)
set(MSDFGEN_INSTALL OFF CACHE BOOL "Generate installation target for msdfgen")
add_subdirectory(msdfgen)

find_package(Threads REQUIRED)

file(GLOB_RECURSE msdf-atlas-gen_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "msdf-atlas-gen/*.h"
    "msdf-atlas-gen/*.hpp"
)

file(GLOB_RECURSE msdf-atlas-gen_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "msdf-atlas-gen/*.cpp"
)

# msdf-atlas-gen library
add_library(msdf-atlas-gen ${msdf-atlas-gen_SOURCES} ${msdf-atlas-gen_HEADERS})
add_library(msdf-atlas-gen::msdf-atlas-gen ALIAS msdf-atlas-gen)
set_target_properties(msdf-atlas-gen PROPERTIES PUBLIC_HEADER "${msdf-atlas-gen_HEADERS}")
target_include_directories(msdf-atlas-gen
INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/msdfgen/include # for lodepng.h
    ${CMAKE_CURRENT_SOURCE_DIR}/artery-font-format
)

if (MSVC)
    target_compile_definitions(msdf-atlas-gen PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()
target_compile_features(msdf-atlas-gen PUBLIC cxx_std_11)
target_link_libraries(msdf-atlas-gen PUBLIC Threads::Threads msdfgen::msdfgen msdfgen::msdfgen-ext)

# TODO make these public in msdfgen so that this doesn't have to be repeated here
if(FREETYPE_WITH_PNG)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    target_link_libraries(msdf-atlas-gen PUBLIC ZLIB::ZLIB PNG::PNG)
endif()
if(FREETYPE_WITH_HARFBUZZ)
    # No FindHarfBuzz.cmake available so falling back to CMAKE_LIBRARY_PATH
    find_library(FREETYPE_HARFBUZZ_LIB "harfbuzz")
    target_link_libraries(msdf-atlas-gen PUBLIC "${FREETYPE_HARFBUZZ_LIB}")
endif()

# msdf-atlas-gen standalone executable
if(MSDF_ATLAS_GEN_BUILD_STANDALONE)
    add_executable(msdf-atlas-gen-standalone msdf-atlas-gen/main.cpp)
    set_target_properties(msdf-atlas-gen-standalone PROPERTIES ARCHIVE_OUTPUT_DIRECTORY archive OUTPUT_NAME msdf-atlas-gen)
    target_compile_definitions(msdf-atlas-gen-standalone PUBLIC MSDF_ATLAS_STANDALONE)
    target_compile_features(msdf-atlas-gen-standalone PUBLIC cxx_std_11)
    target_link_libraries(msdf-atlas-gen-standalone PUBLIC msdf-atlas-gen::msdf-atlas-gen)
endif()
