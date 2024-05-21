#pragma once

#include "Graphics/Font.h"
#include "Core/OS/FileSystem.h"

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
    };

    struct PrefabComponent
    {
        PrefabComponent(const std::string& path)
        {
            Path = path;
        }

        std::string Path;
    };
}
