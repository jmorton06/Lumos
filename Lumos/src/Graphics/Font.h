#pragma once

#include "JM.h"
#include "API/Renderer.h"

namespace jm
{
    class Texture2D;
    class VertexArray;
    class Material;

    struct Character
    {
        std::shared_ptr<Texture2D> Texture;
        maths::Vector2 Size;    // Size of glyph
        maths::Vector2 Bearing;  // Offset from baseline to left/top of glyph
        uint Advance;    // Horizontal offset to advance to next glyph
    };

    class JM_EXPORT Font
    {
    public:
        Font(const char* fontName);
        ~Font();

        const std::map<char, Character>& GetCharacters() const { return m_Characters; };

        Material* GetMaterial() const { return m_Material.get(); }
        VertexArray* GetVertexArray() const { return m_VAO; }

    private:

        std::map<char, Character> m_Characters;
        VertexArray* m_VAO;
        std::shared_ptr<Material> m_Material;
    };
}

