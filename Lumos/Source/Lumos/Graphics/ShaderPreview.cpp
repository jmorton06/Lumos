#include "Precompiled.h"
#include "ShaderPreview.h"
#include "ShaderCompiler.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/CommandBuffer.h"

#include "CompiledSPV/Headers/ScreenPassvertspv.hpp"

namespace Lumos
{
    namespace Graphics
    {
        static const std::string s_FragmentPrefix = R"(
#version 450
layout(location = 0) in vec2 outTexCoord;
layout(set = 0, binding = 0) uniform UBO {
    float iTime;
    float iTimeDelta;
    float iResolutionX;
    float iResolutionY;
    float iMouseX;
    float iMouseY;
    float _pad0;
    float _pad1;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D iChannel0;
layout(set = 0, binding = 2) uniform sampler2D iChannel1;
layout(set = 0, binding = 3) uniform sampler2D iChannel2;
layout(set = 0, binding = 4) uniform sampler2D iChannel3;

// Convenience aliases
#define iResolution vec2(ubo.iResolutionX, ubo.iResolutionY)
#define iTime ubo.iTime
#define iTimeDelta ubo.iTimeDelta
#define iMouse vec4(ubo.iMouseX, ubo.iMouseY, 0.0, 0.0)

layout(location = 0) out vec4 outFrag;

)";

        static const std::string s_FragmentSuffix = R"(

void main() {
    vec2 fragCoord = outTexCoord * vec2(ubo.iResolutionX, ubo.iResolutionY);
    mainImage(outFrag, fragCoord);
    outFrag.w = 1.0;
}
)";

        ShaderPreview::ShaderPreview(uint32_t width, uint32_t height)
            : m_Width(width)
            , m_Height(height)
        {
            CreateOutputTexture(width, height);
        }

        ShaderPreview::~ShaderPreview()
        {
            if(m_ExportCommandBuffer)
            {
                m_ExportCommandBuffer->Flush();
                delete m_ExportCommandBuffer;
            }
        }

        void ShaderPreview::CreateOutputTexture(uint32_t width, uint32_t height)
        {
            TextureDesc desc;
            desc.format    = RHIFormat::R8G8B8A8_Unorm;
            desc.flags     = TextureFlags::Texture_RenderTarget | TextureFlags::Texture_Sampled;
            desc.wrap      = TextureWrap::CLAMP_TO_EDGE;
            desc.minFilter = TextureFilter::LINEAR;
            desc.magFilter = TextureFilter::LINEAR;
            desc.generateMipMaps = false;
            m_OutputTexture = SharedPtr<Texture2D>(Texture2D::Create(desc, width, height));
        }

        bool ShaderPreview::Compile(const std::string& fragmentSource)
        {
            m_Compiled = false;
            m_Error.clear();

            if(!ShaderCompiler::IsAvailable())
            {
                m_Error = "Shader compiler not available on this platform";
                LERROR("ShaderPreview: %s", m_Error.c_str());
                return false;
            }

            std::string fullSource = s_FragmentPrefix + fragmentSource + s_FragmentSuffix;
            LINFO("ShaderPreview: compiling %zu bytes of GLSL", fullSource.size());
            auto result = ShaderCompiler::Compile(fullSource, ShaderType::FRAGMENT, "preview.frag");

            if(!result.success)
            {
                m_Error = result.error;
                LERROR("ShaderPreview compile error: %s", m_Error.c_str());
                return false;
            }
            LINFO("ShaderPreview: SPIR-V generated, %zu words", result.spirv.size());

            m_Shader = SharedPtr<Shader>(Shader::CreateFromEmbeddedArray(
                spirv_ScreenPassvertspv.data(),
                spirv_ScreenPassvertspv_size,
                result.spirv.data(),
                (uint32_t)(result.spirv.size() * sizeof(uint32_t))));

            if(!m_Shader)
            {
                m_Error = "Failed to create shader from SPIR-V";
                return false;
            }

            DescriptorDesc descDesc {};
            descDesc.layoutIndex = 0;
            descDesc.shader      = m_Shader.get();
            m_DescriptorSet      = SharedPtr<DescriptorSet>(DescriptorSet::Create(descDesc));

            m_Compiled = true;
            return true;
        }

