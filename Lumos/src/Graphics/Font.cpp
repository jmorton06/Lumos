#include "LM.h"
#include "Font.h"
#include "System/VFS.h"
#include "Utilities/AssetsManager.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/VertexArray.h"
#include "Graphics/Material.h"


#include <ft2build.h>
#include FT_FREETYPE_H

namespace Lumos
{
    Font::Font(const char* fontName)
            : m_VAO(nullptr)
    {

        m_Material = std::make_shared<Material>();//std::shared_ptr<Shader>(Shader::CreateFromFile("text", "/Shaders/Scene/text")));

        String filePath = String(fontName);
#ifdef LUMOS_PLATFORM_MOBILE
        if (filePath.find_last_of("/") != String::npos)
            filePath = filePath.substr(filePath.find_last_of("/") + 1);
#endif

        std::string physicalPath;
        Lumos::VFS::Get()->ResolvePhysicalPath(filePath, physicalPath);
        fontName = physicalPath.c_str();

        FT_Library ft;
        FT_Face face;

        // All functions return a value different than 0 whenever an error occurred
        if (FT_Init_FreeType(&ft))
            LUMOS_CORE_ERROR("ERROR::FREETYPE: Could not init FreeType Library");

        // Load font as face

        //if (FT_New_Face(ft, TEXTUREDIR"OpenSans-Semibold.ttf", 0, &face))
        if (FT_New_Face(ft, fontName, 0, &face))
            LUMOS_CORE_ERROR("ERROR::FREETYPE: Failed to load font");

        // Set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // Disable byte-alignment restriction
        Renderer::GetRenderer()->SetPixelPackType(PixelPackType::UNPACK);

        // Load first 128 characters of ASCII set
        for (byte c = 0; c < 128; c++)
        {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                LUMOS_CORE_ERROR("ERROR::FREETYTPE: Failed to load Glyph");
                continue;
            }

            m_Characters.insert(std::pair<char, Character>(c, {
                    std::shared_ptr<Texture2D>(Texture2D::Create(face->glyph->bitmap.width, face->glyph->bitmap.rows, face->glyph->bitmap.buffer)),
                    maths::Vector2(static_cast<float>(face->glyph->bitmap.width), static_cast<float>(face->glyph->bitmap.rows)),
                    maths::Vector2(static_cast<float>(face->glyph->bitmap_left), static_cast<float>(face->glyph->bitmap_top)),
                    static_cast<uint>(face->glyph->advance.x)
            }));
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        m_VAO = VertexArray::Create();

        VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::DYNAMIC);
        buffer->SetData(sizeof(float) * 6 * 4, nullptr);

        graphics::BufferLayout layout;
        layout.Push<float>("vertex", 4, false);
        buffer->SetLayout(layout);

        m_VAO->PushBuffer(buffer);
    }

    Font::~Font()
    {
        if(m_VAO)
            delete m_VAO;
    }
}
