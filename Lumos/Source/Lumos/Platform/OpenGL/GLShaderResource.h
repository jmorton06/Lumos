#pragma once

#include "Graphics/API/ShaderResource.h"
#include "GLShaderUniform.h"
#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLShaderResourceDeclaration : public ShaderResourceDeclaration
        {
        public:
            enum class Type
            {
                NONE,
                TEXTURE2D,
                TEXTURECUBE,
                TEXTURESHADOW,
                TEXTURESHADOWARRAY
            };

        private:
            friend class GLShader;

        private:
            std::string m_Name;
            uint32_t m_Register;
            uint32_t m_Count;
            Type m_Type;

        public:
            GLShaderResourceDeclaration(Type type, const std::string& name, uint32_t count);

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline uint32_t GetRegister() const override
            {
                return m_Register;
            }
            inline uint32_t GetCount() const override
            {
                return m_Count;
            }

            inline Type GetType() const
            {
                return m_Type;
            }

        public:
            static Type StringToType(const std::string& type);
            static std::string TypeToString(Type type);
        };
    }
}
