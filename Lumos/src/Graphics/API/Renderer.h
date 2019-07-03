#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{
	namespace Graphics
	{
		class Pipeline;
		class CommandBuffer;
		class DescriptorSet;
		class Swapchain;
		class IndexBuffer;

		enum LUMOS_EXPORT RendererBufferType
		{
			RENDERER_BUFFER_NONE = 0,
			RENDERER_BUFFER_COLOUR = BIT(0),
			RENDERER_BUFFER_DEPTH = BIT(1),
			RENDERER_BUFFER_STENCIL = BIT(2)
		};

		enum class LUMOS_EXPORT DrawType
		{
			POINT,
			TRIANGLE,
			LINES
		};

		enum class LUMOS_EXPORT StencilType
		{
			EQUAL,
			NOTEQUAL,
			KEEP,
			REPLACE,
			ZERO,
			ALWAYS
		};

		enum class LUMOS_EXPORT PixelPackType
		{
			PACK,
			UNPACK
		};

		enum class LUMOS_EXPORT RendererBlendFunction
		{
			NONE,
			ZERO,
			ONE,
			SOURCE_ALPHA,
			DESTINATION_ALPHA,
			ONE_MINUS_SOURCE_ALPHA
		};

		enum class LUMOS_EXPORT RendererBlendEquation
		{
			NONE,
			ADD,
			SUBTRACT
		};

		enum class LUMOS_EXPORT RenderMode
		{
			FILL,
			WIREFRAME
		};

		enum class LUMOS_EXPORT DataType
		{
			FLOAT,
			UNSIGNED_INT,
			UNSIGNED_BYTE
		};

		class Mesh;
		class VertexArray;

		class LUMOS_EXPORT Renderer
		{
		public:
			Renderer()
			{
			}

			virtual ~Renderer() = default;

			static void Init(u32 width, u32 height);
			static void Release();
			virtual void InitInternal() = 0;
			virtual void Begin() = 0;
			virtual void BindScreenFBOInternal() = 0;
			virtual void OnResize(u32 width, u32 height) = 0;
			inline static Renderer* GetRenderer() { return s_Instance; }

			virtual void Render(VertexArray* vertexArray, IndexBuffer* indexBuffer, Graphics::CommandBuffer* cmdBuffer, std::vector<Graphics::DescriptorSet*>& descriptorSets, Graphics::Pipeline* pipeline, u32 dynamicOffset = 0) = 0;

			virtual void PresentInternal() = 0;
			virtual void PresentInternal(Graphics::CommandBuffer* cmdBuffer) = 0;

			virtual const String& GetTitleInternal() const = 0;
			virtual void DrawArraysInternal(DrawType type, u32 numIndices, u32 start) const = 0;
			virtual void DrawInternal(DrawType type, u32 count, DataType datayType, void* indices) const = 0;
			virtual void RenderMeshInternal(Mesh* mesh, Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) = 0;
			virtual Graphics::Swapchain* GetSwapchainInternal() const = 0;

			inline static void BindScreenFBO() { s_Instance->BindScreenFBOInternal(); }
			inline static void Present() { s_Instance->PresentInternal(); }
			inline static void Present(Graphics::CommandBuffer* cmdBuffer) { s_Instance->PresentInternal(cmdBuffer); }
			inline static void Draw(DrawType type, u32 count, DataType datayType = DataType::UNSIGNED_INT, void* indices = nullptr) { s_Instance->DrawInternal(type, count, datayType, indices); }
			inline static void DrawArrays(DrawType type, u32 numIndices, u32 start = 0) { s_Instance->DrawArraysInternal(type, numIndices, start); }
			inline static void RenderMesh(Mesh* mesh, Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) { s_Instance->RenderMeshInternal(mesh, pipeline, cmdBuffer, dynamicOffset, descriptorSets); }
			inline static const String& GetTitle() { return s_Instance->GetTitleInternal(); }

			inline static Swapchain* GetSwapchain() { return s_Instance->GetSwapchainInternal(); }
            
        protected:
            static Renderer* s_Instance;
		};
	}
}

