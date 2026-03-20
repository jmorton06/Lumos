#pragma once
#include "Core/Core.h"
#include "Graphics/RHI/RHIDefinitions.h"
#include <string>
#include <vector>

namespace Lumos
{
    namespace Graphics
    {
        struct ShaderCompileResult
        {
            bool success = false;
            std::string error;
            std::vector<uint32_t> spirv;
        };

        class ShaderCompiler
        {
        public:
            static ShaderCompileResult Compile(const std::string& source, ShaderType type, const std::string& filename = "shader");
            static bool IsAvailable();
        };
    }
}
