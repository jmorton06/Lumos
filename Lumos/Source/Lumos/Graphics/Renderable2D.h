#pragma once
#include "Maths/Maths.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Texture.h"

#define RENDERER2D_VERTEX_SIZE sizeof(VertexData)

namespace Lumos
{
    namespace Graphics
    {
        struct LUMOS_EXPORT VertexData
        {
            Maths::Vector3 vertex;
            Maths::Vector2 uv;
            Maths::Vector2 tid;
            Maths::Vector4 colour;

            bool operator==(const VertexData& other) const
            {
                return vertex == other.vertex && uv == other.uv && tid == other.tid && colour == other.colour;
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
            const std::array<Maths::Vector2, 4>& GetUVs() const { return m_UVs; }

            static const std::array<Maths::Vector2, 4>& GetDefaultUVs();
            static const std::array<Maths::Vector2, 4>& GetUVs(const Maths::Vector2& min, const Maths::Vector2& max);

        protected:
            SharedRef<Texture2D> m_Texture;
            Maths::Vector2 m_Position;
            Maths::Vector2 m_Scale;
            Maths::Vector4 m_Colour;
            std::array<Maths::Vector2, 4> m_UVs;
        };
    }
}
