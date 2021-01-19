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
			u32 m_Register;
			u32 m_Count;
			Type m_Type;

		public:
			GLShaderResourceDeclaration(Type type, const std::string& name, u32 count);

			_FORCE_INLINE_ const std::string& GetName() const override
			{
				return m_Name;
			}
			_FORCE_INLINE_ u32 GetRegister() const override
			{
				return m_Register;
			}
			_FORCE_INLINE_ u32 GetCount() const override
			{
				return m_Count;
			}

			_FORCE_INLINE_ Type GetType() const
			{
				return m_Type;
			}

		public:
			static Type StringToType(const std::string& type);
			static std::string TypeToString(Type type);
		};
	}
}
