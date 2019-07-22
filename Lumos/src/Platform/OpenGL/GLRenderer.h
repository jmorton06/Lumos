#pragma once
#include "LM.h"
#include "Graphics/API/Pipeline.h"
#include "Maths/Maths.h"
#include "App/Window.h"
#include "Graphics/API/Renderer.h"
#include "GLSwapchain.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLContext;
		class CommandBuffer;
		class Shader;
		class Window;

		class LUMOS_EXPORT GLRenderer : public Renderer
		{
		public:
			friend class Window;
			GLRenderer(u32 width, u32 height);
			~GLRenderer();

            static GLRenderer* Instance() { return static_cast<GLRenderer*>(s_Instance); }

			void Begin() override;
			void BindScreenFBOInternal() override;
			void InitInternal() override;

			void DrawInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, DataType dataType = DataType::UNSIGNED_INT, void* indices = nullptr) const override;
			void DrawArraysInternal(CommandBuffer* commandBuffer, DrawType type, u32 numIndices, u32 start)	const override;
			void SetRenderModeInternal(RenderMode mode);
			void OnResize(u32 width, u32 height) override;
			void PresentInternal() override;
			void PresentInternal(Graphics::CommandBuffer* cmdBuffer) override;
			void SetDepthTestingInternal(bool enabled);
			void SetBlendInternal(bool enabled);
			void SetStencilTestInternal(bool enabled);
			void SetCullingInternal(bool enabled, bool front);
			void SetDepthMaskInternal(bool enabled);
			void SetViewportInternal(u32 x, u32 y, u32 width, u32 height);
			void SetPixelPackType(PixelPackType type);

			void SetColourMaskInternal(bool r, bool g, bool b, bool a);

			void SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination);
			void SetBlendEquationInternal(RendererBlendFunction blendEquation);
			void SetStencilFunctionInternal(StencilType type, u32 ref, u32 mask);
			void SetStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass);

			void RenderMeshInternal(Mesh* mesh, Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) override;
			void Render(VertexArray* vertexArray, IndexBuffer* indexBuffer, Graphics::CommandBuffer* cmdBuffer, std::vector<Graphics::DescriptorSet*>& descriptorSets, Graphics::Pipeline* pipeline, u32 dynamicOffset) override;

			static void ClearInternal(u32 buffer);

			Swapchain* GetSwapchainInternal() const override { return m_Swapchain; }

			const String& GetTitleInternal() const override;

		protected:

			String m_RendererTitle;
			Graphics::GLContext* m_Context;
			Graphics::GLSwapchain* m_Swapchain;

		};
	}
}



