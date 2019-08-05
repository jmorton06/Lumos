#include "LM.h"
#include "Sprite.h"
#include "Graphics/Material.h"
#include "Graphics/API/Texture.h"
#include "Mesh.h"
#include "Utilities/AssetsManager.h"
#include "MeshFactory.h"

namespace Lumos
{
	namespace Graphics
	{
		Sprite::Sprite(const Maths::Vector2& position, const Maths::Vector2& scale, const Maths::Vector4& colour)
		{
			m_Position = position;
			m_Scale = scale;
			m_Colour = colour;
			m_UVs = GetDefaultUVs();
			m_Texture = nullptr;
		}

		Sprite::Sprite(std::shared_ptr<Texture2D> texture, const Maths::Vector2& position, const Maths::Vector2& scale, const Maths::Vector4& colour)
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
}
