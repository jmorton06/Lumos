#include "Precompiled.h"
#include "Sprite.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/GraphicsContext.h"

namespace Lumos
{
    namespace Graphics
    {
        Sprite::Sprite(const Vec2& position, const Vec2& scale, const Vec4& colour)
        {
            m_Position = position;
            m_Scale    = scale;
            m_Colour   = colour;
            m_UVs      = GetDefaultUVs();
            m_Texture  = nullptr;
        }

        Sprite::Sprite(const SharedPtr<Texture2D>& texture, const Vec2& position, const Vec2& scale, const Vec4& colour)
        {
            m_Texture  = texture;
            m_Position = position;
            m_Scale    = scale;
            m_Colour   = colour;
            m_UVs      = GetDefaultUVs();
        }

        Sprite::~Sprite()
        {
        }

        void Sprite::SetSpriteSheet(const Vec2& index, const Vec2& cellSize, const Vec2& spriteSize, float boarder /* = 0.0f*/)
        {
            if(!m_Texture)
                return;

            Vec2 min = { (index.x * (cellSize.x + boarder)) / m_Texture->GetWidth(), (index.y * (cellSize.y + boarder)) / m_Texture->GetHeight() };
            Vec2 max = { ((index.x * (cellSize.x + boarder)) + spriteSize.x) / m_Texture->GetWidth(), ((index.y * (cellSize.y + boarder)) + spriteSize.y) / m_Texture->GetHeight() };

            m_UVs = GetUVs(min, max);
        }

        void Sprite::SetSpriteSheetIndex(int x, int y)
        {
            if(!m_Texture)
                return;

            Vec2 min = { static_cast<float>((x * (SpriteSheetTileSizeX)) / m_Texture->GetWidth()), static_cast<float>((y * (SpriteSheetTileSizeY)) / m_Texture->GetHeight()) };
            Vec2 max = { static_cast<float>(((x * (SpriteSheetTileSizeX)) + SpriteSheetTileSizeX) / m_Texture->GetWidth()), static_cast<float>(((y * SpriteSheetTileSizeY) + SpriteSheetTileSizeY) / m_Texture->GetHeight()) };

            m_UVs = GetUVs(min, max);
        }

        void Sprite::SetTextureFromFile(const std::string& filePath)
        {
            auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(filePath, filePath));
            if(tex)
            {
                m_Texture = tex;
            }
        }
    }
}
