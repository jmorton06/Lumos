#include "Precompiled.h"
#include "GLRenderer.h"
#include "Graphics/API/Shader.h"
#include "Core/OS/Window.h"
#include "Core/Engine.h"
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

        GLRenderer::GLRenderer(uint32_t width, uint32_t height)
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
            LUMOS_PROFILE_FUNCTION();
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

        void GLRenderer::ClearInternal(uint32_t buffer)
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
            LUMOS_PROFILE_FUNCTION();
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
            LUMOS_PROFILE_FUNCTION();
            GLCall(glDepthMask(enabled ? GL_TRUE : GL_FALSE));
        }

        void GLRenderer::SetPixelPackType(const PixelPackType type)
        {
            LUMOS_PROFILE_FUNCTION();
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
            LUMOS_PROFILE_FUNCTION();
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
            LUMOS_PROFILE_FUNCTION();
            GLCall(glBlendFunc(GLTools::RendererBlendFunctionToGL(source), GLTools::RendererBlendFunctionToGL(destination)));
        }

        void GLRenderer::SetBlendEquationInternal(RendererBlendFunction blendEquation)
        {
            LUMOS_ASSERT(false, "Not implemented");
        }

        void GLRenderer::SetViewportInternal(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glViewport(x, y, width, height));
        }

        const std::string& GLRenderer::GetTitleInternal() const
        {
            return m_RendererTitle;
        }

        void GLRenderer::SetRenderModeInternal(RenderMode mode)
        {
            LUMOS_PROFILE_FUNCTION();
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

        void GLRenderer::OnResize(uint32_t width, uint32_t height)
        {
        }

        void GLRenderer::SetCullingInternal(bool enabled, bool front)
        {
            LUMOS_PROFILE_FUNCTION();
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
            LUMOS_PROFILE_FUNCTION();
            if(enabled)
            {
                GLCall(glEnable(GL_STENCIL_TEST));
            }
            else
            {
                GLCall(glDisable(GL_STENCIL_TEST));
            }
        }

        void GLRenderer::SetStencilFunctionInternal(const StencilType type, uint32_t ref, uint32_t mask)
        {
            LUMOS_PROFILE_FUNCTION();
            glStencilFunc(GLTools::StencilTypeToGL(type), ref, mask);
        }

        void GLRenderer::SetStencilOpInternal(const StencilType fail, const StencilType zfail, const StencilType zpass)
        {
            LUMOS_PROFILE_FUNCTION();
            glStencilOp(GLTools::StencilTypeToGL(fail), GLTools::StencilTypeToGL(zfail), GLTools::StencilTypeToGL(zpass));
        }

        void GLRenderer::SetColourMaskInternal(bool r, bool g, bool b, bool a)
        {
            LUMOS_PROFILE_FUNCTION();
            glColorMask(r, g, b, a);
        }

        void GLRenderer::DrawInternal(CommandBuffer* commandBuffer, const DrawType type, uint32_t count, DataType dataType, void* indices) const
        {
            LUMOS_PROFILE_FUNCTION();
            Engine::Get().Statistics().NumDrawCalls++;
            GLCall(glDrawElements(GLTools::DrawTypeToGL(type), count, GLTools::DataTypeToGL(dataType), indices));
        }

        void GLRenderer::DrawIndexedInternal(CommandBuffer* commandBuffer, const DrawType type, uint32_t count, uint32_t start) const
        {
            LUMOS_PROFILE_FUNCTION();
            Engine::Get().Statistics().NumDrawCalls++;
            GLCall(glDrawElements(GLTools::DrawTypeToGL(type), count, GLTools::DataTypeToGL(DataType::UNSIGNED_INT), nullptr));
            //GLCall(glDrawArrays(GLTools::DrawTypeToGL(type), start, count));
        }

        void GLRenderer::BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, uint32_t dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto descriptor : descriptorSets)
            {
                if(descriptor)
                    static_cast<Graphics::GLDescriptorSet*>(descriptor)->Bind(dynamicOffset);
            }
        }

        void GLRenderer::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        Renderer* GLRenderer::CreateFuncGL(uint32_t width, uint32_t height)
        {
            return new GLRenderer(width, height);
        }
    }
}
