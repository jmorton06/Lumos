project "freetype"
	kind "StaticLib"
	language "C"
    staticruntime "on"
    warnings "off"

	files {
        "include/config/ftconfig.h",
       "include/config/ftheader.h",
       "include/config/ftmodule.h",
       "include/config/ftoption.h",
       "include/config/ftstdlib.h",
       "include/ft2build.h",

       "src/autofit/autofit.c",
       "src/base/ftbase.c",
       "src/base/ftbbox.c",
       "src/base/ftbitmap.c",
       "src/base/ftdebug.c",
       "src/base/ftfstype.c",
       "src/base/ftgasp.c",
       "src/base/ftglyph.c",
       "src/base/ftgxval.c",
       "src/base/ftinit.c",
       "src/base/ftlcdfil.c",
       "src/base/ftmm.c",
       "src/base/ftotval.c",
       "src/base/ftpatent.c",
       "src/base/ftpfr.c",
       "src/base/ftstroke.c",
       "src/base/ftsynth.c",
       "src/base/ftsystem.c",
       "src/base/fttype1.c",
       "src/base/ftwinfnt.c",
       "src/bdf/bdf.c",
       "src/cache/ftcache.c",
       "src/cff/cff.c",
       "src/cid/type1cid.c",
       "src/gzip/ftgzip.c",
       "src/lzw/ftlzw.c",
       "src/pcf/pcf.c",
       "src/pfr/pfr.c",
       "src/psaux/psaux.c",
       "src/pshinter/pshinter.c",
       "src/psnames/psmodule.c",
       "src/raster/raster.c",
       "src/sfnt/sfnt.c",
       "src/smooth/smooth.c",
       "src/truetype/truetype.c",
       "src/type1/type1.c",
       "src/type42/type42.c",
       "src/winfonts/winfnt.c"
   }

    buildoptions
    {
        "-w"
    }

   defines {
       "FT2_BUILD_LIBRARY",
   }

   externalincludedirs {
		"include"
    }
    
    includedirs {
		"include"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter "configurations:Production"
        runtime "Release"
        optimize "On"