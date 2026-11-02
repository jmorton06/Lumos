#pragma once
#include "Core/Core.h"
#include "Core/Reference.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/DescriptorSet.h"
#include <string>
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    class MusicStream;

    namespace Graphics
    {
        class ShaderPreview
        {
        public:
            ShaderPreview(uint32_t width, uint32_t height);
            ~ShaderPreview();

            bool Compile(const std::string& fragmentSource);
            // Compiled shaders are kept as <hash of the source>.spv: looked up in `dirs` (//VFS paths
            // work - an app ships its presets baked) before compiling, and written to `writeDir`.
            // Shipped builds have no GLSL compiler, so this is how their shaders get in.
            static void SetSpirvCache(const TDArray<std::string>& dirs, const std::string& writeDir);
            // Resizes the output texture to resX x resY when it changes.
            void Render(float time, float dt, float resX, float resY, float mouseX, float mouseY);
            void RenderAtResolution(uint32_t width, uint32_t height, float time, float mouseX, float mouseY);
            // Renders at width*supersample and box-filters down before writing the PNG.
            bool ExportPNG(const std::string& path, uint32_t width, uint32_t height, float time, uint32_t supersample = 1);

            // Shadertoy iMouse: xy = current, zw = click position (negative once released).
            void SetMouse(float x, float y, float clickX, float clickY) { m_Mouse[0] = x; m_Mouse[1] = y; m_Mouse[2] = clickX; m_Mouse[3] = clickY; }
            void SetFrame(int frame) { m_Frame = frame; }
            // Lines the preview wraps around user code - subtract from compiler line numbers.
            static int GetPrefixLineCount();

            Texture2D* GetOutputTexture() { return m_OutputTexture.get(); }
            Texture2D* GetExportTexture() { return m_ExportTexture.get(); }
            const std::string& GetError() { return m_Error; }
            bool IsCompiled() { return m_Compiled; }

            // raw: not owned, the caller keeps it alive while it's set. Shared: held here too.
            void SetChannelTexture(uint32_t channel, Texture2D* texture);
            void SetChannelTexture(uint32_t channel, const SharedPtr<Texture2D>& texture);
            // Shadertoy music input: a 512x2 texture on `channel` - row 0 FFT, row 1 waveform (red channel).
            void SetAudio(MusicStream* stream, uint32_t channel = 0);
            void ClearChannelTexture(uint32_t channel);
            // Feedback (MilkDrop-style trails): the previous frame's output on `channel`, -1 = off.
            // Two outputs ping-pong, so the one being drawn is never the one sampled.
            void SetFeedbackChannel(int channel);

        private:
            bool CreateFromSpirv(const uint32_t* words, size_t bytes);
            void CreateOutputTexture(uint32_t width, uint32_t height);
            void RenderToTexture(Texture2D* target, float time, float dt, float resX, float resY, float mouseX, float mouseY, CommandBuffer* cmdBuffer = nullptr);

            // std140: vec4s start on 16-byte boundaries
            struct UBO
            {
                float iTime;
                float iTimeDelta;
                float iResolutionX;
                float iResolutionY;
                float iMouse[4];
                float iDate[4];
                float iFrame;
                float iFrameRate;
                float _pad0;
                float _pad1;
            };

            SharedPtr<Texture2D> m_AudioTextures[3]; // ring: the GPU may still be sampling last frame's
            u32 m_AudioIndex = 0;
            float m_Mouse[4] = {};
            int m_Frame      = 0;
            SharedPtr<Texture2D> m_Retired[3]; // resized-away outputs, kept until the GPU is done with them
            u32 m_RetiredIndex = 0;

            SharedPtr<Texture2D> m_OutputTexture;
            SharedPtr<Texture2D> m_FeedbackTexture; // the other half of the ping-pong
            int m_FeedbackChannel = -1;
            SharedPtr<Texture2D> m_ExportTexture;
            SharedPtr<Texture2D> m_TextureChannels[4];
            Texture2D* m_RawChannels[4] = {}; // SetChannelTexture(raw): borrowed
            SharedPtr<Shader> m_Shader;
            SharedPtr<DescriptorSet> m_DescriptorSet;
            CommandBuffer* m_ExportCommandBuffer = nullptr;
            std::string m_Error;
            bool m_Compiled = false;
            uint32_t m_Width;
            uint32_t m_Height;
        };
    }
}
