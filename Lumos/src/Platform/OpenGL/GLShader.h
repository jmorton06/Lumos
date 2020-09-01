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
				: shader(0){};
			u32 shader;
			std::string message[6];
			u32 line[6];
		};

		class GLShader : public Shader
		{
		private:
			friend class Shader;
			friend class ShaderManager;

		private:
			u32 m_Handle;
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

			void* GetHandle() const override
			{
				return (void*)(size_t)m_Handle;
			}

		public:
			GLShader(const std::string& name, const std::string& source, bool loadSPV = false);

			~GLShader();

			void Init();
			void Shutdown() const;
			void Bind() const override;
			void Unbind() const override;

			void SetUserUniformBuffer(ShaderType type, u8* data, u32 size);
			void SetUniform(const std::string& name, u8* data);
			void ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, u8* data, i32 offset, u32 count) const;

			_FORCE_INLINE_ const std::string& GetName() const override
			{
				return m_Name;
			}
			_FORCE_INLINE_ const std::string& GetFilePath() const override
			{
				return m_Path;
			}

			_FORCE_INLINE_ const std::vector<ShaderType> GetShaderTypes() const override
			{
				return m_ShaderTypes;
			}
			_FORCE_INLINE_ const ShaderResourceList& GetResources() const
			{
				return m_Resources;
			}

			static GLuint CompileShader(ShaderType type, std::string source, u32 program, GLShaderErrorInfo& info);
			static u32 Compile(std::map<ShaderType, std::string>* sources, GLShaderErrorInfo& info);
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
			i32 GetUniformLocation(const std::string& name) const;
			u32 GetHandleInternal() const
			{
				return m_Handle;
			}
			static ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name, const ShaderUniformBufferDeclaration* buffer);
			ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);

			void ResolveAndSetUniforms(ShaderUniformBufferDeclaration* buffer, u8* data, u32 size) const;
			void ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, u8* data, u32 size, u32 count) const;

			void SetUniformStruct(GLShaderUniformDeclaration* uniform, u8* data, i32 offset) const;

			void SetUniform1f(const std::string& name, float value) const;
			void SetUniform1fv(const std::string& name, float* value, i32 count) const;
			void SetUniform1i(const std::string& name, i32 value) const;
			void SetUniform1ui(const std::string& name, u32 value) const;
			void SetUniform1iv(const std::string& name, i32* value, i32 count) const;
			void SetUniform2f(const std::string& name, const Maths::Vector2& vector) const;
			void SetUniform3f(const std::string& name, const Maths::Vector3& vector) const;
			void SetUniform4f(const std::string& name, const Maths::Vector4& vector) const;
			void SetUniformMat4(const std::string& name, const Maths::Matrix4& matrix) const;

			void BindUniformBuffer(GLUniformBuffer* buffer, u32 slot, const std::string& name);

			static void SetUniform1f(u32 location, float value);
			static void SetUniform1fv(u32 location, float* value, i32 count);
			static void SetUniform1i(u32 location, i32 value);
			static void SetUniform1ui(u32 location, u32 value);
			static void SetUniform1iv(u32 location, i32* value, i32 count);
			static void SetUniform2f(u32 location, const Maths::Vector2& vector);
			static void SetUniform3f(u32 location, const Maths::Vector3& vector);
			static void SetUniform4f(u32 location, const Maths::Vector4& vector);
			static void SetUniformMat3(u32 location, const Maths::Matrix3& matrix);
			static void SetUniformMat4(u32 location, const Maths::Matrix4& matrix);
			static void SetUniformMat4Array(u32 location, u32 count, const Maths::Matrix4& matrix);

			static void MakeDefault();

		protected:
			static Shader* CreateFuncGL(const std::string& name, const std::string& filePath);

		public:
			static bool TryCompile(const std::string& source, std::string& error);
			static bool TryCompileFromFile(const std::string& filepath, std::string& error);
		};
	}
}
