#pragma once
#include "LM.h"
#include "Maths/Maths.h"
#include "VertexArray.h"

#define MAX_POINT_LIGHTS 50

namespace Lumos
{
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

	namespace graphics
	{
		namespace api
		{
			class Pipeline;
			class CommandBuffer;
			class DescriptorSet;
			class Swapchain;
		};
	};

	class LUMOS_EXPORT Renderer
	{
	public:
		Renderer() : init(false)
		{
		}

		virtual ~Renderer() {};

		static void Init(uint width, uint height);
		static void Release();
		virtual void InitInternal() = 0;
		virtual void Begin() = 0;
		virtual void BindScreenFBOInternal() = 0;
		virtual void OnResize(uint width, uint height) = 0;
		static Renderer* s_Instance;
		inline static Renderer* GetRenderer() { return s_Instance; }

		virtual void Render(VertexArray* vertexArray, IndexBuffer* indexBuffer, graphics::api::CommandBuffer* cmdBuffer, std::vector<graphics::api::DescriptorSet*>& descriptorSets, graphics::api::Pipeline* pipeline, uint dynamicOffset = 0 ) = 0;

		bool init;

		virtual void ClearInternal(uint buffer) = 0;
		virtual void PresentInternal() = 0;
		virtual void PresentInternal(graphics::api::CommandBuffer* cmdBuffer) = 0;

		virtual void SetColourMaskInternal(bool r, bool g, bool b, bool a) = 0;
		virtual void SetDepthTestingInternal(bool enabled) = 0;
		virtual void SetStencilTestInternal(bool enabled) = 0;
		virtual void SetCullingInternal(bool enabled, bool front) = 0;
		virtual void SetBlendInternal(bool enabled) = 0;
		virtual void SetDepthMaskInternal(bool enabled) = 0;
		virtual void SetViewportInternal(uint x, uint y, uint width, uint height) = 0;

		virtual void SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination) = 0;
		virtual void SetBlendEquationInternal(RendererBlendFunction blendEquation) = 0;
		virtual void SetStencilFunctionInternal(StencilType type, uint ref, uint mask) = 0;
		virtual void SetStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass) = 0;

		virtual const String& GetTitleInternal() const = 0;
		virtual void DrawArraysInternal(DrawType type, uint numIndices) const = 0;
		virtual void DrawArraysInternal(DrawType type, uint start, uint numIndices) const = 0;
		virtual void DrawInternal(DrawType type, uint count, DataType datayType, void* indices) const = 0;
		virtual void SetRenderTargets(uint numTargets) = 0;
		virtual void SetPixelPackType(PixelPackType type) = 0;
		virtual void SetRenderModeInternal(RenderMode mode) = 0;
		virtual void RenderMeshInternal(Mesh* mesh, graphics::api::Pipeline* pipeline, graphics::api::CommandBuffer* cmdBuffer, uint dynamicOffset, graphics::api::DescriptorSet* descriptorSet, bool useMaterialDescriptorSet) = 0;
		virtual graphics::api::Swapchain* GetSwapchainInternal() const = 0;

		inline static void Clear(uint buffer) { s_Instance->ClearInternal(buffer); }
		inline static void BindScreenFBO() { s_Instance->BindScreenFBOInternal(); }
		inline static void Present() { s_Instance->PresentInternal(); }
		inline static void Present(graphics::api::CommandBuffer* cmdBuffer) { s_Instance->PresentInternal(cmdBuffer); }
		inline static void Draw(DrawType type, uint count, DataType datayType = DataType::UNSIGNED_INT, void* indices = nullptr) { s_Instance->DrawInternal(type, count, datayType, indices); }
		inline static void DrawArrays(DrawType type, uint numIndices) { s_Instance->DrawArraysInternal(type, numIndices); }
		inline static void DrawArrays(DrawType type, uint start, uint numIndices) { s_Instance->DrawArraysInternal(type, start, numIndices); }
		inline static void SetDepthTesting(bool enabled) { s_Instance->SetDepthTestingInternal(enabled); }
		inline static void SetCulling(bool enabled, bool front = false){ s_Instance->SetCullingInternal(enabled, front); }
		inline static void SetDepthMask(bool enabled)  { s_Instance->SetDepthMaskInternal(enabled); }
		inline static void SetBlend(bool enabled) { s_Instance->SetBlendInternal(enabled); }
		inline static void SetStencilTest(bool enabled) { s_Instance->SetStencilTestInternal(enabled); }
		inline static void SetViewport(uint x, uint y, uint width, uint height) { s_Instance->SetViewportInternal(x, y, width, height); }
		inline static void SetColourMask(bool r, bool g, bool b, bool a) { s_Instance->SetColourMaskInternal(r, g, b, a); }
		inline static void SetRenderMode(RenderMode mode) { s_Instance->SetRenderModeInternal(mode); }
		inline static void SetBlendFunction(RendererBlendFunction source, RendererBlendFunction destination) { s_Instance->SetBlendFunctionInternal(source, destination); }
		inline static void SetBlendEquation(RendererBlendFunction blendEquation) { s_Instance->SetBlendEquationInternal(blendEquation); }
		inline static void SetStencilFunction(StencilType type, uint ref, uint mask){ s_Instance->SetStencilFunctionInternal(type, ref, mask); }
		inline static void SetStencilOp(StencilType fail, StencilType zfail, StencilType zpass){ s_Instance->SetStencilOpInternal(fail, zfail, zpass); }
		inline static void RenderMesh(Mesh* mesh, graphics::api::Pipeline* pipeline, graphics::api::CommandBuffer* cmdBuffer, uint dynamicOffset, graphics::api::DescriptorSet* descriptorSet, bool useMaterialDescriptorSet = true) { s_Instance->RenderMeshInternal(mesh,pipeline,cmdBuffer,dynamicOffset, descriptorSet, useMaterialDescriptorSet); }
		inline static const String& GetTitle() { return s_Instance->GetTitleInternal(); }

		inline static graphics::api::Swapchain* GetSwapchain() { return s_Instance->GetSwapchainInternal();}
	};
}

