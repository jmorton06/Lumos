
project "SpirvCross"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    files
    {
        "vulkan/SPIRV-Cross/spirv.h",
        "vulkan/SPIRV-Cross/spirv.hpp",
        "vulkan/SPIRV-Cross/spirv_cfg.cpp",
        "vulkan/SPIRV-Cross/spirv_cfg.hpp",
        "vulkan/SPIRV-Cross/spirv_common.hpp",
        "vulkan/SPIRV-Cross/spirv_cpp.cpp",
        "vulkan/SPIRV-Cross/spirv_cpp.hpp",
        "vulkan/SPIRV-Cross/spirv_cross.cpp",
        "vulkan/SPIRV-Cross/spirv_cross.hpp",
        "vulkan/SPIRV-Cross/spirv_cross_c.cpp",
        "vulkan/SPIRV-Cross/spirv_cross_c.h",
        "vulkan/SPIRV-Cross/spirv_cross_containers.hpp",
        "vulkan/SPIRV-Cross/spirv_cross_error_handling.hpp",
        "vulkan/SPIRV-Cross/spirv_cross_parsed_ir.cpp",
        "vulkan/SPIRV-Cross/spirv_cross_parsed_ir.hpp",
        "vulkan/SPIRV-Cross/spirv_cross_util.cpp",
        "vulkan/SPIRV-Cross/spirv_cross_util.hpp",
        "vulkan/SPIRV-Cross/spirv_glsl.cpp",
        "vulkan/SPIRV-Cross/spirv_glsl.hpp",
        "vulkan/SPIRV-Cross/spirv_hlsl.cpp",
        "vulkan/SPIRV-Cross/spirv_hlsl.hpp",
        "vulkan/SPIRV-Cross/spirv_msl.cpp",
        "vulkan/SPIRV-Cross/spirv_msl.hpp",
        "vulkan/SPIRV-Cross/spirv_parser.cpp",
        "vulkan/SPIRV-Cross/spirv_parser.hpp",
        "vulkan/SPIRV-Cross/spirv_reflect.cpp",
        "vulkan/SPIRV-Cross/spirv_reflect.hpp"
    }

    filter "system:windows"
        systemversion "latest"

    filter "system:macosx"
        systemversion "11.0"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter "configurations:Production"
        runtime "Release"
        optimize "On"