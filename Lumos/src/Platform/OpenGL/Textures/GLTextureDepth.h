#pragma once
#include "LM.h"
#include "Graphics/API/Textures/TextureDepth.h"
#include "../GLDebug.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLTextureDepth : public TextureDepth
		{
		private:
			String m_Name;
			u32 m_Handle;
			u32 m_Width, m_Height;
		public:
			GLTextureDepth(u32 width, u32 height);
			~GLTextureDepth();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;
			void Resize(u32 width, u32 height) override;

			inline void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }
		protected:
			void Init();
		};
	}
}