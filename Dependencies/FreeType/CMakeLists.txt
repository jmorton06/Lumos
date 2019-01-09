cmake_minimum_required(VERSION 2.6)
project(FREETYPE)


##########################################################################################
# XMake-related settings

# Set the output paths
if (NOT DEFINED XMAKE_BINARY_DIR)
    set(XMAKE_BINARY_DIR "${FREETYPE_BINARY_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${XMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${XMAKE_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${XMAKE_BINARY_DIR}/bin")
endif()

# Set the dependencies path
if (NOT DEFINED XMAKE_DEPENDENCIES_DIR)
    set(XMAKE_DEPENDENCIES_DIR "${FREETYPE_SOURCE_DIR}/dependencies")
endif()


##########################################################################################
# XMake importation

if (NOT EXISTS "${XMAKE_DEPENDENCIES_DIR}/XMake/XMake.cmake")
    message(FATAL_ERROR
"Missing dependency: XMake
This distribution of FreeType requires XMake as a dependency.

It is provided as GIT submodule of this repository.

Did you forgot to execute the following commands?
   git submodule init
   git submodule update")
endif()

include("${XMAKE_DEPENDENCIES_DIR}/XMake/XMake.cmake")


##########################################################################################
# Declaration of the library

# List the header files
set(HEADERS include/freetype/config/ftconfig.h
            include/freetype/config/ftheader.h
            include/freetype/config/ftmodule.h
            include/freetype/config/ftoption.h
            include/freetype/config/ftstdlib.h
            include/freetype/freetype.h
            include/freetype/ftadvanc.h
            include/freetype/ftbbox.h
            include/freetype/ftbdf.h
            include/freetype/ftbitmap.h
            include/freetype/ftbzip2.h
            include/freetype/ftcache.h
            include/freetype/ftchapters.h
            include/freetype/ftcid.h
            include/freetype/fterrdef.h
            include/freetype/fterrors.h
            include/freetype/ftgasp.h
            include/freetype/ftglyph.h
            include/freetype/ftgxval.h
            include/freetype/ftgzip.h
            include/freetype/ftimage.h
            include/freetype/ftincrem.h
            include/freetype/ftlcdfil.h
            include/freetype/ftlist.h
            include/freetype/ftlzw.h
            include/freetype/ftmac.h
            include/freetype/ftmm.h
            include/freetype/ftmodapi.h
            include/freetype/ftmoderr.h
            include/freetype/ftotval.h
            include/freetype/ftoutln.h
            include/freetype/ftpfr.h
            include/freetype/ftrender.h
            include/freetype/ftsizes.h
            include/freetype/ftsnames.h
            include/freetype/ftstroke.h
            include/freetype/ftsynth.h
            include/freetype/ftsystem.h
            include/freetype/fttrigon.h
            include/freetype/fttypes.h
            include/freetype/ftwinfnt.h
            include/freetype/ftxf86.h
            include/freetype/t1tables.h
            include/freetype/ttnameid.h
            include/freetype/tttables.h
            include/freetype/tttags.h
            include/freetype/ttunpat.h
            include/ft2build.h
)

# List the source files
set(SRCS src/autofit/autofit.c
         src/base/ftbase.c
         src/base/ftbbox.c
         src/base/ftbitmap.c
         src/base/ftfstype.c
         src/base/ftgasp.c
         src/base/ftglyph.c
         src/base/ftinit.c
         src/base/ftmm.c
         src/base/ftpfr.c
         src/base/ftstroke.c
         src/base/ftsynth.c
         src/base/ftsystem.c
         src/base/fttype1.c
         src/base/ftwinfnt.c
         src/bdf/bdf.c
         src/bzip2/ftbzip2.c
         src/cache/ftcache.c
         src/cff/cff.c
         src/cid/type1cid.c
         src/gzip/ftgzip.c
         src/lzw/ftlzw.c
         src/pcf/pcf.c
         src/pfr/pfr.c
         src/psaux/psaux.c
         src/pshinter/pshinter.c
         src/psnames/psmodule.c
         src/raster/raster.c
         src/sfnt/sfnt.c
         src/smooth/smooth.c
         src/truetype/truetype.c
         src/type1/type1.c
         src/type42/type42.c
         src/winfonts/winfnt.c
)

# List the include paths
set(INCLUDE_PATHS "${FREETYPE_SOURCE_DIR}/include")

include_directories(${INCLUDE_PATHS})

# Declaration of the library
xmake_create_static_library(FREETYPE freetype ${HEADERS} ${SRCS})

xmake_add_to_list_property(FREETYPE COMPILE_DEFINITIONS "FT2_BUILD_LIBRARY")

if (NOT WIN32)
    xmake_add_to_property(FREETYPE COMPILE_FLAGS "-w")
else()
    xmake_add_to_property(FREETYPE COMPILE_FLAGS "/W0")
    xmake_add_to_list_property(FREETYPE COMPILE_DEFINITIONS "_CRT_SECURE_NO_DEPRECATE")
endif()

# Exportations
xmake_export_include_paths(FREETYPE ${INCLUDE_PATHS})
