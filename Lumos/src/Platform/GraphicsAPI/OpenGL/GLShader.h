#pragma once
#include "LM.h"
#include "Graphics/API/Shader.h"
#include "GLDebug.h"
#include "GLShaderUniform.h"
#include "GLShaderResource.h"

namespace Lumos
{

	struct GLShaderErrorInfo
	{
		GLShaderErrorInfo() : shader(0)
		{
		};
		uint shader;
		String message[6];
		uint line[6];
	};

	class GLShader : public Shader
	{
	private:
		friend class Shader;
		friend class ShaderManager;
	private:
		uint m_Handle;
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

		void SetSystemUniformBuffer(ShaderType type, byte* data, uint size, uint slot) override;
		void SetUserUniformBuffer(ShaderType type, byte* data, uint size) override;

		void SetUniform(const String& name, byte* data);
		void ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, byte* data, int32 offset, uint count) const;

		inline const String& GetName() const override { return m_Name; }
		inline const String& GetFilePath() const override { return m_Path; }

		inline const ShaderUniformBufferList GetSystemUniforms(ShaderType type) const override { try { return m_UniformBuffers.at(type); } catch (std::exception) { return ShaderUniformBufferList(); } }
		inline const ShaderUniformBufferDeclaration* GetUserUniformBuffer(ShaderType type) const override { try { return m_UserUniformBuffers.at(type); } catch (std::exception) { return nullptr; } }
		inline const std::vector<ShaderType> GetShaderTypes() const override { return m_ShaderTypes; }
		inline const ShaderResourceList& GetResources() const { return m_Resources; }

		static GLuint CompileShader(ShaderType type, String source, uint program, GLShaderErrorInfo& info);
		static uint Compile(std::map<ShaderType, String>* sources, GLShaderErrorInfo& info);
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
		int32 GetUniformLocation(const String& name) const;
		uint GetHandle() const { return m_Handle; }
		static ShaderUniformDeclaration* FindUniformDeclaration(const String& name, const ShaderUniformBufferDeclaration* buffer);
		ShaderUniformDeclaration* FindUniformDeclaration(const String& name);

		void ResolveAndSetUniforms(ShaderUniformBufferDeclaration* buffer, byte* data, uint size) const;
		void ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, byte* data, uint size, uint count) const;

		void SetUniformStruct(GLShaderUniformDeclaration* uniform, byte* data, int32 offset) const;

		void SetUniform1f(const String& name, float value) const;
		void SetUniform1fv(const String& name, float* value, int32 count) const;
		void SetUniform1i(const String& name, int32 value) const;
		void SetUniform1iv(const String& name, int32* value, int32 count) const;
		void SetUniform2f(const String& name, const maths::Vector2& vector) const;
		void SetUniform3f(const String& name, const maths::Vector3& vector) const;
		void SetUniform4f(const String& name, const maths::Vector4& vector) const;
		void SetUniformMat4(const String& name, const maths::Matrix4& matrix) const;

		static void SetUniform1f(uint location, float value);
		static void SetUniform1fv(uint location, float* value, int32 count);
		static void SetUniform1i(uint location, int32 value);
		static void SetUniform1iv(uint location, int32* value, int32 count);
		static void SetUniform2f(uint location, const maths::Vector2& vector);
		static void SetUniform3f(uint location, const maths::Vector3& vector);
		static void SetUniform4f(uint location, const maths::Vector4& vector);
		static void SetUniformMat3(uint location, const maths::Matrix3& matrix);
		static void SetUniformMat4(uint location, const maths::Matrix4& matrix);
		static void SetUniformMat4Array(uint location, uint count, const maths::Matrix4& matrix);

	public:
		static bool TryCompile(const String& source, String& error);
		static bool TryCompileFromFile(const String& filepath, String& error);
	};
}
