#pragma once
#include "LM.h"
#include "Textures/Texture2D.h"
#include "Maths/Maths.h"

namespace Lumos
{
	enum class LUMOS_EXPORT Attachment
	{
		Colour0,
		Colour1,
		Colour2,
		Colour3,
		Colour4,
		Colour5,
		Depth,
		DepthArray,
		Stencil
	};

	enum class CubeFace
	{
		PositiveX,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ
	};

	class Texture2D;
	class TextureCube;

	namespace graphics
	{
		namespace api
		{
			class RenderPass;
		}
	}

	struct FramebufferInfo
	{
		uint width;
		uint height;
		uint layer = 0;
		uint attachmentCount;
		bool screenFBO = false;
		Lumos::Texture** attachments;
		Lumos::TextureType* attachmentTypes;
		Lumos::graphics::api::RenderPass* renderPass;
	};

	class LUMOS_EXPORT Framebuffer
	{
	public:

		static Framebuffer* Create(FramebufferInfo framebufferInfo = FramebufferInfo());

		virtual ~Framebuffer(){};

		virtual void Bind(uint width, uint height) const = 0;
		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;
		virtual void Clear() = 0;
		virtual void Validate() {};
		virtual void AddTextureAttachment(Attachment attachmentType, Texture* texture) = 0;
		virtual void AddCubeTextureAttachment(Attachment attachmentType, CubeFace face, TextureCube* texture) = 0;
		virtual void AddShadowAttachment(Texture* texture) = 0;
		virtual void AddTextureLayer(int index, Texture* texture) = 0;
		virtual void GenerateFramebuffer() = 0;

		virtual uint GetWidth() const = 0;
		virtual uint GetHeight() const = 0;
		virtual void SetClearColour(const maths::Vector4& colour) = 0;
	};
}
