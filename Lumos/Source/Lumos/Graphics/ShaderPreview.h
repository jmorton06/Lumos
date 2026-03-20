#pragma once
#include "Core/Core.h"
#include "Core/Reference.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/DescriptorSet.h"
#include <string>

namespace Lumos
{
    namespace Graphics
    {
        class ShaderPreview
        {
        public:
            ShaderPreview(uint32_t width, uint32_t height);
            ~ShaderPreview();

            bool Compile(const std::string& fragmentSource);
            void Render(float time, float dt, float resX, float resY, float mouseX, float mouseY);
            void RenderAtResolution(uint32_t width, uint32_t height, float time, float mouseX, float mouseY);

            Texture2D* GetOutputTexture() { return m_OutputTexture.get(); }
            Texture2D* GetExportTexture() { return m_ExportTexture.get(); }
            const std::string& GetError() { return m_Error; }
            bool IsCompiled() { return m_Compiled; }

            void SetChannelTexture(uint32_t channel, Texture2D* texture);
            void ClearChannelTexture(uint32_t channel);

        private:
            void CreateOutputTexture(uint32_t width, uint32_t height);
            void RenderToTexture(Texture2D* target, float time, float dt, float resX, float resY, float mouseX, float mouseY, CommandBuffer* cmdBuffer = nullptr);

            struct UBO
            {
                float iTime;
                float iTimeDelta;
                float iResolutionX;
                float iResolutionY;
                float iMouseX;
                float iMouseY;
                float _pad0;
                float _pad1;
            };

            SharedPtr<Texture2D> m_OutputTexture;
            SharedPtr<Texture2D> m_ExportTexture;
            SharedPtr<Texture2D> m_TextureChannels[4];
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
