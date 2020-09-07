#pragma once


namespace Lumos
{
	namespace Maths
	{
		class Vector4;
	}

	namespace Graphics
	{
		class CommandBuffer;
		class Framebuffer;
		enum class TextureType : int;
		enum class TextureFormat;

		struct AttachmentInfo
		{
			TextureType textureType;
			TextureFormat format;
		};

		struct RenderpassInfo
		{
			AttachmentInfo* textureType;
			int attachmentCount;
			bool clear = true;
		};

		enum SubPassContents
		{
			INLINE,
			SECONDARY
		};

		class LUMOS_EXPORT RenderPass
		{
		public:
			virtual ~RenderPass();
			static RenderPass* Create();

			virtual bool Init(const RenderpassInfo& renderpassCI) = 0;
			virtual void Unload() const = 0;
			virtual void BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame,
				SubPassContents contents, uint32_t width, uint32_t height) const = 0;
			virtual void EndRenderpass(CommandBuffer * commandBuffer) = 0;
            
        protected:
            static RenderPass* (*CreateFunc)();
		};
	}
}
