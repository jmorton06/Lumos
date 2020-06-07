#pragma once

#include "lmpch.h"
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
				NONE, FLOAT32, VEC2, VEC3, VEC4, MAT3, MAT4, INT32, INT, UINT, STRUCT, MAT4ARRAY
			};
		private:
			String m_Name;
			u32 m_Size;
			u32 m_Count;
			u32 m_Offset;

			Type m_Type;
			ShaderStruct* m_Struct;
			mutable i32 m_Location;
		public:
			GLShaderUniformDeclaration(Type type, const String& name, u32 count = 1);
			GLShaderUniformDeclaration(ShaderStruct* uniformStruct, const String& name, u32 count = 1);

			_FORCE_INLINE_ const String& GetName() const override { return m_Name; }
			_FORCE_INLINE_ void SetName(const String& name) override { m_Name = name; }
			_FORCE_INLINE_ u32 GetSize() const override { return m_Size; }
			_FORCE_INLINE_ u32 GetCount() const override { return m_Count; }
			_FORCE_INLINE_ u32 GetOffset() const override { return m_Offset; }
			_FORCE_INLINE_ u32 GetAbsoluteOffset() const { return m_Struct ? m_Struct->GetOffset() + m_Offset : m_Offset; }

			i32 GetLocation() const { return m_Location; }
			_FORCE_INLINE_ Type GetType() const { return m_Type; }
			_FORCE_INLINE_ const ShaderStruct& GetShaderUniformStruct() const { LUMOS_ASSERT(m_Struct, "");  return *m_Struct; }

		protected:
			void SetOffset(u32 offset) override;
		public:
			static u32 SizeOfUniformType(Type type);
			static Type StringToType(const String& type, u32 count);
			static String TypeToString(Type type);
		};

		struct LUMOS_EXPORT GLShaderUniformField
		{
			GLShaderUniformDeclaration::Type type;
			String name;
			u32 count;
			mutable u32 size;
			mutable i32 location;
		};

		// TODO: Eventually support OpenGL uniform buffers. This is more platform-side.
		class GLShaderUniformBufferDeclaration : public ShaderUniformBufferDeclaration
		{
		private:
			friend class Shader;
		private:
			String m_Name;
			ShaderUniformList m_Uniforms;
			u32 m_Register;
			u32 m_Size;
			u32 m_ShaderType; // 0 = VS, 1 = PS, 2 = GS
		public:
			GLShaderUniformBufferDeclaration(const String& name, u32 shaderType);

			void PushUniform(GLShaderUniformDeclaration* uniform);

			_FORCE_INLINE_ const String& GetName() const override { return m_Name; }
			_FORCE_INLINE_ u32 GetRegister() const override { return m_Register; }
			_FORCE_INLINE_ u32 GetShaderType() const override { return m_ShaderType; }
			_FORCE_INLINE_ u32 GetSize() const override { return m_Size; }
			_FORCE_INLINE_ const ShaderUniformList& GetUniformDeclarations() const override { return m_Uniforms; }

			ShaderUniformDeclaration* FindUniform(const String& name) override;
			~GLShaderUniformBufferDeclaration() override;
            
            static void MakeDefault();
        protected:
            static CommandBuffer* CreateFuncGL();
		};
	}
}
