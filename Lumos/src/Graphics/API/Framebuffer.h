#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"

namespace Lumos
{
	namespace Graphics
	{
		enum class CubeFace
		{
			PositiveX,
			NegativeX,
			PositiveY,
			NegativeY,
			PositiveZ,
			NegativeZ
		};

		class Texture;
		class Texture2D;
		class TextureCube;
		enum class TextureType;
		enum class TextureFormat;
		class RenderPass;

		struct FramebufferInfo
		{
			u32 width;
			u32 height;
			u32 layer = 0;
			u32 attachmentCount;
			bool screenFBO = false;
			Texture** attachments;
			TextureType* attachmentTypes;
			Graphics::RenderPass* renderPass;
		};

		class LUMOS_EXPORT Framebuffer
		{
		public:

			static Framebuffer* Create(const FramebufferInfo& framebufferInfo);

			virtual ~Framebuffer(){};

			virtual void Bind(u32 width, u32 height) const = 0;
			virtual void Bind() const = 0;
			virtual void UnBind() const = 0;
			virtual void Clear() = 0;
			virtual void Validate() {};
			virtual void AddTextureAttachment(TextureFormat format, Texture* texture) = 0;
			virtual void AddCubeTextureAttachment(TextureFormat format, CubeFace face, TextureCube* texture) = 0;
			virtual void AddShadowAttachment(Texture* texture) = 0;
			virtual void AddTextureLayer(int index, Texture* texture) = 0;
			virtual void GenerateFramebuffer() = 0;

			virtual u32 GetWidth() const = 0;
			virtual u32 GetHeight() const = 0;
			virtual void SetClearColour(const Maths::Vector4& colour) = 0;
            
        protected:
            static Framebuffer* (*CreateFunc)(const FramebufferInfo&);
		};
	}
}
