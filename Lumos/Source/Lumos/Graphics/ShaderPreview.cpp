#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include "ShaderPreview.h"
#include "ShaderCompiler.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/CommandBuffer.h"

#include "Audio/MusicStream.h"
#include "Utilities/ImageExport.h"
#include "Utilities/LoadImage.h"

#include "CompiledSPV/Headers/ScreenPassvertspv.hpp"

#include <ctime>
#include <filesystem>
#include <stb_image_resize2.h>

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
    vec4 iMouse;
    vec4 iDate;
    float iFrame;
    float iFrameRate;
    float _pad0;
    float _pad1;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D iChannel0;
layout(set = 0, binding = 2) uniform sampler2D iChannel1;
layout(set = 0, binding = 3) uniform sampler2D iChannel2;
layout(set = 0, binding = 4) uniform sampler2D iChannel3;

// Shadertoy names
#define iResolution vec3(ubo.iResolutionX, ubo.iResolutionY, 1.0)
#define iTime ubo.iTime
#define iTimeDelta ubo.iTimeDelta
#define iMouse ubo.iMouse
#define iDate ubo.iDate
#define iFrame int(ubo.iFrame)
#define iFrameRate ubo.iFrameRate
#define iSampleRate 44100.0
#define iChannelTime vec4(ubo.iTime)

layout(location = 0) out vec4 outFrag;

)";

        static const std::string s_FragmentSuffix = R"(

