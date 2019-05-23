#include "LM.h"
#include "Sprite.h"
#include "Graphics/Material.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Mesh.h"
#include "Utilities/AssetsManager.h"
#include "MeshFactory.h"

namespace lumos
{
	Sprite::Sprite(const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour)
	{
		m_Position = position;
		m_Scale = scale;
		m_Colour = colour;
		m_UVs = GetDefaultUVs();
		m_Texture = nullptr;
	}

	Sprite::Sprite(std::shared_ptr<graphics::Texture2D> texture, const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour)
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
