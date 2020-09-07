#pragma once

namespace Lumos
{
	namespace Graphics
	{
		class Pipeline;
		class CommandBuffer;
		class DescriptorSet;
		class Swapchain;
		class IndexBuffer;
		class Mesh;

		enum RendererBufferType
		{
			RENDERER_BUFFER_NONE = 0,
			RENDERER_BUFFER_COLOUR = BIT(0),
			RENDERER_BUFFER_DEPTH = BIT(1),
			RENDERER_BUFFER_STENCIL = BIT(2)
		};

		enum class DrawType
		{
			POINT,
			TRIANGLE,
			LINES
		};

		enum class StencilType
		{
			EQUAL,
			NOTEQUAL,
			KEEP,
			REPLACE,
			ZERO,
			ALWAYS
		};

		enum class PixelPackType
		{
			PACK,
			UNPACK
		};

		enum class RendererBlendFunction
		{
			NONE,
			ZERO,
			ONE,
			SOURCE_ALPHA,
			DESTINATION_ALPHA,
			ONE_MINUS_SOURCE_ALPHA
		};

		enum class RendererBlendEquation
		{
			NONE,
			ADD,
			SUBTRACT
		};

		enum class RenderMode
		{
			FILL,
			WIREFRAME
		};

		enum class DataType
		{
			FLOAT,
			UNSIGNED_INT,
			UNSIGNED_BYTE
		};

		struct RenderAPICapabilities
		{
			std::string Vendor;
			std::string Renderer;
			std::string Version;

			int MaxSamples = 0;
			float MaxAnisotropy = 0.0f;
			int MaxTextureUnits = 0;
			int UniformBufferOffsetAlignment = 0;
		};

		class LUMOS_EXPORT Renderer
		{
		public:
			Renderer() = default;
			virtual ~Renderer() = default;

			static void Init(u32 width, u32 height);
			static void Release();
			virtual void InitInternal() = 0;
			virtual void Begin() = 0;
			virtual void OnResize(u32 width, u32 height) = 0;
			_FORCE_INLINE_ static Renderer* GetRenderer()
			{
				return s_Instance;
			}

			virtual void PresentInternal() = 0;
			virtual void PresentInternal(Graphics::CommandBuffer* cmdBuffer) = 0;
			virtual void BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) = 0;

			virtual const std::string& GetTitleInternal() const = 0;
			virtual void DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, u32 start) const = 0;
			virtual void DrawInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, DataType datayType, void* indices) const = 0;
			virtual Graphics::Swapchain* GetSwapchainInternal() const = 0;

			_FORCE_INLINE_ static void Present()
			{
				s_Instance->PresentInternal();
			}
			_FORCE_INLINE_ static void Present(Graphics::CommandBuffer* cmdBuffer)
			{
				s_Instance->PresentInternal(cmdBuffer);
			}
			_FORCE_INLINE_ static void BindDescriptorSets(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
			{
				s_Instance->BindDescriptorSetsInternal(pipeline, cmdBuffer, dynamicOffset, descriptorSets);
			}
			_FORCE_INLINE_ static void Draw(CommandBuffer* commandBuffer, DrawType type, u32 count, DataType datayType = DataType::UNSIGNED_INT, void* indices = nullptr)
			{
				s_Instance->DrawInternal(commandBuffer, type, count, datayType, indices);
			}
			_FORCE_INLINE_ static void DrawIndexed(CommandBuffer* commandBuffer, DrawType type, u32 count, u32 start = 0)
			{
				s_Instance->DrawIndexedInternal(commandBuffer, type, count, start);
			}
			_FORCE_INLINE_ static const std::string& GetTitle()
			{
				return s_Instance->GetTitleInternal();
			}

			_FORCE_INLINE_ static Swapchain* GetSwapchain()
			{
				return s_Instance->GetSwapchainInternal();
			}

			static RenderAPICapabilities& GetCapabilities()
			{
				static RenderAPICapabilities capabilities;
				return capabilities;
			}

		protected:
			static Renderer* (*CreateFunc)(u32, u32);

			static Renderer* s_Instance;
		};
	}
}
