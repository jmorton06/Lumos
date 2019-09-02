#pragma once
#include "LM.h"
#include "Graphics/API/Framebuffer.h"
#include "VK.h"
#include "VKTexture.h"
#include "VKRenderpass.h"

namespace Lumos
{
	namespace Graphics
	{
		class LUMOS_EXPORT VKFramebuffer : public Framebuffer
		{
		public:

			VKFramebuffer(const FramebufferInfo& frameBufferInfo);
			~VKFramebuffer();

			inline vk::Framebuffer GetFramebuffer() const { return m_Framebuffer; }

			void SetClearColour(const Maths::Vector4& colour) override {};

			u32 GetWidth() const override { return m_Width; }
			u32 GetHeight() const override { return m_Height; }

			void Bind(u32 width, u32 height) const override {};
			void Bind() const override {};
			void UnBind() const override {};
			void Clear() override {};

			void AddTextureAttachment(TextureFormat format, Texture* texture) override {};
			void AddCubeTextureAttachment(TextureFormat format, CubeFace face, TextureCube* texture) override {};
			void AddShadowAttachment(Texture* texture) override {};
			void AddTextureLayer(int index, Texture* texture) override {};
			void GenerateFramebuffer() override {};
            
            static void MakeDefault();
        protected:
            static Framebuffer* CreateFuncVulkan(const FramebufferInfo&);
		private:

			u32 m_Width;
			u32 m_Height;
			u32 m_AttachmentCount;
			vk::Framebuffer m_Framebuffer;
		};
	}
}
