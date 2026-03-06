
project "shaderc"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"

    local shaderc_root = "shaderc/"
    local glslang_root = shaderc_root .. "third_party/glslang/"
    local spirvtools_root = shaderc_root .. "third_party/spirv-tools/"
    local spirvheaders_root = shaderc_root .. "third_party/spirv-headers/"

    files
    {
        -- libshaderc
        shaderc_root .. "libshaderc/src/shaderc.cc",

        -- libshaderc_util
        shaderc_root .. "libshaderc_util/src/args.cc",
        shaderc_root .. "libshaderc_util/src/compiler.cc",
        shaderc_root .. "libshaderc_util/src/file_finder.cc",
        shaderc_root .. "libshaderc_util/src/io_shaderc.cc",
        shaderc_root .. "libshaderc_util/src/message.cc",
        shaderc_root .. "libshaderc_util/src/resources.cc",
        shaderc_root .. "libshaderc_util/src/shader_stage.cc",
        shaderc_root .. "libshaderc_util/src/spirv_tools_wrapper.cc",
        shaderc_root .. "libshaderc_util/src/version_profile.cc",

        -- glslang: MachineIndependent
        glslang_root .. "glslang/MachineIndependent/*.cpp",
        glslang_root .. "glslang/MachineIndependent/preprocessor/*.cpp",

        -- glslang: GenericCodeGen
        glslang_root .. "glslang/GenericCodeGen/*.cpp",

        -- glslang: ResourceLimits
        glslang_root .. "glslang/ResourceLimits/*.cpp",

        -- glslang: HLSL
        glslang_root .. "glslang/HLSL/*.cpp",

        -- glslang: SPIRV backend
        glslang_root .. "SPIRV/*.cpp",

        -- SPIRV-Tools
        spirvtools_root .. "source/*.cpp",
        spirvtools_root .. "source/util/*.cpp",
        spirvtools_root .. "source/val/*.cpp",
        spirvtools_root .. "source/opt/*.cpp",
        spirvtools_root .. "source/link/*.cpp",
        spirvtools_root .. "source/diff/*.cpp",
        spirvtools_root .. "source/lint/*.cpp",
    }

    removefiles
    {
        -- Platform-specific: remove all, re-add per platform below
        glslang_root .. "glslang/OSDependent/Unix/ossource.cpp",
        glslang_root .. "glslang/OSDependent/Windows/ossource.cpp",
        glslang_root .. "glslang/OSDependent/Web/glslang.js.cpp",
        glslang_root .. "glslang/stub.cpp",
        -- mimalloc optional allocator (requires separate mimalloc dep)
        spirvtools_root .. "source/mimalloc.cpp",
    }

    includedirs
    {
        -- shaderc
        shaderc_root .. "libshaderc/include",
        shaderc_root .. "libshaderc/src",
        shaderc_root .. "libshaderc_util/include",

        -- glslang
        glslang_root,

        -- SPIRV-Tools
        spirvtools_root,
        spirvtools_root .. "include",
        spirvtools_root .. "source",

        -- SPIRV-Headers
        spirvheaders_root .. "include",

        -- Generated files
        shaderc_root .. "generated/spirv-tools",
        shaderc_root .. "generated",
    }

    defines
    {
        "ENABLE_HLSL",
    }

    filter "system:windows"
        systemversion "latest"
        files { glslang_root .. "glslang/OSDependent/Windows/ossource.cpp" }

    filter "system:macosx"
        systemversion "11.0"
        files { glslang_root .. "glslang/OSDependent/Unix/ossource.cpp" }

    filter "system:linux"
        files { glslang_root .. "glslang/OSDependent/Unix/ossource.cpp" }

    filter "system:ios"
        files { glslang_root .. "glslang/OSDependent/Unix/ossource.cpp" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter "configurations:Production"
        runtime "Release"
        optimize "On"
