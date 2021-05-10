#pragma once

#include "Graphics/API/Shader.h"
#include "GLDebug.h"
#include "GLShaderUniform.h"
#include "GLShaderResource.h"
#include "GLUniformBuffer.h"

#include <spirv_glsl.hpp>

namespace Lumos
{
    namespace Graphics
    {
        struct GLShaderErrorInfo
        {
            GLShaderErrorInfo()
                : shader(0) {};
            uint32_t shader;
            std::string message[6];
            uint32_t line[6];
        };

        class GLShader : public Shader
        {
        private:
            friend class Shader;
            friend class ShaderManager;

        private:
            uint32_t m_Handle;
            std::string m_Name, m_Path;
            std::string m_Source;

            UnorderedMap<ShaderType, ShaderUniformBufferList> m_UniformBuffers;
            UnorderedMap<ShaderType, GLShaderUniformBufferDeclaration*> m_UserUniformBuffers;

            std::vector<ShaderType> m_ShaderTypes;

            ShaderResourceList m_Resources;
            ShaderStructList m_Structs;
            bool m_LoadSPV = false;

            bool CreateLocations();
            bool SetUniformLocation(const char* szName);

            std::map<uint32_t, std::string> m_names;
            std::map<uint32_t, uint32_t> m_uniformBlockLocations;
            std::map<uint32_t, uint32_t> m_sampledImageLocations;
            std::vector<spirv_cross::CompilerGLSL*> m_pShaderCompilers;
            std::vector<PushConstant> m_PushConstants;
            Graphics::BufferLayout m_Layout;

            void* GetHandle() const override
            {
                return (void*)(size_t)m_Handle;
            }

        public:
            GLShader(const std::string& filePath, bool loadSPV = false);

            ~GLShader();

            void Init();
            void Shutdown() const;
            void Bind() const override;
            void Unbind() const override;

            void SetUserUniformBuffer(ShaderType type, uint8_t* data, uint32_t size);
            void SetUniform(const std::string& name, uint8_t* data);
            void ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, uint8_t* data, int32_t offset, uint32_t count) const;

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilePath() const override
            {
                return m_Path;
            }

            inline const std::vector<ShaderType> GetShaderTypes() const override
            {
                return m_ShaderTypes;
            }
            inline const ShaderResourceList& GetResources() const
            {
                return m_Resources;
            }

            static GLuint CompileShader(ShaderType type, std::string source, uint32_t program, GLShaderErrorInfo& info);
            static uint32_t Compile(std::map<ShaderType, std::string>* sources, GLShaderErrorInfo& info);
            static void PreProcess(const std::string& source, std::map<ShaderType, std::string>* sources);
            static void ReadShaderFile(std::vector<std::string> lines, std::map<ShaderType, std::string>* shaders);

            void Parse(std::map<ShaderType, std::string>* sources);
            void ParseUniform(const std::string& statement, ShaderType type);
            void ParseUniformStruct(const std::string& block, ShaderType shaderType);

            static bool IsTypeStringResource(const std::string& type);

            ShaderStruct* FindStruct(const std::string& name);

            void ResolveUniforms();
            static void ValidateUniforms();
            static bool IsSystemUniform(ShaderUniformDeclaration* uniform);
            int32_t GetUniformLocation(const std::string& name) const;
            uint32_t GetHandleInternal() const
            {
                return m_Handle;
            }
            const Graphics::BufferLayout& GetBufferLayout() const { return m_Layout; }

            static ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name, const ShaderUniformBufferDeclaration* buffer);
            ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);

            void ResolveAndSetUniforms(ShaderUniformBufferDeclaration* buffer, uint8_t* data, uint32_t size) const;
            void ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, uint8_t* data, uint32_t size, uint32_t count) const;

            void SetUniformStruct(GLShaderUniformDeclaration* uniform, uint8_t* data, int32_t offset) const;

            void SetUniform1f(const std::string& name, float value) const;
            void SetUniform1fv(const std::string& name, float* value, int32_t count) const;
            void SetUniform1i(const std::string& name, int32_t value) const;
            void SetUniform1ui(const std::string& name, uint32_t value) const;
            void SetUniform1iv(const std::string& name, int32_t* value, int32_t count) const;
            void SetUniform2f(const std::string& name, const Maths::Vector2& vector) const;
            void SetUniform3f(const std::string& name, const Maths::Vector3& vector) const;
            void SetUniform4f(const std::string& name, const Maths::Vector4& vector) const;
            void SetUniformMat4(const std::string& name, const Maths::Matrix4& matrix) const;

            void BindUniformBuffer(GLUniformBuffer* buffer, uint32_t slot, const std::string& name);

            PushConstant* GetPushConstant(uint32_t index) override
            {
                LUMOS_ASSERT(index < m_PushConstants.size(), "Push constants out of bounds");
                return &m_PushConstants[index];
            }
            std::vector<PushConstant>& GetPushConstants() override { return m_PushConstants; }
            void BindPushConstants(Graphics::CommandBuffer* cmdBuffer, Graphics::Pipeline* pipeline) override;

            static void SetUniform1f(uint32_t location, float value);
            static void SetUniform1fv(uint32_t location, float* value, int32_t count);
            static void SetUniform1i(uint32_t location, int32_t value);
            static void SetUniform1ui(uint32_t location, uint32_t value);
            static void SetUniform1iv(uint32_t location, int32_t* value, int32_t count);
            static void SetUniform2f(uint32_t location, const Maths::Vector2& vector);
            static void SetUniform3f(uint32_t location, const Maths::Vector3& vector);
            static void SetUniform4f(uint32_t location, const Maths::Vector4& vector);
            static void SetUniformMat3(uint32_t location, const Maths::Matrix3& matrix);
            static void SetUniformMat4(uint32_t location, const Maths::Matrix4& matrix);
            static void SetUniformMat4Array(uint32_t location, uint32_t count, const Maths::Matrix4& matrix);

            static void MakeDefault();

        protected:
            static Shader* CreateFuncGL(const std::string& filePath);
        };
    }
}
