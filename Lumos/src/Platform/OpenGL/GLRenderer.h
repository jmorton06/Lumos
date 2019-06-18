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
			GLRenderer(uint width, uint height);
			~GLRenderer();
            
            static GLRenderer* Instance() { return static_cast<GLRenderer*>(s_Instance); }

			void Begin() override;
			void BindScreenFBOInternal() override;
			void InitInternal() override;

			void SetRenderTargets(uint numTargets) override;
			void DrawInternal(DrawType type, uint count, DataType dataType = DataType::UNSIGNED_INT, void* indices = nullptr) const override;
			void DrawArraysInternal(DrawType type, uint numIndices) const override;
			void DrawArraysInternal(DrawType type, uint start, uint numIndices)	const override;
			void SetRenderModeInternal(RenderMode mode) override;
			void OnResize(uint width, uint height) override;
			void ClearInternal(uint buffer) override;
			void PresentInternal() override;
			void PresentInternal(Graphics::CommandBuffer* cmdBuffer) override;
			void SetDepthTestingInternal(bool enabled) override;
			void SetBlendInternal(bool enabled) override;
			void SetStencilTestInternal(bool enabled) override;
			void SetCullingInternal(bool enabled, bool front) override;
			void SetDepthMaskInternal(bool enabled) override;
			void SetViewportInternal(uint x, uint y, uint width, uint height) override;
			void SetPixelPackType(PixelPackType type) override;

			void SetColourMaskInternal(bool r, bool g, bool b, bool a) override;

			void SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination) override;
			void SetBlendEquationInternal(RendererBlendFunction blendEquation) override;
			void SetStencilFunctionInternal(StencilType type, uint ref, uint mask) override;
			void SetStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass) override;

			void RenderMeshInternal(Mesh* mesh, Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, uint dynamicOffset, Graphics::DescriptorSet* descriptorSet, bool useMaterialDescriptorSet) override;
			void Render(VertexArray* vertexArray, IndexBuffer* indexBuffer, Graphics::CommandBuffer* cmdBuffer, std::vector<Graphics::DescriptorSet*>& descriptorSets, Graphics::Pipeline* pipeline, uint dynamicOffset) override;

			Swapchain* GetSwapchainInternal() const override { return m_Swapchain; }

			const String& GetTitleInternal() const override;

			static uint RendererBufferToGL(uint buffer);
			static uint RendererBlendFunctionToGL(RendererBlendFunction function);
			static uint DataTypeToGL(DataType dataType);
			static uint DrawTypeToGL(DrawType drawType);

		protected:

			String m_RendererTitle;
			Graphics::GLContext* m_Context;
			Graphics::GLSwapchain* m_Swapchain;

		};
	}
}



