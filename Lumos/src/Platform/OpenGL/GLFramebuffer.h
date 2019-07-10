#pragma once
#include "LM.h"
#include "Graphics/API/Framebuffer.h"
#include "Platform/OpenGL/GL.h"
#include "Textures/GLTexture2D.h"

namespace Lumos
{
	namespace Graphics
	{
		enum class Format;

		class LUMOS_EXPORT GLFramebuffer : public Framebuffer
		{
		public:

			GLFramebuffer();
			GLFramebuffer(FramebufferInfo bufferInfo);
			~GLFramebuffer();

			inline u32 GetFramebuffer() const { return m_Handle; }

			void GenerateFramebuffer() override;

			void Bind(u32 width, u32 height) const override;
			void Bind() const override;
			void UnBind() const override;
			void Clear() override {}
			u32 GetWidth() const override { return m_Width; }
			u32 GetHeight() const override { return m_Height; }

			GLenum GetAttachmentPoint(Graphics::TextureFormat format);

			inline void SetClearColour(const Maths::Vector4& colour) override { m_ClearColour = colour; }

			void AddTextureAttachment(TextureFormat format, Texture* texture) override;
			void AddCubeTextureAttachment(TextureFormat format, CubeFace face, TextureCube* texture) override;

			void AddShadowAttachment(Texture*  texture) override;
			void AddTextureLayer(int index, Texture*  texture) override;

			void Validate() override;

		private:

			u32 m_Handle;
			u32 m_Width, m_Height, m_ColourAttachmentCount;
			Maths::Vector4 m_ClearColour;
			std::vector<GLenum> m_AttachmentData;
		};
	}
}
