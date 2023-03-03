#include "Precompiled.h"
#include "Sprite.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/GraphicsContext.h"

namespace Lumos
{
    namespace Graphics
    {
        Sprite::Sprite(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& colour)
        {
            m_Position = position;
            m_Scale    = scale;
            m_Colour   = colour;
            m_UVs      = GetDefaultUVs();
            m_Texture  = nullptr;
        }

        Sprite::Sprite(const SharedPtr<Texture2D>& texture, const glm::vec2& position, const glm::vec2& scale, const glm::vec4& colour)
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

        void Sprite::SetSpriteSheet(const glm::vec2& index, const glm::vec2& cellSize, const glm::vec2& spriteSize, float boarder /* = 0.0f*/)
        {
            if(!m_Texture)
                return;

            glm::vec2 min = { (index.x * (cellSize.x + boarder)) / m_Texture->GetWidth(), (index.y * (cellSize.y + boarder)) / m_Texture->GetHeight() };
            glm::vec2 max = { ((index.x * (cellSize.x + boarder)) + spriteSize.x) / m_Texture->GetWidth(), ((index.y * (cellSize.y + boarder)) + spriteSize.y) / m_Texture->GetHeight() };

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
