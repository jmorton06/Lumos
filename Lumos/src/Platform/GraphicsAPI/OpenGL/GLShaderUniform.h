#pragma once

#include "LM.h"
#include "Graphics/API/ShaderUniform.h"
#include "GLDebug.h"


namespace Lumos
{

	class GLShaderUniformDeclaration : public ShaderUniformDeclaration
	{
	private:
		friend class GLShader;
		friend class GLShaderUniformBufferDeclaration;
	public:
		enum class Type
		{
			NONE, FLOAT32, VEC2, VEC3, VEC4, MAT3, MAT4, INT32, INT, STRUCT, MAT4ARRAY
		};
	private:
		String m_Name;
		uint m_Size;
		uint m_Count;
		uint m_Offset;

		Type m_Type;
		ShaderStruct* m_Struct;
		mutable int32 m_Location;
	public:
		GLShaderUniformDeclaration(Type type, const String& name, uint count = 1);
		GLShaderUniformDeclaration(ShaderStruct* uniformStruct, const String& name, uint count = 1);

		inline const String& GetName() const override { return m_Name; }
		inline void SetName(const String& name) override { m_Name = name; }
		inline uint GetSize() const override { return m_Size; }
		inline uint GetCount() const override { return m_Count; }
		inline uint GetOffset() const override { return m_Offset; }
		inline uint GetAbsoluteOffset() const { return m_Struct ? m_Struct->GetOffset() + m_Offset : m_Offset; }

		int32 GetLocation() const { return m_Location; }
		inline Type GetType() const { return m_Type; }
		inline const ShaderStruct& GetShaderUniformStruct() const { LUMOS_CORE_ASSERT(m_Struct, "");  return *m_Struct; }

	protected:
		void SetOffset(uint offset) override;
	public:
		static uint SizeOfUniformType(Type type);
		static Type StringToType(const String& type, uint count);
		static String TypeToString(Type type);
	};

	struct LUMOS_EXPORT GLShaderUniformField
	{
		GLShaderUniformDeclaration::Type type;
		String name;
		uint count;
		mutable uint size;
		mutable int32 location;
	};

	// TODO: Eventually support OpenGL uniform buffers. This is more platform-side.
	class GLShaderUniformBufferDeclaration : public ShaderUniformBufferDeclaration
	{
	private:
		friend class Shader;
	private:
		String m_Name;
		ShaderUniformList m_Uniforms;
		uint m_Register;
		uint m_Size;
		uint m_ShaderType; // 0 = VS, 1 = PS, 2 = GS
	public:
		GLShaderUniformBufferDeclaration(const String& name, uint shaderType);

		void PushUniform(GLShaderUniformDeclaration* uniform);

		inline const String& GetName() const override { return m_Name; }
		inline uint GetRegister() const override { return m_Register; }
		inline uint GetShaderType() const override { return m_ShaderType; }
		inline uint GetSize() const override { return m_Size; }
		inline const ShaderUniformList& GetUniformDeclarations() const override { return m_Uniforms; }

		ShaderUniformDeclaration* FindUniform(const String& name) override;
		~GLShaderUniformBufferDeclaration() override;
	};
}