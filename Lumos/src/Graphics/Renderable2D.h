#pragma once
#include "LM.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"

#include "Graphics/API/DescriptorSet.h"
#include "Maths/Vector4.h"

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
		maths::Vector4 color;

        bool operator==(const VertexData& other) const
        {
            return vertex == other.vertex  && uv == other.uv && tid == other.tid && mid == other.mid && color == other.color;
        }
        
        static std::array<graphics::api::VertexInputDescription, 5> getAttributeDescriptions()
        {
            std::array<graphics::api::VertexInputDescription, 5> attributeDescriptions = {};
            
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = graphics::api::Format::R32G32B32_FLOAT;
            attributeDescriptions[0].offset = offsetof(VertexData, vertex);
            
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = graphics::api::Format::R32G32_FLOAT;
            attributeDescriptions[1].offset = offsetof(VertexData, uv);
            
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = graphics::api::Format::R32_FLOAT;
            attributeDescriptions[2].offset = offsetof(VertexData, tid);
            
            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = graphics::api::Format::R32_FLOAT;
            attributeDescriptions[3].offset = offsetof(VertexData, mid);
            
            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = graphics::api::Format::R32G32B32A32_FLOAT;
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
		maths::Vector2 GetPosition() const { return m_Position; }
		maths::Vector2 GetScale() const { return m_Scale; }
		const maths::Vector4& GetColour() const { return m_Colour; }
		const std::vector<maths::Vector2>& GetUVs() const { return m_UVs; }

		static const std::vector<maths::Vector2>& GetDefaultUVs();

	protected:
		std::shared_ptr<Texture2D> m_Texture;
		maths::Vector2 m_Position;
		maths::Vector2 m_Scale;
		maths::Vector4 m_Colour;
		std::vector<maths::Vector2> m_UVs;
	};
}
