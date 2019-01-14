#pragma once
#include "JM.h"
#include "Maths/Vector2.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix4.h"

namespace jm
{
	class Material;
	class Texture2D;
	class Mesh;

	class JM_EXPORT Sprite
	{
	public:
		Sprite(const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour, float colourMix);
		Sprite(std::shared_ptr<Texture2D> texture, const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour, float colourMix);
		virtual ~Sprite();

		virtual void Render(const maths::Matrix4& projMatrix, const maths::Matrix4& viewMatrix) const;

		Texture2D* GetTexture() const { return m_Texture.get(); }
		maths::Vector2 GetPosition()	const { return m_Position; }
		maths::Vector2 GetScale()		const { return m_Scale; }

		std::shared_ptr<Texture2D>	m_Texture;
		maths::Vector2		m_Position;
		maths::Vector2		m_Scale;
		maths::Vector4		m_Colour;
		float				m_ColourMix;
		maths::Matrix4		m_TranslationMatrix;
		maths::Matrix4		m_RotationMatrix;

		Material*   m_Material;

		Mesh*		m_Mesh;
	};
}
