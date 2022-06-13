#pragma once
#include "Graphics/RHI/Definitions.h"

namespace Lumos
{
    namespace Graphics
    {

        class Font
        {
        public:
            Font(const std::string& path);
            ~Font();

            SharedPtr<Texture2D> GetFontAtlas() const { return m_TextureAtlas; }

            const std::string& GetName() const;
            const std::string& GetPath() const;

            const FontData& GetFontData() const;
            const FontData& GetFontData(const std::string& name) const;

        private:
            std::string m_Name;
            std::string m_Path;
            SharedPtr<Texture2D> m_TextureAtlas;
        };
    }
}