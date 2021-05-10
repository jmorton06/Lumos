#include "Precompiled.h"
#include "GLFunctions.h"
#include "GLCommandBuffer.h"
#include "GLContext.h"
#include "GLDescriptorSet.h"
#include "GLFramebuffer.h"
#include "GLIMGUIRenderer.h"
#include "GLIndexBuffer.h"
#include "GLPipeline.h"
#include "GLRenderDevice.h"
#include "GLRenderer.h"
#include "GLRenderPass.h"
#include "GLShader.h"
#include "GLSwapchain.h"
#include "GLTexture.h"
#include "GLUniformBuffer.h"
#include "GLVertexBuffer.h"

void Lumos::Graphics::GL::MakeDefault()
{
    GLCommandBuffer::MakeDefault();
    GLContext::MakeDefault();
    GLDescriptorSet::MakeDefault();
    GLFramebuffer::MakeDefault();
    GLIMGUIRenderer::MakeDefault();
    GLIndexBuffer::MakeDefault();
    GLPipeline::MakeDefault();
    GLRenderDevice::MakeDefault();
    GLRenderer::MakeDefault();
    GLRenderPass::MakeDefault();
    GLShader::MakeDefault();
    GLSwapchain::MakeDefault();
    GLTexture2D::MakeDefault();
    GLTextureCube::MakeDefault();
    GLTextureDepth::MakeDefault();
    GLTextureDepthArray::MakeDefault();
    GLUniformBuffer::MakeDefault();
    GLVertexBuffer::MakeDefault();
}