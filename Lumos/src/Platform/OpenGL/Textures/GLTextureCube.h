#pragma once
#include "LM.h"
#include "Graphics/API/Textures/TextureCube.h"

#define MAX_MIPS 11
namespace Lumos
{
	namespace Graphics
	{
		class GLTextureCube : public TextureCube
		{
		private:
			u32 m_Handle;
			u32 m_Width, m_Height, m_Size;
			String m_Name;
			String m_Files[MAX_MIPS];
			u32 m_Bits;
			u32 m_NumMips;
			InputFormat m_Format;
			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;
		public:
			GLTextureCube(u32 size);
			GLTextureCube(const String& name, const String& filepath);
			GLTextureCube(const String& name, const String* files);
			GLTextureCube(const String& name, const String* files, u32 mips, InputFormat format);
			~GLTextureCube();

			inline void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;

			inline u32 GetSize() const override { return m_Size; }
			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Files[0]; }
		private:
			static u32 LoadFromSingleFile();
			u32 LoadFromMultipleFiles();
			u32 LoadFromVCross(u32 mips);
		};
	}
}
