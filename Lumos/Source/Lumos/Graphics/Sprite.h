#pragma once
#include "Renderable2D.h"
#include "Core/VFS.h"

#include "Scene/Serialisation.h"
#include "Maths/MathsSerialisation.h"

namespace Lumos
{
    namespace Graphics
    {
        class Texture2D;

        class LUMOS_EXPORT Sprite : public Renderable2D
        {
        public:
            Sprite(const glm::vec2& position = glm::vec2(0.0f, 0.0f), const glm::vec2& scale = glm::vec2(1.0f, 1.0f), const glm::vec4& colour = glm::vec4(1.0f));
            Sprite(const SharedPtr<Texture2D>& texture, const glm::vec2& position, const glm::vec2& scale, const glm::vec4& colour);
            virtual ~Sprite();
            void SetPosition(const glm::vec2& vector2) { m_Position = vector2; };
            void SetColour(const glm::vec4& colour) { m_Colour = colour; }
            void SetScale(const glm::vec2& scale) { m_Scale = scale; }

            void SetSpriteSheetIndex(int x, int y);
            void SetSpriteSheet(const glm::vec2& index, const glm::vec2& cellSize, const glm::vec2& spriteSize, float boarder = 0.0f);
            void SetTexture(const SharedPtr<Texture2D>& texture) { m_Texture = texture; }

            void SetTextureFromFile(const std::string& filePath);

            bool UsingSpriteSheet        = false;
            uint32_t SpriteSheetTileSize = 32;

            template <typename Archive>
            void save(Archive& archive) const
            {
                std::string newPath = "";
                if(m_Texture)
                {
                    VFS::Get().AbsoulePathToVFS(m_Texture->GetFilepath(), newPath);
                }

                archive(cereal::make_nvp("TexturePath", newPath),
                        cereal::make_nvp("Position", m_Position),
                        cereal::make_nvp("Scale", m_Scale),
                        cereal::make_nvp("Colour", m_Colour));

                archive(UsingSpriteSheet, SpriteSheetTileSize);
            }

            template <typename Archive>
            void load(Archive& archive)
            {
                std::string textureFilePath;
                archive(cereal::make_nvp("TexturePath", textureFilePath),
                        cereal::make_nvp("Position", m_Position),
                        cereal::make_nvp("Scale", m_Scale),
                        cereal::make_nvp("Colour", m_Colour));

                if(!textureFilePath.empty())
                    m_Texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("sprite", textureFilePath));

                if(Serialisation::CurrentSceneVersion > 21)
                    archive(UsingSpriteSheet, SpriteSheetTileSize);
            }
        };
    }
}
