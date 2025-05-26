#include "Precompiled.h"
#include "VKFunctions.h"
#include "VKCommandBuffer.h"
#include "VKContext.h"
#include "VKDescriptorSet.h"
#include "VKFramebuffer.h"
#include "VKIMGUIRenderer.h"
#include "VKIndexBuffer.h"
#include "VKPipeline.h"
#include "VKRenderer.h"
#include "VKRenderPass.h"
#include "VKShader.h"
#include "VKSwapChain.h"
#include "VKTexture.h"
#include "VKUniformBuffer.h"
#include "VKVertexBuffer.h"

void Lumos::Graphics::Vulkan::MakeDefault()
{
    VKCommandBuffer::MakeDefault();
    VKContext::MakeDefault();
    VKDescriptorSet::MakeDefault();
    VKFramebuffer::MakeDefault();
    VKIMGUIRenderer::MakeDefault();
    VKIndexBuffer::MakeDefault();
    VKPipeline::MakeDefault();
    VKRenderer::MakeDefault();
    VKRenderPass::MakeDefault();
    VKShader::MakeDefault();
    VKSwapChain::MakeDefault();
    VKTexture2D::MakeDefault();
    VKTextureCube::MakeDefault();
    VKTextureDepth::MakeDefault();
    VKTextureDepthArray::MakeDefault();
    VKUniformBuffer::MakeDefault();
    VKVertexBuffer::MakeDefault();
}
