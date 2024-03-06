#pragma once
#include "SoundComponent.h"
#include "TextureMatrixComponent.h"
#include "RigidBody2DComponent.h"
#include "RigidBody3DComponent.h"
#include "AIComponent.h"
#include "Graphics/Font.h"
#include "Core/OS/FileSystem.h"
#include "Scene/Serialisation.h"
#include "Utilities/AssetManager.h"

namespace Lumos
{
    struct TextComponent
    {
        TextComponent()                           = default;
        TextComponent(const TextComponent& other) = default;

        SharedPtr<Graphics::Font> FontHandle;
        std::string TextString  = "Text";
        glm::vec4 Colour        = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 OutlineColour = { 1.0f, 1.0f, 1.0f, 1.0f };
        float LineSpacing       = 0.0f;
        float Kerning           = 0.0f;
        float MaxWidth          = 10.0f;
        float OutlineWidth      = 0.0f;

        void LoadFont(const std::string& filePath)
        {
            FontHandle = CreateSharedPtr<Graphics::Font>(filePath);
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            std::string path;
            FileSystem::Get().AbsolutePathToFileSystem(FontHandle ? FontHandle->GetFilePath() : "", path);

            archive(cereal::make_nvp("TextString", TextString), cereal::make_nvp("Path", path), cereal::make_nvp("Colour", Colour), cereal::make_nvp("LineSpacing", LineSpacing), cereal::make_nvp("Kerning", Kerning),
                    cereal::make_nvp("MaxWidth", MaxWidth), cereal::make_nvp("OutlineColour", OutlineColour), cereal::make_nvp("OutlineWidth", OutlineWidth));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            std::string fontFilePath;

            if(Serialisation::CurrentSceneVersion >= 15)
            {
                archive(cereal::make_nvp("TextString", TextString), cereal::make_nvp("Path", fontFilePath), cereal::make_nvp("Colour", Colour), cereal::make_nvp("LineSpacing", LineSpacing), cereal::make_nvp("Kerning", Kerning),
                        cereal::make_nvp("MaxWidth", MaxWidth), cereal::make_nvp("OutlineColour", OutlineColour), cereal::make_nvp("OutlineWidth", OutlineWidth));
            }
            else
            {
                archive(cereal::make_nvp("TextString", TextString), cereal::make_nvp("Path", fontFilePath), cereal::make_nvp("Colour", Colour), cereal::make_nvp("LineSpacing", LineSpacing), cereal::make_nvp("Kerning", Kerning),
                        cereal::make_nvp("MaxWidth", MaxWidth));
            }

            if(!fontFilePath.empty() && fontFilePath != Graphics::Font::GetDefaultFont()->GetFilePath() && FileSystem::FileExists(fontFilePath))
            {
                Application::Get().GetAssetManager()->AddAsset(fontFilePath, FontHandle);
            }
            else
            {
                FontHandle = Graphics::Font::GetDefaultFont();
            }
        }
    };

    struct PrefabComponent
    {
        PrefabComponent(const std::string& path)
        {
            Path = path;
        }

        std::string Path;
        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(Path);
        }
    };
}
