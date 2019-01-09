#pragma once

#include "JM.h"
#include "Graphics/API/ShaderResource.h"
#include "GLShaderUniform.h"
#include "GLDebug.h"

namespace jm
{

	class GLShaderResourceDeclaration : public ShaderResourceDeclaration
	{
	public:
		enum class Type
		{
			NONE, TEXTURE2D, TEXTURECUBE, TEXTURESHADOW, TEXTURESHADOWARRAY
		};
	private:
		friend class GLShader;
	private:
		String m_Name;
		uint m_Register;
		uint m_Count;
		Type m_Type;
	public:
		GLShaderResourceDeclaration(Type type, const String& name, uint count);

		inline const String& GetName() const override { return m_Name; }
		inline uint GetRegister() const override { return m_Register; }
		inline uint GetCount() const override { return m_Count; }

		inline Type GetType() const { return m_Type; }
	public:
		static Type StringToType(const String& type);
		static String TypeToString(Type type);
	};
}