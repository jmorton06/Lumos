#pragma once
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Texture.h"

#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"

#define RENDERER2D_VERTEX_SIZE sizeof(VertexData)
#define RENDERERTEXT_VERTEX_SIZE sizeof(TextVertexData)

namespace Lumos
{
    namespace Graphics
    {
        struct LUMOS_EXPORT VertexData
        {
            Vec3 vertex;
            Vec4 uv;
            Vec2 tid;
            Vec4 colour;

            bool operator==(const VertexData& other) const
            {
                return vertex == other.vertex && uv == other.uv && tid == other.tid && colour == other.colour;
            }
        };

        struct LUMOS_EXPORT TextVertexData
        {
            Vec3 vertex;
            Vec2 uv;
            Vec2 tid;
            Vec4 colour;
            Vec4 outlineColour;

            bool operator==(const TextVertexData& other) const
            {
                return vertex == other.vertex && uv == other.uv && tid == other.tid && colour == other.colour && outlineColour == other.outlineColour;
            }
        };

        class LUMOS_EXPORT Renderable2D
        {
        public:
            Renderable2D();
            virtual ~Renderable2D();

            SharedPtr<Texture2D> GetTexture() { return m_Texture; }
            Vec2 GetPosition() const { return m_Position; }
            Vec2 GetScale() const { return m_Scale; }
            const Vec4& GetColour() const { return m_Colour; }
            const std::array<Vec2, 4>& GetUVs() const { return m_UVs; }

            static const std::array<Vec2, 4>& GetDefaultUVs();
            static const std::array<Vec2, 4>& GetUVs(const Vec2& min, const Vec2& max);

        protected:
            SharedPtr<Texture2D> m_Texture;
            Vec2 m_Position;
            Vec2 m_Scale;
            Vec4 m_Colour;
            std::array<Vec2, 4> m_UVs;
        };
    }
}
