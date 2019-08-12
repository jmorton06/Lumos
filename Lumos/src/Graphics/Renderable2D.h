#pragma once
#include "LM.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"

#include "Graphics/API/DescriptorSet.h"

#define RENDERER2D_VERTEX_SIZE	sizeof(VertexData)

namespace Lumos
{
	namespace Graphics
	{
		class Texture2D;

		struct LUMOS_EXPORT VertexData
		{
			Maths::Vector3 vertex;
			Maths::Vector2 uv;
			Maths::Vector2 tid;
			Maths::Vector4 color;

			bool operator==(const VertexData& other) const
			{
				return vertex == other.vertex  && uv == other.uv && tid == other.tid && color == other.color;
			}

			static std::array<Graphics::VertexInputDescription, 5> getAttributeDescriptions()
			{
				std::array<Graphics::VertexInputDescription, 5> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = Graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[0].offset = offsetof(VertexData, vertex);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = Graphics::Format::R32G32_FLOAT;
				attributeDescriptions[1].offset = offsetof(VertexData, uv);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = Graphics::Format::R32G32_FLOAT;
				attributeDescriptions[2].offset = offsetof(VertexData, tid);

				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 3;
				attributeDescriptions[4].format = Graphics::Format::R32G32B32A32_FLOAT;
				attributeDescriptions[4].offset = offsetof(VertexData, color);

				return attributeDescriptions;
			}
		};

		class LUMOS_EXPORT Renderable2D
		{
		public:
			Renderable2D();
			virtual ~Renderable2D();

			Texture2D* GetTexture() const { return m_Texture.get(); }
			Maths::Vector2 GetPosition() const { return m_Position; }
			Maths::Vector2 GetScale() const { return m_Scale; }
			const Maths::Vector4& GetColour() const { return m_Colour; }
			const std::vector<Maths::Vector2>& GetUVs() const { return m_UVs; }

			static const std::vector<Maths::Vector2>& GetDefaultUVs();

		protected:
			Ref<Texture2D> m_Texture;
			Maths::Vector2 m_Position;
			Maths::Vector2 m_Scale;
			Maths::Vector4 m_Colour;
			std::vector<Maths::Vector2> m_UVs;
		};
	}
}
