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
			uint m_Handle;
			uint m_Width, m_Height, m_Count;
		public:
			GLTextureDepthArray(uint width, uint height, uint count);
			~GLTextureDepthArray();

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;
			void Resize(uint width, uint height, uint count) override;

			inline void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			inline void SetCount(uint count) { m_Count = count; }

			void Init() override;
		};
	}
}