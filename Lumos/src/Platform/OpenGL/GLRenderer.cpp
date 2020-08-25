#include "lmpch.h"
#include "GLRenderer.h"
#include "Graphics/API/Shader.h"
#include "Core/OS/Window.h"
#include "GLDebug.h"

#include "GL.h"
#include "GLTools.h"
#include "Graphics/Mesh.h"
#include "GLDescriptorSet.h"
#include "Graphics/Material.h"

namespace Lumos
{
	namespace Graphics
	{

		GLRenderer::GLRenderer(u32 width, u32 height)
			: m_Context(nullptr)
		{
			m_Swapchain = new Graphics::GLSwapchain(width, height);

			m_RendererTitle = "OPENGL";

			auto& caps = Renderer::GetCapabilities();

			caps.Vendor = (const char*)glGetString(GL_VENDOR);
			caps.Renderer = (const char*)glGetString(GL_RENDERER);
			caps.Version = (const char*)glGetString(GL_VERSION);

			glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);
			glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &caps.UniformBufferOffsetAlignment);
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
			GLCall(glBlendEquation(GL_FUNC_ADD));

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

		void GLRenderer::ClearInternal(u32 buffer)
		{
			GLCall(glClear(GLTools::RendererBufferToGL(buffer)));
		}

		void GLRenderer::PresentInternal()
		{
		}

		void GLRenderer::PresentInternal(Graphics::CommandBuffer* cmdBuffer)
		{
		}

		void GLRenderer::SetDepthTestingInternal(bool enabled)
		{
			if(enabled)
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
			GLCall(glDepthMask(enabled ? GL_TRUE : GL_FALSE));
		}

		void GLRenderer::SetPixelPackType(const PixelPackType type)
		{
			switch(type)
			{
			case PixelPackType::PACK:
				GLCall(glPixelStorei(GL_PACK_ALIGNMENT, 1));
				break;
			case PixelPackType::UNPACK:
				GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
				break;
			}
		}

		void GLRenderer::SetBlendInternal(bool enabled)
		{
			if(enabled)
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
			GLCall(glBlendFunc(GLTools::RendererBlendFunctionToGL(source), GLTools::RendererBlendFunctionToGL(destination)));
		}

		void GLRenderer::SetBlendEquationInternal(RendererBlendFunction blendEquation)
		{
			LUMOS_ASSERT(false, "Not implemented");
		}

		void GLRenderer::SetViewportInternal(u32 x, u32 y, u32 width, u32 height)
		{
			GLCall(glViewport(x, y, width, height));
		}

		const std::string& GLRenderer::GetTitleInternal() const
		{
			return m_RendererTitle;
		}

		void GLRenderer::SetRenderModeInternal(RenderMode mode)
		{
#ifndef LUMOS_PLATFORM_MOBILE
			switch(mode)
			{
			case RenderMode::FILL:
				GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
				break;
			case RenderMode::WIREFRAME:
				GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
				break;
			}
#endif
		}

		void GLRenderer::OnResize(u32 width, u32 height)
		{
		}

		void GLRenderer::SetCullingInternal(bool enabled, bool front)
		{
			if(enabled)
			{
				GLCall(glEnable(GL_CULL_FACE));
				GLCall(glCullFace(front ? GL_FRONT : GL_BACK));
			}
			else
			{
				GLCall(glDisable(GL_CULL_FACE));
			}
		}

		void GLRenderer::SetStencilTestInternal(bool enabled)
		{
			if(enabled)
			{
				GLCall(glEnable(GL_STENCIL_TEST));
			}
			else
			{
				GLCall(glDisable(GL_STENCIL_TEST));
			}
		}

		void GLRenderer::SetStencilFunctionInternal(const StencilType type, u32 ref, u32 mask)
		{
			glStencilFunc(GLTools::StencilTypeToGL(type), ref, mask);
		}

		void GLRenderer::SetStencilOpInternal(const StencilType fail, const StencilType zfail, const StencilType zpass)
		{
			glStencilOp(GLTools::StencilTypeToGL(fail), GLTools::StencilTypeToGL(zfail), GLTools::StencilTypeToGL(zpass));
		}

		void GLRenderer::SetColourMaskInternal(bool r, bool g, bool b, bool a)
		{
			glColorMask(r, g, b, a);
		}

		void GLRenderer::DrawInternal(CommandBuffer* commandBuffer, const DrawType type, u32 count, DataType dataType, void* indices) const
		{
			GLCall(glDrawElements(GLTools::DrawTypeToGL(type), count, GLTools::DataTypeToGL(dataType), indices));
		}

		void GLRenderer::DrawIndexedInternal(CommandBuffer* commandBuffer, const DrawType type, u32 count, u32 start) const
		{
			GLCall(glDrawElements(GLTools::DrawTypeToGL(type), count, GLTools::DataTypeToGL(DataType::UNSIGNED_INT), nullptr));
			//GLCall(glDrawArrays(GLTools::DrawTypeToGL(type), start, count));
		}

		void GLRenderer::BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
		{
			for(auto descriptor : descriptorSets)
			{
				static_cast<Graphics::GLDescriptorSet*>(descriptor)->Bind(dynamicOffset);
			}
		}

		void GLRenderer::MakeDefault()
		{
			CreateFunc = CreateFuncGL;
		}

		Renderer* GLRenderer::CreateFuncGL(u32 width, u32 height)
		{
			return new GLRenderer(width, height);
		}
	}
}