void main() {
    // Shadertoy's origin is bottom-left; texture row 0 is the top of the image
    vec2 fragCoord = vec2(outTexCoord.x, 1.0 - outTexCoord.y) * vec2(ubo.iResolutionX, ubo.iResolutionY);
    mainImage(outFrag, fragCoord);
    outFrag.w = 1.0;
}
)";

        int ShaderPreview::GetPrefixLineCount()
        {
            int lines = 0;
            for(char c : s_FragmentPrefix)
                lines += c == '\n';
            return lines;
        }

        // "preview.frag:123: error: ..." -> "line 110: error: ..." relative to the user's code
        static std::string RemapErrorLines(const std::string& error, const char* file, int offset)
        {
            std::string out;
            std::string tag = std::string(file) + ":";
            size_t pos      = 0;
            while(pos < error.size())
            {
                size_t at = error.find(tag, pos);
                if(at == std::string::npos)
                {
                    out += error.substr(pos);
                    break;
                }
                out += error.substr(pos, at - pos);
                size_t num = at + tag.size();
                size_t end = num;
                while(end < error.size() && isdigit((unsigned char)error[end]))
                    end++;
                if(end > num)
                {
                    int line = atoi(error.substr(num, end - num).c_str()) - offset;
                    out += "line " + std::to_string(line < 1 ? 1 : line);
                    pos = end;
                }
                else
                {
                    out += "shader:";
                    pos = num;
                }
            }
            return out;
        }

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

        void ShaderPreview::SetFeedbackChannel(int channel)
        {
            if(m_FeedbackChannel >= 0 && m_FeedbackChannel < 4 && channel != m_FeedbackChannel)
                m_TextureChannels[m_FeedbackChannel] = nullptr;
            m_FeedbackChannel = (channel >= 0 && channel < 4) ? channel : -1;
            if(m_FeedbackChannel < 0)
                m_FeedbackTexture = nullptr;
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

        namespace
        {
            TDArray<std::string> s_SpirvDirs;
            std::string s_SpirvWriteDir;

            std::string SpirvName(const std::string& source)
            {
                u64 h = 1469598103934665603ull; // FNV-1a
                for(unsigned char c : source)
                {
                    h ^= c;
                    h *= 1099511628211ull;
                }
                char buf[32];
                snprintf(buf, sizeof(buf), "%016llx.spv", (unsigned long long)h);
                return buf;
            }

            bool ReadAny(const std::string& path, TDArray<uint8_t>& out)
            {
                ArenaTemp scratch = ScratchBegin(0, 0);
                bool ok           = false;
                String8 p         = Str8StdS(path);
                if(path.size() > 1 && path[0] == '/' && path[1] == '/')
                {
                    if(FileSystem::Get().FileExistsVFS(p))
                    {
                        const int64_t size = FileSystem::Get().GetFileSizeVFS(p);
                        uint8_t* data      = size > 0 ? FileSystem::Get().ReadFileVFS(scratch.arena, p) : nullptr;
                        if(data)
                        {
                            out.Resize((size_t)size);
                            memcpy(out.Data(), data, (size_t)size);
                            ok = true;
                        }
                    }
                }
                else if(FileSystem::FileExists(p))
                {
                    const int64_t size = FileSystem::GetFileSize(p);
                    uint8_t* data      = size > 0 ? FileSystem::ReadFile(scratch.arena, p) : nullptr;
                    if(data)
                    {
                        out.Resize((size_t)size);
                        memcpy(out.Data(), data, (size_t)size);
                        ok = true;
                    }
                }
                ScratchEnd(scratch);
                return ok;
            }
        }

        void ShaderPreview::SetSpirvCache(const TDArray<std::string>& dirs, const std::string& writeDir)
        {
            s_SpirvDirs     = dirs;
            s_SpirvWriteDir = writeDir;
        }

        bool ShaderPreview::CreateFromSpirv(const uint32_t* words, size_t bytes)
        {
            m_Shader = SharedPtr<Shader>(Shader::CreateFromEmbeddedArray(
                spirv_ScreenPassvertspv.data(),
                spirv_ScreenPassvertspv_size,
                words,
                (uint32_t)bytes));
            if(!m_Shader)
            {
                m_Error = "Failed to create shader from SPIR-V";
                return false;
            }
            DescriptorDesc descDesc {};
            descDesc.layoutIndex = 0;
            descDesc.shader      = m_Shader.get();
            m_DescriptorSet      = SharedPtr<DescriptorSet>(DescriptorSet::Create(descDesc));
            m_Compiled           = true;
            return true;
        }

        bool ShaderPreview::Compile(const std::string& fragmentSource)
        {
            m_Compiled = false;
            m_Error.clear();

            std::string fullSource = s_FragmentPrefix + fragmentSource + s_FragmentSuffix;
            const std::string name = SpirvName(fullSource);
            for(auto& dir : s_SpirvDirs)
            {
                TDArray<uint8_t> spv;
                if(ReadAny(dir + "/" + name, spv) && spv.Size() >= 20 && spv.Size() % 4 == 0)
                    return CreateFromSpirv((const uint32_t*)spv.Data(), spv.Size());
            }

            if(!ShaderCompiler::IsAvailable())
            {
                m_Error = "Shader compiler not available on this platform";
                LERROR("ShaderPreview: %s", m_Error.c_str());
                return false;
            }

            LINFO("ShaderPreview: compiling %zu bytes of GLSL", fullSource.size());
            auto result = ShaderCompiler::Compile(fullSource, ShaderType::FRAGMENT, "preview.frag");

            if(!result.success)
            {
                m_Error = RemapErrorLines(result.error, "preview.frag", GetPrefixLineCount());
                LERROR("ShaderPreview compile error: %s", m_Error.c_str());
                return false;
            }
            LINFO("ShaderPreview: SPIR-V generated, %zu words", result.spirv.size());
            if(!s_SpirvWriteDir.empty())
            {
                std::string path = s_SpirvWriteDir + "/" + name;
                FILE* f          = fopen(path.c_str(), "wb");
                if(f)
                {
                    fwrite(result.spirv.data(), sizeof(uint32_t), result.spirv.size(), f);
                    fclose(f);
                }
            }
            return CreateFromSpirv(result.spirv.data(), result.spirv.size() * sizeof(uint32_t));
        }

        void ShaderPreview::SetChannelTexture(uint32_t channel, Texture2D* texture)
        {
            // wrapping it in a new SharedPtr started a second count and freed textures others still held
            if(channel < 4)
            {
                m_TextureChannels[channel] = nullptr;
                m_RawChannels[channel]     = texture;
            }
        }

        void ShaderPreview::SetChannelTexture(uint32_t channel, const SharedPtr<Texture2D>& texture)
        {
            if(channel < 4)
            {
                m_TextureChannels[channel] = texture;
                m_RawChannels[channel]     = nullptr;
            }
        }

        void ShaderPreview::SetAudio(MusicStream* stream, uint32_t channel)
        {
            if(channel >= 4)
                return;
            constexpr u32 W = 512;
            float fft[W], wave[W];
            if(stream)
            {
                stream->GetFFT(fft, W);
                stream->GetWaveform(wave, W);
            }
            else
            {
                for(u32 i = 0; i < W; i++)
                    fft[i] = wave[i] = 0.0f;
            }

            u8 pixels[W * 2 * 4];
            for(u32 i = 0; i < W; i++)
            {
                u8 f = (u8)(Maths::Clamp(fft[i], 0.0f, 1.0f) * 255.0f);
                u8 w = (u8)(Maths::Clamp(wave[i] * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f);
                u8* a = &pixels[i * 4];
                u8* b = &pixels[(W + i) * 4];
                a[0] = a[1] = a[2] = f;
                a[3] = 255;
                b[0] = b[1] = b[2] = w;
                b[3] = 255;
            }

            m_AudioIndex = (m_AudioIndex + 1) % 3;
            auto& tex    = m_AudioTextures[m_AudioIndex];
            if(!tex)
            {
                TextureDesc desc;
                desc.format          = RHIFormat::R8G8B8A8_Unorm;
                desc.minFilter       = TextureFilter::LINEAR;
                desc.magFilter       = TextureFilter::LINEAR;
                desc.wrap            = TextureWrap::CLAMP_TO_EDGE;
                desc.generateMipMaps = false;
                desc.flags           = TextureFlags::Texture_Sampled;
                tex                  = SharedPtr<Texture2D>(Texture2D::CreateFromSource(W, 2, pixels, desc));
            }
            else
                tex->SetData(pixels);
            m_TextureChannels[channel] = tex;
        }

        void ShaderPreview::ClearChannelTexture(uint32_t channel)
        {
            if(channel < 4)
            {
                m_TextureChannels[channel] = nullptr;
                m_RawChannels[channel]     = nullptr;
            }
        }

        void ShaderPreview::RenderToTexture(Texture2D* target, float time, float dt, float resX, float resY, float mouseX, float mouseY, CommandBuffer* cmdBuffer)
        {
            if(!m_Compiled || !m_Shader || !m_DescriptorSet || !target)
                return;

            auto commandBuffer = cmdBuffer ? cmdBuffer : Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            if(!commandBuffer)
                return;

            commandBuffer->UnBindPipeline();

            UBO ubo          = {};
            ubo.iTime        = time;
            ubo.iTimeDelta   = dt;
            ubo.iResolutionX = resX;
            ubo.iResolutionY = resY;
            ubo.iMouse[0]    = mouseX;
            ubo.iMouse[1]    = mouseY;
            ubo.iMouse[2]    = m_Mouse[2];
            ubo.iMouse[3]    = m_Mouse[3];
            ubo.iFrame       = (float)m_Frame;
            ubo.iFrameRate   = dt > 0.0f ? 1.0f / dt : 60.0f;

            time_t now = ::time(nullptr);
            tm local   = *localtime(&now);
            ubo.iDate[0] = (float)(local.tm_year + 1900);
            ubo.iDate[1] = (float)local.tm_mon; // 0-based like Shadertoy
            ubo.iDate[2] = (float)local.tm_mday;
            ubo.iDate[3] = (float)(local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec);

            m_DescriptorSet->SetUniformBufferData(0, &ubo);

            auto defaultTex = Material::GetDefaultTexture();
            for(int i = 0; i < 4; i++)
            {
                Texture2D* tex = m_TextureChannels[i] ? m_TextureChannels[i].get() : (m_RawChannels[i] ? m_RawChannels[i] : defaultTex.get());
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
            uint32_t w = (uint32_t)Maths::Max(1.0f, resX);
            uint32_t h = (uint32_t)Maths::Max(1.0f, resY);
            if(m_OutputTexture && (m_OutputTexture->GetWidth() != w || m_OutputTexture->GetHeight() != h))
            {
                m_Retired[m_RetiredIndex] = m_OutputTexture;
                m_RetiredIndex            = (m_RetiredIndex + 1) % 3;
                CreateOutputTexture(w, h);
                m_Width  = w;
                m_Height = h;
            }
            if(m_FeedbackChannel >= 0)
            {
                // draw into the other texture while last frame's output is the feedback input, then swap
                if(!m_FeedbackTexture || m_FeedbackTexture->GetWidth() != w || m_FeedbackTexture->GetHeight() != h)
                {
                    if(m_FeedbackTexture)
                    {
                        m_Retired[m_RetiredIndex] = m_FeedbackTexture;
                        m_RetiredIndex            = (m_RetiredIndex + 1) % 3;
                    }
                    SharedPtr<Texture2D> keep = m_OutputTexture;
                    CreateOutputTexture(w, h);
                    m_FeedbackTexture = m_OutputTexture;
                    m_OutputTexture   = keep;
                }
                m_TextureChannels[m_FeedbackChannel] = m_OutputTexture;
                RenderToTexture(m_FeedbackTexture.get(), time, dt, (float)w, (float)h, mouseX, mouseY);
                std::swap(m_OutputTexture, m_FeedbackTexture);
                m_TextureChannels[m_FeedbackChannel] = nullptr;
                return;
            }
            RenderToTexture(m_OutputTexture.get(), time, dt, (float)w, (float)h, mouseX, mouseY);
        }

        bool ShaderPreview::ExportPNG(const std::string& path, uint32_t width, uint32_t height, float time, uint32_t supersample)
        {
            if(!m_Compiled || width == 0 || height == 0)
                return false;

            supersample = Maths::Clamp(supersample, 1u, 4u);
            // Stay under common max texture size
            while(supersample > 1 && (width * supersample > 16384 || height * supersample > 16384))
                supersample--;

            uint32_t rw = width * supersample, rh = height * supersample;
            RenderAtResolution(rw, rh, time, m_Mouse[0], m_Mouse[1]);
            auto renderer = Renderer::GetRenderer();
            if(!renderer || !m_ExportTexture)
                return false;

            if(supersample == 1)
            {
                renderer->SaveScreenshot(path, m_ExportTexture.get());
                return true;
            }

            // Read back through a temp PNG, then downsample on the CPU
            namespace fs = std::filesystem;
            std::error_code ec;
            std::string tmp = (fs::temp_directory_path(ec) / "lumos_shader_export.png").string();
            renderer->SaveScreenshot(tmp, m_ExportTexture.get());

            uint32_t lw = 0, lh = 0, bits = 0;
            bool hdr     = false;
            uint8_t* big = LoadImageFromFile(tmp.c_str(), &lw, &lh, &bits, &hdr, false, false);
            fs::remove(tmp, ec);
            if(!big)
                return false;

            TDArray<uint8_t> small;
            small.Resize((size_t)width * height * 4);
            bool ok = stbir_resize_uint8_linear(big, (int)lw, (int)lh, 0, small.Data(), (int)width, (int)height, 0, STBIR_RGBA) != nullptr;
            FreeImageMemory(big);
            return ok && ImageExport::SavePNG(path, width, height, small.Data());
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
