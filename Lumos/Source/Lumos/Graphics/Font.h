#pragma once
#include "Core/Asset/Asset.h"

namespace Lumos
{
    struct MSDFData;
    namespace Graphics
    {
        class Texture2D;

        class Font : public Asset
        {
            class FontHolder;

        public:
            Font(const std::string& filepath);
            Font(uint8_t* data, uint32_t dataSize, const std::string& name);

            virtual ~Font();

            SharedPtr<Graphics::Texture2D> GetFontAtlas() const;
            const MSDFData* GetMSDFData() const { return m_MSDFData; }
            const std::string& GetFilePath() const { return m_FilePath; }
            Vec2 CalculateTextSize(const std::string& text, float fontSize);
            Vec2 CalculateTextSize(const String8& text, float fontSize);
            // every character (bar spaces / line breaks) has a glyph in the atlas - else it draws as '?'
            bool HasGlyphs(const std::string& text) const;

            void Init();

            static void InitDefaultFont();
            static void ShutdownDefaultFont();
            static SharedPtr<Font> GetDefaultFont();
            // Fixed-width font for code (<engine assets>/Fonts/Cousine-Regular.ttf), loaded on first use.
            // The default font when that file is not there.
            static SharedPtr<Font> GetMonoFont();

            SET_ASSET_TYPE(AssetType::Font);

        private:
            std::string m_FilePath;
            SharedPtr<Graphics::Texture2D> m_TextureAtlas;
            MSDFData* m_MSDFData = nullptr;
            uint8_t* m_FontData;
            uint32_t m_FontDataSize;

        private:
            static SharedPtr<Font> s_DefaultFont;
            static SharedPtr<Font> s_MonoFont;
        };
    }

}
