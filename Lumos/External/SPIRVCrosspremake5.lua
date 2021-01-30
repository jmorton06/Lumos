
project "SpirvCross"
    kind "StaticLib"
    language "C++"
    systemversion "latest"
    cppdialect "C++17"
    staticruntime "On"
    files
    {
        "SPIRV-Cross/spirv.h",
        "SPIRV-Cross/spirv.hpp",
        "SPIRV-Cross/spirv_cfg.cpp",
        "SPIRV-Cross/spirv_cfg.hpp",
        "SPIRV-Cross/spirv_common.hpp",
        "SPIRV-Cross/spirv_cpp.cpp",
        "SPIRV-Cross/spirv_cpp.hpp",
        "SPIRV-Cross/spirv_cross.cpp",
        "SPIRV-Cross/spirv_cross.hpp",
        "SPIRV-Cross/spirv_cross_c.cpp",
        "SPIRV-Cross/spirv_cross_c.h",
        "SPIRV-Cross/spirv_cross_containers.hpp",
        "SPIRV-Cross/spirv_cross_error_handling.hpp",
        "SPIRV-Cross/spirv_cross_parsed_ir.cpp",
        "SPIRV-Cross/spirv_cross_parsed_ir.hpp",
        "SPIRV-Cross/spirv_cross_util.cpp",
        "SPIRV-Cross/spirv_cross_util.hpp",
        "SPIRV-Cross/spirv_glsl.cpp",
        "SPIRV-Cross/spirv_glsl.hpp",
        "SPIRV-Cross/spirv_hlsl.cpp",
        "SPIRV-Cross/spirv_hlsl.hpp",
        "SPIRV-Cross/spirv_msl.cpp",
        "SPIRV-Cross/spirv_msl.hpp",
        "SPIRV-Cross/spirv_parser.cpp",
        "SPIRV-Cross/spirv_parser.hpp",
        "SPIRV-Cross/spirv_reflect.cpp",
        "SPIRV-Cross/spirv_reflect.hpp"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter "configurations:Production"
        runtime "Release"
        optimize "On"