#pragma once
#include "LM.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"

#define RENDERER2D_VERTEX_SIZE	sizeof(VertexData)

namespace Lumos
{
	class Texture2D;

	struct LUMOS_EXPORT VertexData
	{
		maths::Vector3 vertex;
		maths::Vector2 uv;
		float tid;
		float mid;
		uint color;
	};

	class LUMOS_EXPORT Renderable2D
	{
	public:
		Renderable2D();
		virtual ~Renderable2D();

		Texture2D* GetTexture() const { return m_Texture.get(); }
		maths::Vector2 GetPosition() const { return m_Position; }
		maths::Vector2 GetScale() const { return m_Scale; }
		uint GetColour() const { return m_Colour; }
		const std::vector<maths::Vector2>& GetUVs() const { return m_UVs; }

		static const std::vector<maths::Vector2>& GetDefaultUVs();

	protected:
		std::shared_ptr<Texture2D> m_Texture;
		maths::Vector2 m_Position;
		maths::Vector2 m_Scale;
		uint m_Colour;
		std::vector<maths::Vector2> m_UVs;
	};
}
