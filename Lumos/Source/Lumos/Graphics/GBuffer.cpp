#include "Precompiled.h"
#include "GBuffer.h"
#include "RHI/Framebuffer.h"
#include "RHI/Texture.h"

namespace Lumos
{
    namespace Graphics
    {
        GBuffer::GBuffer(uint32_t width, uint32_t height)
            : m_Width(width)
            , m_Height(height)
        {
            Init();
        }

        GBuffer::~GBuffer()
        {
            for(auto& m_Texture : m_ScreenTex)
            {
                delete m_Texture;
            }

            delete m_DepthTexture;
        }

        void GBuffer::UpdateTextureSize(uint32_t width, uint32_t height)
        {
            m_Width  = width;
            m_Height = height;

            BuildTextures();
        }

        void GBuffer::Init()
        {
            for(auto& texture : m_ScreenTex)
            {
                texture = nullptr;
            }

            m_DepthTexture = nullptr;

            BuildTextures();
        }

        void GBuffer::BuildTextures()
        {
            if(!m_ScreenTex[0])
            {
                for(auto& m_Texture : m_ScreenTex)
                {
                    // m_Texture = Texture2D::Create();
                }

                m_DepthTexture = TextureDepth::Create(m_Width, m_Height);
            }

#ifdef LUMOS_PLATFORM_IOS
            // Unless all render targets were rgba32 there were visual glitches on ios
            m_Formats[0] = RHIFormat::R32G32B32A32_Float;
            m_Formats[1] = RHIFormat::R32G32B32A32_Float;
            m_Formats[2] = RHIFormat::R32G32B32A32_Float;
            m_Formats[3] = RHIFormat::R32G32B32A32_Float;
            m_Formats[4] = RHIFormat::R32G32B32A32_Float;
#else
            m_Formats[0] = RHIFormat::R8G8B8A8_Unorm;
            m_Formats[1] = RHIFormat::R32G32B32A32_Float;
            m_Formats[2] = RHIFormat::R16G16B16A16_Float;
            m_Formats[3] = RHIFormat::R16G16B16A16_Float;
            m_Formats[4] = RHIFormat::R8G8B8A8_Unorm;
#endif
            /*
                        m_ScreenTex[SCREENTEX_COLOUR]->BuildTexture(m_Formats[0], m_Width, m_Height, false, false, false);
                        m_ScreenTex[SCREENTEX_POSITION]->BuildTexture(m_Formats[1], m_Width, m_Height, false, false, false);
                        m_ScreenTex[SCREENTEX_NORMALS]->BuildTexture(m_Formats[2], m_Width, m_Height, false, false, false);
                        m_ScreenTex[SCREENTEX_PBR]->BuildTexture(m_Formats[3], m_Width, m_Height, false, false, false);
                        m_ScreenTex[SCREENTEX_OFFSCREEN0]->BuildTexture(m_Formats[4], m_Width, m_Height, false, false, false);
            */
            m_DepthTexture->Resize(m_Width, m_Height);
        }

        void GBuffer::Bind(int32_t mode)
        {
        }

        void GBuffer::SetReadBuffer(ScreenTextures type)
        {
        }
    }
}
