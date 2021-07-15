#include "Precompiled.h"
#include "Shader.h"

#include "Graphics/RHI/GraphicsContext.h"
#include "Core/VFS.h"

#include <spirv_cross.hpp>

namespace Lumos
{
    namespace Graphics
    {
        Shader* (*Shader::CreateFunc)(const std::string&) = nullptr;

        const Shader* Shader::s_CurrentlyBound = nullptr;

        Shader* Shader::CreateFromFile(const std::string& filepath)
        {
            LUMOS_ASSERT(CreateFunc, "No Shader Create Function");
            return CreateFunc(filepath);
        }

        ShaderDataType Shader::SPIRVTypeToLumosDataType(const spirv_cross::SPIRType type)
        {
            switch(type.basetype)
            {
            case spirv_cross::SPIRType::Boolean:
                return ShaderDataType::BOOL;
            case spirv_cross::SPIRType::Int:
                if(type.vecsize == 1)
                    return ShaderDataType::INT;
                if(type.vecsize == 2)
                    return ShaderDataType::IVEC2;
                if(type.vecsize == 3)
                    return ShaderDataType::IVEC3;
                if(type.vecsize == 4)
                    return ShaderDataType::IVEC4;

            case spirv_cross::SPIRType::UInt:
                return ShaderDataType::UINT;
            case spirv_cross::SPIRType::Float:
                if(type.columns == 3)
                    return ShaderDataType::MAT3;
                if(type.columns == 4)
                    return ShaderDataType::MAT4;

                if(type.vecsize == 1)
                    return ShaderDataType::FLOAT32;
                if(type.vecsize == 2)
                    return ShaderDataType::VEC2;
                if(type.vecsize == 3)
                    return ShaderDataType::VEC3;
                if(type.vecsize == 4)
                    return ShaderDataType::VEC4;
                break;
            }
            LUMOS_LOG_WARN("Unknown spirv type!");
            return ShaderDataType::NONE;
        }

    }
}
