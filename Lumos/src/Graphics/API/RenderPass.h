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

		struct RenderPassInfo
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
			static RenderPass* Create(const RenderPassInfo& renderPassCI);

			virtual void BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height, bool beginCommandBuffer = true) const = 0;
			virtual void EndRenderpass(CommandBuffer* commandBuffer, bool endCommandBuffer = true) = 0;
            virtual int GetAttachmentCount() const = 0;
            
        protected:
            static RenderPass* (*CreateFunc)(const RenderPassInfo&);
		};
	}
}
