#pragma once
#include "Renderable2D.h"
#include "Core/OS/FileSystem.h"

namespace Lumos
{
    namespace Graphics
    {
        class Texture2D;

        class LUMOS_EXPORT Sprite : public Renderable2D
        {
            template <typename Archive>
            friend void save(Archive& archive, const Sprite& sprite);

            template <typename Archive>
            friend void load(Archive& archive, Sprite& sprite);

        public:
            Sprite(const Vec2& position = Vec2(0.0f, 0.0f), const Vec2& scale = Vec2(1.0f, 1.0f), const Vec4& colour = Vec4(1.0f));
            Sprite(const SharedPtr<Texture2D>& texture, const Vec2& position, const Vec2& scale, const Vec4& colour);
            virtual ~Sprite();
            void SetPosition(const Vec2& vector2) { m_Position = vector2; };
            void SetColour(const Vec4& colour) { m_Colour = colour; }
            void SetScale(const Vec2& scale) { m_Scale = scale; }

            void SetSpriteSheetIndex(int x, int y);
            void SetSpriteSheet(const Vec2& index, const Vec2& cellSize, const Vec2& spriteSize, float boarder = 0.0f);
            void SetTexture(const SharedPtr<Texture2D>& texture) { m_Texture = texture; }

            void SetTextureFromFile(const std::string& filePath);

            bool UsingSpriteSheet        = false;
            uint32_t SpriteSheetTileSize = 32;
        };
    }
}
