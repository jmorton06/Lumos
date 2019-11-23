#pragma once
#include "lmpch.h"
#include "Graphics/API/Texture.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLTexture2D : public Texture2D
		{
		public:
			GLTexture2D(u32 width, u32 height, void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			GLTexture2D(const String& name, const String& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			GLTexture2D();
			~GLTexture2D();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;

			virtual void SetData(const void* pixels) override;

			virtual void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			_FORCE_INLINE_ u32 GetWidth() const override { return m_Width; }
			_FORCE_INLINE_ u32 GetHeight() const override { return m_Height; }

			_FORCE_INLINE_ const String& GetName() const override { return m_Name; }
			_FORCE_INLINE_ const String& GetFilepath() const override { return m_FileName; }

			void BuildTexture(TextureFormat internalformat, u32 width, u32 height, bool depth, bool samplerShadow) override;

			u8* LoadTextureData();
			u32  LoadTexture(void* data) const;
            
            static void MakeDefault();
        protected:
			static Texture2D* CreateFuncGL();
			static Texture2D* CreateFromSourceFuncGL(u32, u32, void*, TextureParameters, TextureLoadOptions);
			static Texture2D* CreateFromFileFuncGL(const String&, const String&, TextureParameters, TextureLoadOptions);

		private:
			u32 Load(void* data);

			String m_Name;
			String m_FileName;
			u32 m_Handle;
			u32 m_Width, m_Height;
			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;
		};

		class GLTextureCube : public TextureCube
		{
		public:
			GLTextureCube(u32 size);
			GLTextureCube(const String& filepath);
			GLTextureCube(const String* files);
			GLTextureCube(const String* files, u32 mips, InputFormat format);
			~GLTextureCube();

			_FORCE_INLINE_ void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;

			_FORCE_INLINE_ u32 GetSize() const override { return m_Size; }
			_FORCE_INLINE_ const String& GetName() const override { return m_Name; }
			_FORCE_INLINE_ const String& GetFilepath() const override { return m_Files[0]; }
            
            static void MakeDefault();
        protected:
			static TextureCube* CreateFuncGL(u32);
			static TextureCube* CreateFromFileFuncGL(const String& filepath);
			static TextureCube* CreateFromFilesFuncGL(const String* files);
			static TextureCube* CreateFromVCrossFuncGL(const String* files, u32 mips, InputFormat format);
            
		private:
			static u32 LoadFromSingleFile();
			u32 LoadFromMultipleFiles();
			u32 LoadFromVCross(u32 mips);

			u32 m_Handle;
			u32 m_Width, m_Height, m_Size;
			String m_Name;
			String m_Files[MAX_MIPS];
			u32 m_Bits;
			u32 m_NumMips;
			InputFormat m_Format;
			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;
		};

		class GLTextureDepth : public TextureDepth
		{
		public:
			GLTextureDepth(u32 width, u32 height);
			~GLTextureDepth();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;
			void Resize(u32 width, u32 height) override;

			_FORCE_INLINE_ void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			_FORCE_INLINE_ const String& GetName() const override { return m_Name; }
			_FORCE_INLINE_ const String& GetFilepath() const override { return m_Name; }

			static void MakeDefault();
		protected:
			static TextureDepth* CreateFuncGL(u32, u32);

			void Init();

			String m_Name;
			u32 m_Handle;
			u32 m_Width, m_Height;
		};

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

			_FORCE_INLINE_ void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			_FORCE_INLINE_ const String& GetName() const override { return m_Name; }
			_FORCE_INLINE_ const String& GetFilepath() const override { return m_Name; }

			_FORCE_INLINE_ void SetCount(u32 count) { m_Count = count; }

			void Init() override;

			static void MakeDefault();
		protected:
			static TextureDepthArray* CreateFuncGL(u32, u32, u32);
		};
	}
}
