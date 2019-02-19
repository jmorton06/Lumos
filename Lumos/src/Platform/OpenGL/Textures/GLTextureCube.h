#pragma once
#include "LM.h"
#include "Graphics/API/Textures/TextureCube.h"
#include "../GLDebug.h"

#define MAX_MIPS 11
namespace Lumos
{

	class GLTextureCube : public TextureCube
	{
	private:
		uint m_Handle;
		uint m_Width, m_Height, m_Size;
		String m_Name;
		String m_Files[MAX_MIPS];
		uint m_Bits;
		uint m_NumMips;
		InputFormat m_Format;
		TextureParameters m_Parameters;
		TextureLoadOptions m_LoadOptions;
	public:
		GLTextureCube(uint size);
		GLTextureCube(const String& name, const String& filepath);
		GLTextureCube(const String& name, const String* files);
		GLTextureCube(const String& name, const String* files, uint mips, InputFormat format);
		~GLTextureCube();

		inline void* GetHandle() const override { return (void*)(size_t)m_Handle; }

		void Bind(uint slot = 0) const override;
		void Unbind(uint slot = 0) const override;

		inline uint GetSize() const override { return m_Size; }
		inline const String& GetName() const override { return m_Name; }
		inline const String& GetFilepath() const override { return m_Files[0]; }
	private:
		static uint LoadFromSingleFile();
		uint LoadFromMultipleFiles();
		uint LoadFromVCross(uint mips);
	};
}
