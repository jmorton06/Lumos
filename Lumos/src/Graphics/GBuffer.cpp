#include "LM.h"
#include "GBuffer.h"
#include "API/Framebuffer.h"
#include "API/Texture.h"

namespace Lumos
{
	namespace Graphics
	{
		GBuffer::GBuffer(u32 width, u32 height)
			: m_Width(width), m_Height(height)
		{
			Init();
		}

		GBuffer::~GBuffer()
		{
			for (auto& m_Texture : m_ScreenTex)
			{
				delete m_Texture;
			}

			delete m_DepthTexture;
		}

		void GBuffer::UpdateTextureSize(u32 width, u32 height)
		{
			m_Width = width;
			m_Height = height;

			for (auto& texture : m_ScreenTex)
			{
				delete texture;
				texture = nullptr;
			}

			delete m_DepthTexture;
			m_DepthTexture = nullptr;

			BuildTextures();
		}

		void GBuffer::Init()
		{
			for (auto& texture : m_ScreenTex)
			{
				texture = nullptr;
			}

			m_DepthTexture = nullptr;

			BuildTextures();
		}

		void GBuffer::BuildTextures()
		{
			if (!m_ScreenTex[0])
			{
				for (auto& m_Texture : m_ScreenTex)
				{
					m_Texture = Texture2D::Create();
				}

				m_DepthTexture = TextureDepth::Create(m_Width, m_Height);
			}

			m_Formats[0] = TextureFormat::RGBA;
			m_Formats[1] = TextureFormat::RGBA32;
			m_Formats[2] = TextureFormat::RGBA32;
			m_Formats[3] = TextureFormat::RGBA32;
			m_Formats[4] = TextureFormat::RGBA;

			m_ScreenTex[SCREENTEX_COLOUR]->BuildTexture(m_Formats[0], m_Width, m_Height, false, false);
			m_ScreenTex[SCREENTEX_POSITION]->BuildTexture(m_Formats[1], m_Width, m_Height, false, false);
			m_ScreenTex[SCREENTEX_NORMALS]->BuildTexture(m_Formats[2], m_Width, m_Height, false, false);
			m_ScreenTex[SCREENTEX_PBR]->BuildTexture(m_Formats[3], m_Width, m_Height, false, false);
			m_ScreenTex[SCREENTEX_OFFSCREEN0]->BuildTexture(m_Formats[4], m_Width, m_Height, false, false);

			m_DepthTexture->Resize(m_Width, m_Height);
		}

		void GBuffer::Bind(i32 mode)
		{
		}

		void GBuffer::SetReadBuffer(ScreenTextures type)
		{
		}
	}
}
