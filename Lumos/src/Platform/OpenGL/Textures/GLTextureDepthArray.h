#pragma once
#include "LM.h"
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "../GLDebug.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLTextureDepthArray : public TextureDepthArray
		{
		private:
			String m_Name;
			u32 m_Handle;
			u32 m_Width, m_Height, m_Count;
		public:
			GLTextureDepthArray(u32 width, u32 height, u32 count);
			~GLTextureDepthArray();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;
			void Resize(u32 width, u32 height, u32 count) override;

			inline void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			inline void SetCount(u32 count) { m_Count = count; }

			void Init() override;
		};
	}
}