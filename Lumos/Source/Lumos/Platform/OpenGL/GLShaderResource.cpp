#include "Precompiled.h"
#include "GLShaderResource.h"

namespace Lumos
{
    namespace Graphics
    {
        GLShaderResourceDeclaration::GLShaderResourceDeclaration(Type type, const std::string& name, uint32_t count)
            : m_Name(name)
            , m_Register(0)
            , m_Count(count)
            , m_Type(type)
        {
            m_Name = name;
            m_Count = count;
        }

        GLShaderResourceDeclaration::Type GLShaderResourceDeclaration::StringToType(const std::string& type)
        {
            if(type == "sampler2D")
                return Type::TEXTURE2D;
            if(type == "samplerCube")
                return Type::TEXTURECUBE;
            if(type == "samplerShadow")
                return Type::TEXTURESHADOW;
            if(type == "sampler2DArrayShadow")
                return Type::TEXTURESHADOWARRAY;

            return Type::NONE;
        }

        std::string GLShaderResourceDeclaration::TypeToString(Type type)
        {
            switch(type)
            {
            case Type::TEXTURE2D:
                return "sampler2D";
            case Type::TEXTURECUBE:
                return "samplerCube";
            case Type::TEXTURESHADOW:
                return "samplerShadow";
            case Type::TEXTURESHADOWARRAY:
                return "sampler2DArrayShadow";
            default:
                return "Invalid Type";
            }
        }
    }
}
