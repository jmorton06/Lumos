#pragma once
#include "LM.h"
#include "Graphics/API/Pipeline.h"
#include "Maths/Maths.h"
#include "App/Window.h"
#include "Graphics/API/Renderer.h"
#include "GLSwapchain.h"

namespace Lumos
{
	namespace graphics
	{
		class GLContext;
	}

	class Shader;
	class Window;

	class LUMOS_EXPORT GLRenderer : public Renderer
	{
	public:
		friend class Window;
		GLRenderer(uint width, uint height);
		~GLRenderer();

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

		void RenderMeshInternal(Mesh* mesh, graphics::api::Pipeline* pipeline, graphics::api::CommandBuffer* cmdBuffer, uint dynamicOffset, graphics::api::DescriptorSet* descriptorSet, bool useMaterialDescriptorSet) override;

		graphics::api::Swapchain* GetSwapchainInternal() const override { return m_Swapchain; }

		const String& GetTitleInternal() const override;

		static uint RendererBufferToGL(uint buffer);
		static uint RendererBlendFunctionToGL(RendererBlendFunction function);
		static uint DataTypeToGL(DataType dataType);
		static uint DrawTypeToGL(DrawType drawType);

	protected:

		String m_RendererTitle;
		graphics::GLContext* m_Context;
		graphics::GLSwapchain* m_Swapchain;

	};
}



