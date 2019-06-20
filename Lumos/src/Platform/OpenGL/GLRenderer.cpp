#include "LM.h"
#include "GLRenderer.h"
#include "Graphics/API/Shader.h"
#include "App/Window.h"
#include "GLDebug.h"

#include "GL.h"
#include "Graphics/Mesh.h"
#include "GLDescriptorSet.h"
#include "Graphics/Material.h"

namespace Lumos
{
	namespace Graphics
	{
		const GLenum drawbuffers_1[1] = { GL_COLOR_ATTACHMENT0 };
		const GLenum drawbuffers_2[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		const GLenum drawbuffers_3[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		const GLenum drawbuffers_4[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		const GLenum drawbuffers_5[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };

		GLRenderer::GLRenderer(uint width, uint height) : m_Context(nullptr)
		{
			m_Swapchain = new Graphics::GLSwapchain(width, height);

			m_RendererTitle = "OPENGL";
		}

		GLRenderer::~GLRenderer()
		{
			delete m_Swapchain;
		}

		void GLRenderer::InitInternal()
		{
			GLCall(glEnable(GL_DEPTH_TEST));
			GLCall(glEnable(GL_STENCIL_TEST));
			GLCall(glEnable(GL_CULL_FACE));
			GLCall(glEnable(GL_BLEND));
			GLCall(glDepthFunc(GL_LEQUAL));
			GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
#ifndef LUMOS_PLATFORM_MOBILE
			GLCall(glEnable(GL_DEPTH_CLAMP));
			GLCall(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
#endif
		}

		void GLRenderer::Begin()
		{
			GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
			GLCall(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
			GLCall(glClear(GL_COLOR_BUFFER_BIT));
		}

		void GLRenderer::BindScreenFBOInternal()
		{
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}

		void GLRenderer::ClearInternal(uint buffer)
		{
			GLCall(glClear(RendererBufferToGL(buffer)));
		}

		void GLRenderer::PresentInternal()
		{
		}

		void GLRenderer::PresentInternal(Graphics::CommandBuffer* cmdBuffer)
		{
		}

		void GLRenderer::SetDepthTestingInternal(bool enabled)
		{
			if (enabled)
			{
				GLCall(glEnable(GL_DEPTH_TEST));
			}
			else
			{
				GLCall(glDisable(GL_DEPTH_TEST));
			}
		}

		void GLRenderer::SetDepthMaskInternal(bool enabled)
		{
			if (enabled)
			{
				GLCall(glDepthMask(GL_TRUE));
			}
			else
			{
				GLCall(glDepthMask(GL_FALSE));
			}
		}

		void GLRenderer::SetPixelPackType(const PixelPackType type)
		{
			switch (type)
			{
			case PixelPackType::PACK:  GLCall(glPixelStorei(GL_PACK_ALIGNMENT, 1)); break;
			case PixelPackType::UNPACK: GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1)); break;
			}
		}

		void GLRenderer::SetBlendInternal(bool enabled)
		{
			if (enabled)
			{
				GLCall(glEnable(GL_BLEND));
			}
			else
			{
				GLCall(glDisable(GL_BLEND));
			}
		}

		void GLRenderer::SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination)
		{
			GLCall(glBlendFunc(RendererBlendFunctionToGL(source), RendererBlendFunctionToGL(destination)));
		}

		void GLRenderer::SetBlendEquationInternal(RendererBlendFunction blendEquation)
		{
			LUMOS_CORE_ASSERT(false, "Not implemented");
		}

		void GLRenderer::SetViewportInternal(uint x, uint y, uint width, uint height)
		{
			GLCall(glViewport(x, y, width, height));
		}

		const String& GLRenderer::GetTitleInternal() const
		{
			return m_RendererTitle;
		}

		uint GLRenderer::RendererBufferToGL(uint buffer)
		{
			uint result = 0;
			if (buffer & RENDERER_BUFFER_COLOUR)
				result |= GL_COLOR_BUFFER_BIT;
			if (buffer & RENDERER_BUFFER_DEPTH)
				result |= GL_DEPTH_BUFFER_BIT;
			if (buffer & RENDERER_BUFFER_STENCIL)
				result |= GL_STENCIL_BUFFER_BIT;
			return result;
		}

		uint GLRenderer::RendererBlendFunctionToGL(RendererBlendFunction function)
		{
			switch (function)
			{
			case RendererBlendFunction::ZERO:						return GL_ZERO;
			case RendererBlendFunction::ONE:						return GL_ONE;
			case RendererBlendFunction::SOURCE_ALPHA:				return GL_SRC_ALPHA;
			case RendererBlendFunction::DESTINATION_ALPHA:			return GL_DST_ALPHA;
			case RendererBlendFunction::ONE_MINUS_SOURCE_ALPHA:		return GL_ONE_MINUS_SRC_ALPHA;
			default: return 0;
			}
		}

		uint GLRenderer::DataTypeToGL(DataType dataType)
		{
			switch (dataType)
			{
			case DataType::FLOAT: return GL_FLOAT;
			case DataType::UNSIGNED_INT: return GL_UNSIGNED_INT;
			case DataType::UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
			default: LUMOS_CORE_ERROR("Unsupported DataType"); break;
			}
			return 0;
		}

		uint GLRenderer::DrawTypeToGL(DrawType drawType)
		{
			switch (drawType)
			{
			case DrawType::POINT:    return GL_POINTS;
			case DrawType::LINES:    return GL_LINES;
			case DrawType::TRIANGLE: return GL_TRIANGLES;
			default: LUMOS_CORE_ERROR("Unsupported DrawType"); break;
			}
			return 0;
		}

		void GLRenderer::DrawInternal(const DrawType type, uint count, DataType dataType, void* indices) const
		{
			GLCall(glDrawElements(DrawTypeToGL(type), count, DataTypeToGL(dataType), indices));
		}

		void GLRenderer::DrawArraysInternal(const DrawType type, uint numIndices) const
		{
			GLCall(glDrawArrays(DrawTypeToGL(type), 0, numIndices));
		}
		void GLRenderer::DrawArraysInternal(const DrawType type, uint start, uint numIndices) const
		{
			GLCall(glDrawArrays(DrawTypeToGL(type), start, numIndices));
		}

		void GLRenderer::SetRenderModeInternal(RenderMode mode)
		{
#ifndef LUMOS_PLATFORM_MOBILE
			switch (mode)
			{
			case RenderMode::FILL: GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)); break;
			case RenderMode::WIREFRAME: GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)); break;
			}
#endif
		}

		void GLRenderer::OnResize(uint width, uint height)
		{
		}

		void GLRenderer::SetRenderTargets(uint numTargets)
		{
			switch (numTargets)
			{
			case 0: GLCall(glDrawBuffers(0, GL_NONE)); break;
			case 1: GLCall(glDrawBuffers(1, drawbuffers_1)); break;
			case 2: GLCall(glDrawBuffers(2, drawbuffers_2)); break;
			case 3: GLCall(glDrawBuffers(3, drawbuffers_3)); break;
			case 4: GLCall(glDrawBuffers(4, drawbuffers_4)); break;
			case 5: GLCall(glDrawBuffers(5, drawbuffers_5)); break;
			default: LUMOS_CORE_ERROR("Unsupported amount of render targets"); break;
			}
		}

		void GLRenderer::SetCullingInternal(bool enabled, bool front)
		{
			if (enabled)
			{
				GLCall(glEnable(GL_CULL_FACE));
				if (front)
				{
					GLCall(glCullFace(GL_FRONT));
				}
				else
				{
					GLCall(glCullFace(GL_BACK));
				}

			}
			else
			{
				GLCall(glDisable(GL_CULL_FACE));
			}
		}

		void GLRenderer::SetStencilTestInternal(bool enabled)
		{
			if (enabled)
			{
				GLCall(glEnable(GL_STENCIL_TEST));
			}
			else
			{
				GLCall(glDisable(GL_STENCIL_TEST));
			}
		}

		uint StencilTypeToGL(const StencilType type)
		{
			switch (type)
			{
			case StencilType::EQUAL:	return GL_EQUAL;
			case StencilType::NOTEQUAL:	return GL_NOTEQUAL;
			case StencilType::KEEP:		return GL_KEEP;
			case StencilType::REPLACE:	return GL_REPLACE;
			case StencilType::ZERO:		return GL_ZERO;
			case StencilType::ALWAYS:	return GL_ALWAYS;
			}
			return 0;
		}
		void GLRenderer::SetStencilFunctionInternal(const StencilType type, uint ref, uint mask)
		{
			glStencilFunc(StencilTypeToGL(type), ref, mask);
		}

		void GLRenderer::SetStencilOpInternal(const StencilType fail, const StencilType zfail, const StencilType zpass)
		{
			glStencilOp(StencilTypeToGL(fail), StencilTypeToGL(zfail), StencilTypeToGL(zpass));
		}

		void GLRenderer::SetColourMaskInternal(bool r, bool g, bool b, bool a)
		{
			glColorMask(r, g, b, a);
		}

		void GLRenderer::RenderMeshInternal(Mesh *mesh, Graphics::Pipeline *pipeline, Graphics::CommandBuffer* cmdBuffer, uint dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
		{
			for (auto desc : descriptorSets)
			{
				static_cast<Graphics::GLDescriptorSet*>(desc)->Bind(dynamicOffset);
			}

			mesh->Draw();
		}

		void GLRenderer::Render(VertexArray* vertexArray, IndexBuffer* indexBuffer, Graphics::CommandBuffer* cmdBuffer,
			std::vector<Graphics::DescriptorSet*>& descriptorSets, Graphics::Pipeline* pipeline, uint dynamicOffset)
		{
			for (auto descriptor : descriptorSets)
			{
				static_cast<Graphics::GLDescriptorSet*>(descriptor)->Bind(dynamicOffset);
			}

			vertexArray->Bind();
			indexBuffer->Bind();
			Renderer::Draw(DrawType::TRIANGLE, indexBuffer->GetCount());
			indexBuffer->Unbind();
			vertexArray->Unbind();
		}
	}
}
