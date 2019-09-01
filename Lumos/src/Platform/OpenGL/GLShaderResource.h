#pragma once

#include "LM.h"
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
				NONE, TEXTURE2D, TEXTURECUBE, TEXTURESHADOW, TEXTURESHADOWARRAY
			};
		private:
			friend class GLShader;
		private:
			String m_Name;
			u32 m_Register;
			u32 m_Count;
			Type m_Type;
		public:
			GLShaderResourceDeclaration(Type type, const String& name, u32 count);

			inline const String& GetName() const override { return m_Name; }
			inline u32 GetRegister() const override { return m_Register; }
			inline u32 GetCount() const override { return m_Count; }

			inline Type GetType() const { return m_Type; }
		public:
			static Type StringToType(const String& type);
			static String TypeToString(Type type);
            
            static void MakeDefault();
        protected:
            static CommandBuffer* CreateFuncGL();
		};
	}
}
