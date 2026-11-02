#include "Precompiled.h"
#include "ShaderCompiler.h"

#ifdef LUMOS_SHADERC
#include <shaderc/shaderc.hpp>
#elif defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_LINUX) || defined(LUMOS_PLATFORM_WINDOWS)
// No shaderc linked: shell out to the Vulkan SDK's glslc if it's installed.
#define LUMOS_GLSLC_FALLBACK
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#endif

namespace Lumos
{
    namespace Graphics
    {
#ifdef LUMOS_GLSLC_FALLBACK
        static std::string FindGLSLC()
        {
            static std::string s_Path;
            static bool s_Searched = false;
            if(s_Searched)
                return s_Path;
            s_Searched = true;

            namespace fs = std::filesystem;
            std::error_code ec;
#ifdef LUMOS_PLATFORM_WINDOWS
            const char* exe = "glslc.exe";
#else
            const char* exe = "glslc";
#endif
            if(const char* sdk = std::getenv("VULKAN_SDK"))
            {
                fs::path p = fs::path(sdk) / "bin" / exe;
                if(fs::exists(p, ec))
                    return s_Path = p.string();
            }
            for(const char* dir : { "/usr/local/bin", "/opt/homebrew/bin", "/usr/bin" })
            {
                fs::path p = fs::path(dir) / exe;
                if(fs::exists(p, ec))
                    return s_Path = p.string();
            }
            return s_Path;
        }

        static const char* StageFlag(ShaderType type)
        {
            switch(type)
            {
            case ShaderType::VERTEX:
                return "vert";
            case ShaderType::FRAGMENT:
                return "frag";
            case ShaderType::COMPUTE:
                return "comp";
            case ShaderType::GEOMETRY:
                return "geom";
            case ShaderType::TESSELLATION_CONTROL:
                return "tesc";
            case ShaderType::TESSELLATION_EVALUATION:
                return "tese";
            default:
                return nullptr;
            }
        }
#endif

        bool ShaderCompiler::IsAvailable()
        {
#ifdef LUMOS_SHADERC
            return true;
#elif defined(LUMOS_GLSLC_FALLBACK)
            return !FindGLSLC().empty();
#else
            return false;
#endif
        }

        ShaderCompileResult ShaderCompiler::Compile(const std::string& source, ShaderType type, const std::string& filename)
        {
            ShaderCompileResult result;

#ifdef LUMOS_SHADERC
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
            options.SetOptimizationLevel(shaderc_optimization_level_zero);

            shaderc_shader_kind kind;
            switch(type)
            {
            case ShaderType::VERTEX:
                kind = shaderc_vertex_shader;
                break;
            case ShaderType::FRAGMENT:
                kind = shaderc_fragment_shader;
                break;
            case ShaderType::COMPUTE:
                kind = shaderc_compute_shader;
                break;
            case ShaderType::GEOMETRY:
                kind = shaderc_geometry_shader;
                break;
            case ShaderType::TESSELLATION_CONTROL:
                kind = shaderc_tess_control_shader;
                break;
            case ShaderType::TESSELLATION_EVALUATION:
                kind = shaderc_tess_evaluation_shader;
                break;
            default:
                result.error = "Unknown shader type";
                return result;
            }

            auto module = compiler.CompileGlslToSpv(source, kind, filename.c_str(), options);

            if(module.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                result.error = module.GetErrorMessage();
                return result;
            }

            result.spirv = { module.cbegin(), module.cend() };
            result.success = true;
#elif defined(LUMOS_GLSLC_FALLBACK)
            std::string glslc = FindGLSLC();
            const char* stage = StageFlag(type);
            if(glslc.empty() || !stage)
            {
                result.error = "No shader compiler: build with --shaderc or install the Vulkan SDK (glslc)";
                return result;
            }

            namespace fs = std::filesystem;
            std::error_code ec;
            static u32 s_Counter = 0;
            fs::path dir     = fs::temp_directory_path(ec);
            std::string base = (dir / ("lumos_shader_" + std::to_string(s_Counter++))).string();
            std::string src = base + ".glsl", spv = base + ".spv", log = base + ".log";

            if(FILE* f = fopen(src.c_str(), "wb"))
            {
                fwrite(source.data(), 1, source.size(), f);
                fclose(f);
            }
            else
            {
                result.error = "Couldn't write temp shader file";
                return result;
            }

            // Never pass -O: it strips bindings the reflection relies on
            std::string cmd = "\"" + glslc + "\" -fshader-stage=" + stage + " --target-env=vulkan1.1 \"" + src + "\" -o \"" + spv + "\" 2> \"" + log + "\"";
#ifdef LUMOS_PLATFORM_WINDOWS
            cmd = "\"" + cmd + "\""; // cmd.exe strips the outer quotes
#endif
            int rc = std::system(cmd.c_str());

            if(rc == 0)
            {
                if(FILE* f = fopen(spv.c_str(), "rb"))
                {
                    fseek(f, 0, SEEK_END);
                    long size = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    result.spirv.resize(size / 4);
                    result.success = size > 0 && fread(result.spirv.data(), 4, size / 4, f) == (size_t)(size / 4);
                    fclose(f);
                }
            }
            if(!result.success)
            {
                if(FILE* f = fopen(log.c_str(), "rb"))
                {
                    char buf[4096];
                    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
                    buf[n]   = 0;
                    fclose(f);
                    // glslc prefixes each line with the temp path - swap it for the caller's name
                    std::string msg = buf;
                    for(size_t at = msg.find(src); at != std::string::npos; at = msg.find(src, at))
                        msg.replace(at, src.size(), filename);
                    result.error = msg.empty() ? "glslc failed" : msg;
                }
                else
                    result.error = "glslc failed";
            }
            fs::remove(src, ec);
            fs::remove(spv, ec);
            fs::remove(log, ec);
#else
            result.error = "shaderc not available on this platform";
#endif
            return result;
        }
    }
}
