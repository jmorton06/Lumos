#include "LM.h"
#include "Sprite.h"
#include "Graphics/Material.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Mesh.h"
#include "Utilities/AssetsManager.h"
#include "MeshFactory.h"

namespace Lumos
{
	Sprite::Sprite(const maths::Vector2& position, const maths::Vector2& scale, uint colour)
	{
		m_Position = position;
		m_Scale = scale;
		m_Colour = colour;
		m_UVs = GetDefaultUVs();
	}

	Sprite::Sprite(std::shared_ptr<Texture2D> texture, const maths::Vector2& position, const maths::Vector2& scale, uint colour)
	{
		m_Texture = texture;
		m_Position = position;
		m_Scale = scale;
		m_Colour = colour;
		m_UVs = GetDefaultUVs();
	}

	Sprite::~Sprite()
	{
	}
}
