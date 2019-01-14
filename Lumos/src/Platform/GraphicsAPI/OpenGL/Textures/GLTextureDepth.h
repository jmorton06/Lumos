#pragma once
#include "JM.h"
#include "Graphics/API/Textures/TextureDepth.h"
#include "../GLDebug.h"

namespace jm
{

	class GLTextureDepth : public TextureDepth
	{
	private:
		String m_Name;
		uint m_Handle;
		uint m_Width, m_Height;
	public:
		GLTextureDepth(uint width, uint height);
		~GLTextureDepth();

		void Bind(uint slot = 0) const override;
		void Unbind(uint slot = 0) const override;
		void Resize(uint width, uint height) override;

		inline uint GetHandle() const override { return m_Handle; }

		inline const String& GetName() const override { return m_Name; }
		inline const String& GetFilepath() const override { return m_Name; }
	protected:
		void Init();
	};
}