        void ShaderPreview::SetChannelTexture(uint32_t channel, Texture2D* texture)
        {
            if(channel < 4)
                m_TextureChannels[channel] = SharedPtr<Texture2D>(texture);
        }

        void ShaderPreview::ClearChannelTexture(uint32_t channel)
        {
            if(channel < 4)
                m_TextureChannels[channel] = nullptr;
        }

        void ShaderPreview::RenderToTexture(Texture2D* target, float time, float dt, float resX, float resY, float mouseX, float mouseY, CommandBuffer* cmdBuffer)
        {
            if(!m_Compiled || !m_Shader || !m_DescriptorSet || !target)
                return;

            auto commandBuffer = cmdBuffer ? cmdBuffer : Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            if(!commandBuffer)
                return;

            commandBuffer->UnBindPipeline();

            UBO ubo;
            ubo.iTime        = time;
            ubo.iTimeDelta   = dt;
            ubo.iResolutionX = resX;
            ubo.iResolutionY = resY;
            ubo.iMouseX      = mouseX;
            ubo.iMouseY      = mouseY;
            ubo._pad0        = 0.0f;
            ubo._pad1        = 0.0f;

            m_DescriptorSet->SetUniformBufferData(0, &ubo);

            auto defaultTex = Material::GetDefaultTexture();
            for(int i = 0; i < 4; i++)
            {
                auto tex = m_TextureChannels[i] ? m_TextureChannels[i].get() : defaultTex.get();
                m_DescriptorSet->SetTexture((u8)(i + 1), tex);
            }

            m_DescriptorSet->Update();

            PipelineDesc pipelineDesc {};
            pipelineDesc.shader              = m_Shader;
            pipelineDesc.polygonMode         = PolygonMode::FILL;
            pipelineDesc.cullMode            = CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.colourTargets[0]    = target;
            pipelineDesc.DebugName           = "ShaderPreview";
            pipelineDesc.DepthTest           = false;
            pipelineDesc.DepthWrite          = false;

            auto pipeline = Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            auto set = m_DescriptorSet.get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
            Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

            commandBuffer->UnBindPipeline();
        }

        void ShaderPreview::Render(float time, float dt, float resX, float resY, float mouseX, float mouseY)
        {
            RenderToTexture(m_OutputTexture.get(), time, dt, resX, resY, mouseX, mouseY);
        }

        void ShaderPreview::RenderAtResolution(uint32_t width, uint32_t height, float time, float mouseX, float mouseY)
        {
            if(!m_Compiled || !m_Shader)
                return;

            if(!m_ExportTexture || m_ExportTexture->GetWidth() != width || m_ExportTexture->GetHeight() != height)
            {
                TextureDesc desc;
                desc.format      = RHIFormat::R8G8B8A8_Unorm;
                desc.flags       = TextureFlags::Texture_RenderTarget | TextureFlags::Texture_Sampled;
                desc.wrap        = TextureWrap::CLAMP_TO_EDGE;
                desc.minFilter   = TextureFilter::LINEAR;
                desc.magFilter   = TextureFilter::LINEAR;
                desc.generateMipMaps = false;
                m_ExportTexture = SharedPtr<Texture2D>(Texture2D::Create(desc, width, height));
            }

            if(!m_ExportCommandBuffer)
            {
                m_ExportCommandBuffer = CommandBuffer::Create();
                m_ExportCommandBuffer->Init(true);
            }

            m_ExportCommandBuffer->BeginRecording();
            RenderToTexture(m_ExportTexture.get(), time, 0.0f, (float)width, (float)height, mouseX, mouseY, m_ExportCommandBuffer);
            m_ExportCommandBuffer->EndRecording();
            m_ExportCommandBuffer->Submit();
            m_ExportCommandBuffer->Flush();
        }
    }
}
