#include "JM.h"
#include "GBuffer.h"
#include "API/FrameBuffer.h"
#include "API/Textures/TextureDepth.h"
#include "API/Textures/Texture2D.h"

namespace jm
{

	GBuffer::GBuffer(uint width, uint height)
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

	void GBuffer::UpdateTextureSize(uint width, uint height)
	{
		m_Width = width;
		m_Height = height;

		BuildTextures();
	}

	void GBuffer::PostProcessDrawn()
	{
		++m_postProcessDrawTo %= PPtextures;
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

		m_ScreenTex[SCREENTEX_COLOUR]->BuildTexture(TextureFormat::RGBA, m_Width, m_Height, false, false);
		m_ScreenTex[SCREENTEX_POSITION]->BuildTexture(TextureFormat::RGB16, m_Width, m_Height, false, false);
		m_ScreenTex[SCREENTEX_NORMALS]->BuildTexture(TextureFormat::RGB16, m_Width, m_Height, false, false);
		m_ScreenTex[SCREENTEX_PBR]->BuildTexture(TextureFormat::RGB16 , m_Width, m_Height, false, false);

        m_DepthTexture->Resize(m_Width, m_Height);

		//m_ScreenTex[SCREENTEX_DEPTH]->BuildTexture(TextureFormat::DEPTH , m_Width, m_Height, true , false);
		//m_ScreenTex[SCREENTEX_POSTPROCESS0]->BuildTexture(TextureFormat::RGBA  , m_Width, m_Height, false, false);
		//m_ScreenTex[SCREENTEX_POSTPROCESS1]->BuildTexture(TextureFormat::RGBA  , m_Width, m_Height, false, false);
	}

	void GBuffer::Bind(int32 mode)
	{
	}

	void GBuffer::SetReadBuffer(ScreenTextures type)
	{
	}
}
