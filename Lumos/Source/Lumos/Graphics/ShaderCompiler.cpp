#include "Precompiled.h"
#include "ShaderCompiler.h"

#ifdef LUMOS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

namespace Lumos
{
    namespace Graphics
    {
        bool ShaderCompiler::IsAvailable()
        {
#ifdef LUMOS_SHADERC
            return true;
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
            options.SetOptimizationLevel(shaderc_optimization_level_performance);

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
#else
            result.error = "shaderc not available on this platform";
#endif
            return result;
        }
    }
}
