#pragma once

#include "Graphics/API/ShaderUniform.h"
#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLShaderUniformDeclaration : public ShaderUniformDeclaration
        {
        private:
            friend class GLShader;
            friend class GLShaderUniformBufferDeclaration;

        public:
            enum class Type
            {
                NONE,
                FLOAT32,
                VEC2,
                VEC3,
                VEC4,
                MAT3,
                MAT4,
                INT32,
                INT,
                UINT,
                STRUCT,
                MAT4ARRAY
            };

        private:
            std::string m_Name;
            uint32_t m_Size;
            uint32_t m_Count;
            uint32_t m_Offset;

            Type m_Type;
            ShaderStruct* m_Struct;
            mutable int32_t m_Location;

        public:
            GLShaderUniformDeclaration(Type type, const std::string& name, uint32_t count = 1);
            GLShaderUniformDeclaration(ShaderStruct* uniformStruct, const std::string& name, uint32_t count = 1);

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline void SetName(const std::string& name) override
            {
                m_Name = name;
            }
            inline uint32_t GetSize() const override
            {
                return m_Size;
            }
            inline uint32_t GetCount() const override
            {
                return m_Count;
            }
            inline uint32_t GetOffset() const override
            {
                return m_Offset;
            }
            inline uint32_t GetAbsoluteOffset() const
            {
                return m_Struct ? m_Struct->GetOffset() + m_Offset : m_Offset;
            }

            int32_t GetLocation() const
            {
                return m_Location;
            }
            inline Type GetType() const
            {
                return m_Type;
            }
            inline const ShaderStruct& GetShaderUniformStruct() const
            {
                LUMOS_ASSERT(m_Struct, "");
                return *m_Struct;
            }

        protected:
            void SetOffset(uint32_t offset) override;

        public:
            static uint32_t SizeOfUniformType(Type type);
            static Type StringToType(const std::string& type, uint32_t count);
            static std::string TypeToString(Type type);
        };

        struct LUMOS_EXPORT GLShaderUniformField
        {
            GLShaderUniformDeclaration::Type type;
            std::string name;
            uint32_t count;
            mutable uint32_t size;
            mutable int32_t location;
        };

        // TODO: Eventually support OpenGL uniform buffers. This is more platform-side.
        class GLShaderUniformBufferDeclaration : public ShaderUniformBufferDeclaration
        {
        private:
            friend class Shader;

        private:
            std::string m_Name;
            ShaderUniformList m_Uniforms;
            uint32_t m_Register;
            uint32_t m_Size;
            uint32_t m_ShaderType; // 0 = VS, 1 = PS, 2 = GS
        public:
            GLShaderUniformBufferDeclaration(const std::string& name, uint32_t shaderType);

            void PushUniform(GLShaderUniformDeclaration* uniform);

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline uint32_t GetRegister() const override
            {
                return m_Register;
            }
            inline uint32_t GetShaderType() const override
            {
                return m_ShaderType;
            }
            inline uint32_t GetSize() const override
            {
                return m_Size;
            }
            inline const ShaderUniformList& GetUniformDeclarations() const override
            {
                return m_Uniforms;
            }

            ShaderUniformDeclaration* FindUniform(const std::string& name) override;
            ~GLShaderUniformBufferDeclaration() override;

            static void MakeDefault();

        protected:
            static CommandBuffer* CreateFuncGL();
        };
    }
}
