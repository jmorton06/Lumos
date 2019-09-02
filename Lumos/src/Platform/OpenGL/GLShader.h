#pragma once
#include "LM.h"
#include "Graphics/API/Shader.h"
#include "GLDebug.h"
#include "GLShaderUniform.h"
#include "GLShaderResource.h"

namespace Lumos
{
	namespace Graphics
	{
		struct GLShaderErrorInfo
		{
			GLShaderErrorInfo() : shader(0)
			{
			};
			u32 shader;
			String message[6];
			u32 line[6];
		};

		class GLShader : public Shader
		{
		private:
			friend class Shader;
			friend class ShaderManager;
		private:
			u32 m_Handle;
			String m_Name, m_Path;
			String m_Source;

			UnorderedMap<ShaderType, ShaderUniformBufferList> m_UniformBuffers;
			UnorderedMap<ShaderType, GLShaderUniformBufferDeclaration*> m_UserUniformBuffers;

			std::vector<ShaderType> m_ShaderTypes;

			ShaderResourceList m_Resources;
			ShaderStructList m_Structs;

		public:
			GLShader(const String& name, const String& source);
			~GLShader();

			void Init();
			void Shutdown() const;
			void Bind() const override;
			void Unbind() const override;

			void SetSystemUniformBuffer(ShaderType type, u8* data, u32 size, u32 slot) override;
			void SetUserUniformBuffer(ShaderType type, u8* data, u32 size) override;

			void SetUniform(const String& name, u8* data);
			void ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, u8* data, i32 offset, u32 count) const;

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilePath() const override { return m_Path; }

			inline const ShaderUniformBufferList GetSystemUniforms(ShaderType type) const override { try { return m_UniformBuffers.at(type); } catch (std::exception) { return ShaderUniformBufferList(); } }
			inline const ShaderUniformBufferDeclaration* GetUserUniformBuffer(ShaderType type) const override { try { return m_UserUniformBuffers.at(type); } catch (std::exception) { return nullptr; } }
			inline const std::vector<ShaderType> GetShaderTypes() const override { return m_ShaderTypes; }
			inline const ShaderResourceList& GetResources() const { return m_Resources; }

			static GLuint CompileShader(ShaderType type, String source, u32 program, GLShaderErrorInfo& info);
			static u32 Compile(std::map<ShaderType, String>* sources, GLShaderErrorInfo& info);
			static void PreProcess(const String& source, std::map<ShaderType, String>* sources);
			static void ReadShaderFile(std::vector<String> lines, std::map<ShaderType, String>* shaders);

			void Parse(std::map<ShaderType, String>* sources);
			void ParseUniform(const String& statement, ShaderType type);
			void ParseUniformStruct(const String& block, ShaderType shaderType);

			static bool IsTypeStringResource(const String& type);

			ShaderStruct* FindStruct(const String& name);

			void ResolveUniforms();
			static void ValidateUniforms();
			static bool IsSystemUniform(ShaderUniformDeclaration* uniform);
			i32 GetUniformLocation(const String& name) const;
			u32 GetHandle() const { return m_Handle; }
			static ShaderUniformDeclaration* FindUniformDeclaration(const String& name, const ShaderUniformBufferDeclaration* buffer);
			ShaderUniformDeclaration* FindUniformDeclaration(const String& name);

			void ResolveAndSetUniforms(ShaderUniformBufferDeclaration* buffer, u8* data, u32 size) const;
			void ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, u8* data, u32 size, u32 count) const;

			void SetUniformStruct(GLShaderUniformDeclaration* uniform, u8* data, i32 offset) const;

			void SetUniform1f(const String& name, float value) const;
			void SetUniform1fv(const String& name, float* value, i32 count) const;
			void SetUniform1i(const String& name, i32 value) const;
			void SetUniform1iv(const String& name, i32* value, i32 count) const;
			void SetUniform2f(const String& name, const Maths::Vector2& vector) const;
			void SetUniform3f(const String& name, const Maths::Vector3& vector) const;
			void SetUniform4f(const String& name, const Maths::Vector4& vector) const;
			void SetUniformMat4(const String& name, const Maths::Matrix4& matrix) const;

			static void SetUniform1f(u32 location, float value);
			static void SetUniform1fv(u32 location, float* value, i32 count);
			static void SetUniform1i(u32 location, i32 value);
			static void SetUniform1iv(u32 location, i32* value, i32 count);
			static void SetUniform2f(u32 location, const Maths::Vector2& vector);
			static void SetUniform3f(u32 location, const Maths::Vector3& vector);
			static void SetUniform4f(u32 location, const Maths::Vector4& vector);
			static void SetUniformMat3(u32 location, const Maths::Matrix3& matrix);
			static void SetUniformMat4(u32 location, const Maths::Matrix4& matrix);
			static void SetUniformMat4Array(u32 location, u32 count, const Maths::Matrix4& matrix);

            static void MakeDefault();
        protected:
            static Shader* CreateFuncGL(const String& name, const String& source);
            
		public:
			static bool TryCompile(const String& source, String& error);
			static bool TryCompileFromFile(const String& filepath, String& error);
		};
	}
}
