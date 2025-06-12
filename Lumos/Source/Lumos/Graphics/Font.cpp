#include "Precompiled.h"
#include "Font.h"
#include "MSDFData.h"
#include "Core/OS/FileSystem.h"
#include "Core/Buffer.h"
#include "RHI/Texture.h"
#include "Core/OS/FileSystem.h"
#include "Core/Application.h"
#include "Core/OS/OS.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Vector3.h"

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

#include <imgui/Plugins/ImGuiAl/fonts/RobotoRegular.inl>
#include <stb/deprecated/stb.h>
#include <fstream>

#define FONT_DEBUG_LOG 0
#if FONT_DEBUG_LOG
#define FONT_LOG(...) LINFO("Font", __VA_ARGS__)
#else
#define FONT_LOG(...) ((void)0)
#endif

using namespace msdf_atlas;

#define USE_FREETYPE
#ifdef USE_FREETYPE

#else

#endif

#ifdef LUMOS_PLATFORM_IOS
#include "Platform/iOS/iOSOS.h"
#endif

namespace Lumos
{
    namespace Graphics
    {

        struct FontInput
        {
            const char* fontFilename;
            GlyphIdentifierType glyphIdentifierType;
            const char* charsetFilename;
            double fontScale;
            const char* fontName;
            uint32_t* data;
            uint32_t dataSize;
        };

        struct Configuration
        {
            ImageType imageType;
            msdf_atlas::ImageFormat imageFormat;
            YDirection yDirection;
            int width, height;
            double emSize;
            double pxRange;
            double angleThreshold;
            double miterLimit;
            void (*edgeColoring)(msdfgen::Shape&, double, unsigned long long);
            bool expensiveColoring;
            unsigned long long coloringSeed;
            GeneratorAttributes generatorAttributes;
        };

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define DEFAULT_MITER_LIMIT 1.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREADS 8

        static std::filesystem::path GetCacheDirectory()
        {
            const char* home = std::getenv("HOME");
            if(!home)
            {
                throw std::runtime_error("Can't obtain HOME");
            }

            return std::filesystem::path(home) / "Documents/Lumos/Resources/Cache/FontAtlases";
        }

        static void CreateCacheDirectoryIfNeeded()
        {
            try
            {
                std::filesystem::path cacheDirectory = GetCacheDirectory();
                if(!std::filesystem::exists(cacheDirectory))
                {
                    std::filesystem::create_directories(cacheDirectory);
                    LINFO("Created Directory : %s", cacheDirectory.c_str());
                }
            }
            catch(const std::filesystem::filesystem_error& e)
            {
                LERROR("Failed Creating Directory : ", e.what());
            }
        }

        struct AtlasHeader
        {
            uint32_t Type  = 0;
            uint32_t Width = 0, Height = 0;
        };

        static bool TryReadFontAtlasFromCache(const std::string& fontName, float fontSize, AtlasHeader& header, void*& pixels, Buffer& storageBuffer)
        {
            LUMOS_PROFILE_FUNCTION();

            Lumos::ArenaTemp scratch = Lumos::ScratchBegin(nullptr, 0);
            Lumos::String8 filename  = Lumos::PushStr8F(scratch.arena, "%s-%.2f.lfa", fontName.c_str(), fontSize);

            std::filesystem::path filepath = GetCacheDirectory() / (const char*)filename.str;
            Lumos::ScratchEnd(scratch);

            if(std::filesystem::exists(filepath))
            {
                storageBuffer.Allocate(uint32_t(FileSystem::GetFileSize(filepath.string())));
                auto data = FileSystem::ReadFile(filepath.string());
                storageBuffer.Write(data, storageBuffer.Size);
                delete[] data;

                header = *storageBuffer.As<AtlasHeader>();
                pixels = (uint8_t*)storageBuffer.Data + sizeof(AtlasHeader);
                return true;
            }
            return false;
        }

        static void CacheFontAtlas(const std::string& fontName, float fontSize, AtlasHeader header, const void* pixels)
        {
            LUMOS_PROFILE_FUNCTION();
            CreateCacheDirectoryIfNeeded();

            Lumos::ArenaTemp scratch = Lumos::ScratchBegin(nullptr, 0);
            Lumos::String8 filename  = Lumos::PushStr8F(scratch.arena, "%s-%.2f.lfa", fontName.c_str(), fontSize);

            std::filesystem::path filepath = GetCacheDirectory() / (const char*)filename.str;
            Lumos::ScratchEnd(scratch);

            std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
            if(!stream)
            {
                stream.close();
                LERROR("Failed to cache font atlas to %s", filepath.string().c_str());
                return;
            }

            stream.write((char*)&header, sizeof(AtlasHeader));
            stream.write((char*)pixels, header.Width * header.Height * sizeof(float) * 4);
        }

        static SharedPtr<Texture2D> CreateCachedAtlas(AtlasHeader header, void* pixels)
        {
            LUMOS_PROFILE_FUNCTION();
            TextureDesc param;
            param.minFilter            = TextureFilter::LINEAR;
            param.magFilter            = TextureFilter::LINEAR;
            param.format               = RHIFormat::R32G32B32A32_Float;
            param.srgb                 = false;
            param.wrap                 = TextureWrap::CLAMP;
            param.generateMipMaps      = false;
            param.flags                = TextureFlags::Texture_Sampled;
            param.anisotropicFiltering = false;

            return SharedPtr<Texture2D>(Texture2D::CreateFromSource(header.Width, header.Height, pixels, param));
        }

        template <typename T, typename S, int N, GeneratorFunction<S, N> GEN_FN>
        static SharedPtr<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<GlyphGeometry>& glyphs, const FontGeometry& fontGeometry, const Configuration& config)
        {
            LUMOS_PROFILE_FUNCTION();
            ImmediateAtlasGenerator<S, N, GEN_FN, BitmapAtlasStorage<T, N>> generator(config.width, config.height);
            generator.setAttributes(config.generatorAttributes);
            generator.setThreadCount(THREADS);
            generator.generate(glyphs.data(), int(glyphs.size()));

            msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

            AtlasHeader header;
            header.Width  = bitmap.width;
            header.Height = bitmap.height;
            CacheFontAtlas(fontName, fontSize, header, bitmap.pixels);

            TextureDesc param;
            param.minFilter       = TextureFilter::LINEAR;
            param.magFilter       = TextureFilter::LINEAR;
            param.format          = RHIFormat::R32G32B32A32_Float;
            param.srgb            = false;
            param.wrap            = TextureWrap::CLAMP;
            param.generateMipMaps = false;
            param.flags           = TextureFlags::Texture_Sampled;
            return CreateCachedAtlas(header, (void*)bitmap.pixels);
        }

        Font::Font(uint8_t* data, uint32_t dataSize, const std::string& name)
            : m_FontData(data)
            , m_FontDataSize(dataSize)
            , m_MSDFData(new MSDFData())
            , m_FilePath(name)
        {
            Init();
        }

        Font::Font(const std::string& filepath)
            : m_FilePath(filepath)
            , m_MSDFData(new MSDFData())
            , m_FontData(nullptr)
            , m_FontDataSize(0)
        {
            Init();
        }

        Font::~Font()
        {
            delete m_MSDFData;
        }

        void Font::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            FontInput fontInput           = {};
            fontInput.glyphIdentifierType = GlyphIdentifierType::UNICODE_CODEPOINT;
            fontInput.fontScale           = -1;

            Configuration config                             = {};
            config.imageFormat                               = msdf_atlas::ImageFormat::BINARY_FLOAT;
            config.yDirection                                = YDirection::BOTTOM_UP;
            config.edgeColoring                              = msdfgen::edgeColoringInkTrap;
            config.generatorAttributes.config.overlapSupport = true;
            config.generatorAttributes.scanlinePass          = true;
            config.angleThreshold                            = DEFAULT_ANGLE_THRESHOLD;
            config.miterLimit                                = DEFAULT_MITER_LIMIT;
            config.imageType                                 = ImageType::MTSDF;
            config.emSize                                    = 40;

            const char* imageFormatName                                = nullptr;
            int fixedWidth                                             = -1;
            int fixedHeight                                            = -1;
            double minEmSize                                           = 0;
            double rangeValue                                          = 8.0;
            TightAtlasPacker::DimensionsConstraint atlasSizeConstraint = TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE;

            // Load fonts
            bool anyCodepointsAvailable = false;
            class FontHolder
            {
                msdfgen::FreetypeHandle* ft;
                msdfgen::FontHandle* font;
                const char* fontFilename;

            public:
                FontHolder()
                    : ft(msdfgen::initializeFreetype())
                    , font(nullptr)
                    , fontFilename(nullptr)
                {
                }
                ~FontHolder()
                {
                    if(ft)
                    {
                        if(font)
                            msdfgen::destroyFont(font);
                        msdfgen::deinitializeFreetype(ft);
                    }
                }
                bool load(const char* fontFilename)
                {
                    if(ft && fontFilename)
                    {
                        if(this->fontFilename && !strcmp(this->fontFilename, fontFilename))
                            return true;
                        if(font)
                            msdfgen::destroyFont(font);
                        if((font = msdfgen::loadFont(ft, fontFilename)))
                        {
                            this->fontFilename = fontFilename;
                            return true;
                        }
                        this->fontFilename = nullptr;
                    }
                    return false;
                }
                bool load(uint8_t* data, uint32_t dataSize)
                {
                    if(ft && dataSize > 0)
                    {
                        if(font)
                            msdfgen::destroyFont(font);
                        if((font = msdfgen::loadFontData(ft, (const unsigned char*)data, int(dataSize))))
                        {
                            return true;
                        }
                        this->fontFilename = nullptr;
                    }
                    return false;
                }
                operator msdfgen::FontHandle*() const
                {
                    return font;
                }
            } font;

            if(m_FontDataSize == 0)
            {
                std::string outPath;
                if(!FileSystem::Get().ResolvePhysicalPath(m_FilePath, outPath))
                    return;

                FONT_LOG("Font: Loading Font %s", m_FilePath);
                fontInput.fontFilename = outPath.c_str();
                m_FilePath             = outPath;

                if(!font.load(fontInput.fontFilename))
                {
                    FONT_LOG("Font: Failed to load font! - %s", fontInput.fontFilename);
                    return;
                }
            }
            else
            {
                if(!font.load(m_FontData, m_FontDataSize))
                {
                    FONT_LOG("Font: Failed to load font from data!");
                    return;
                }
            }

            if(fontInput.fontScale <= 0)
                fontInput.fontScale = 1;

            // Load character set
            fontInput.glyphIdentifierType = GlyphIdentifierType::UNICODE_CODEPOINT;
            Charset charset;

            // From ImGui
            static const uint32_t charsetRanges[] = {
                0x0020,
                0x00FF, // Basic Latin + Latin Supplement
                0x0400,
                0x052F, // Cyrillic + Cyrillic Supplement
                0x2DE0,
                0x2DFF, // Cyrillic Extended-A
                0xA640,
                0xA69F, // Cyrillic Extended-B
                0,
            };

            for(int range = 0; range < 8; range += 2)
            {
                for(uint32_t c = charsetRanges[range]; c <= charsetRanges[range + 1]; c++)
                    charset.add(c);
            }

            // Load glyphs
            m_MSDFData->FontGeometry = FontGeometry(&m_MSDFData->Glyphs);
            int glyphsLoaded         = -1;
            switch(fontInput.glyphIdentifierType)
            {
            case GlyphIdentifierType::GLYPH_INDEX:
                glyphsLoaded = m_MSDFData->FontGeometry.loadGlyphset(font, fontInput.fontScale, charset);
                break;
            case GlyphIdentifierType::UNICODE_CODEPOINT:
                glyphsLoaded = m_MSDFData->FontGeometry.loadCharset(font, fontInput.fontScale, charset);
                anyCodepointsAvailable |= glyphsLoaded > 0;
                break;
            }

            ASSERT(glyphsLoaded >= 0);
            FONT_LOG("Font: Loaded geometry of %i out of %i glyphs", glyphsLoaded, (int)charset.size());
            // List missing glyphs
            if(glyphsLoaded < (int)charset.size())
            {
                FONT_LOG("Font: Missing %i %i", (int)charset.size() - glyphsLoaded, fontInput.glyphIdentifierType == GlyphIdentifierType::UNICODE_CODEPOINT ? "codepoints" : "glyphs");
            }

            if(fontInput.fontName)
                m_MSDFData->FontGeometry.setName(fontInput.fontName);

            // Determine final atlas dimensions, scale and range, pack glyphs
            double pxRange       = rangeValue;
            bool fixedDimensions = fixedWidth >= 0 && fixedHeight >= 0;
            bool fixedScale      = config.emSize > 0;
            TightAtlasPacker atlasPacker;
            if(fixedDimensions)
                atlasPacker.setDimensions(fixedWidth, fixedHeight);
            else
                atlasPacker.setDimensionsConstraint(atlasSizeConstraint);
            atlasPacker.setPadding(config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF ? 0 : -1);

            if(fixedScale)
                atlasPacker.setScale(config.emSize);
            else
                atlasPacker.setMinimumScale(minEmSize);
            atlasPacker.setPixelRange(pxRange);
            atlasPacker.setMiterLimit(config.miterLimit);
            if(int remaining = atlasPacker.pack(m_MSDFData->Glyphs.data(), int(m_MSDFData->Glyphs.size())))
            {
                LERROR("Font: Could not fit %i out of %i glyphs into the atlas.", remaining, (int)m_MSDFData->Glyphs.size());
            }
            atlasPacker.getDimensions(config.width, config.height);
            ASSERT(config.width > 0 && config.height > 0);
            config.emSize  = atlasPacker.getScale();
            config.pxRange = atlasPacker.getPixelRange();
            if(!fixedScale)
                FONT_LOG("Font: Glyph size: %i pixels/EM", config.emSize);
            if(!fixedDimensions)
                FONT_LOG("Font: Atlas dimensions: %i x %i", config.width, config.height);

            // Edge coloring
            if(config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF)
            {
                if(config.expensiveColoring)
                {
                    Workload([&glyphs = m_MSDFData->Glyphs, &config](int i, int threadNo) -> bool
                             {
                    unsigned long long glyphSeed = (LCG_MULTIPLIER * (config.coloringSeed ^ i) + LCG_INCREMENT) * !!config.coloringSeed;
                    glyphs[i].edgeColoring(config.edgeColoring, config.angleThreshold, glyphSeed);
                    return true; },
                             int(m_MSDFData->Glyphs.size()))
                        .finish(THREADS);
                }
                else
                {
                    unsigned long long glyphSeed = config.coloringSeed;
                    for(GlyphGeometry& glyph : m_MSDFData->Glyphs)
                    {
                        glyphSeed *= LCG_MULTIPLIER;
                        glyph.edgeColoring(config.edgeColoring, config.angleThreshold, glyphSeed);
                    }
                }
            }

            std::string fontName = m_FilePath;

            // Check cache here
            Buffer storageBuffer;
            AtlasHeader header;
            void* pixels;
            if(TryReadFontAtlasFromCache(fontName, (float)config.emSize, header, pixels, storageBuffer))
            {
                m_TextureAtlas = CreateCachedAtlas(header, pixels);
                storageBuffer.Release();
            }
            else
            {
                bool floatingPointFormat = true;
                SharedPtr<Texture2D> texture;
                switch(config.imageType)
                {
                case ImageType::MSDF:
                    if(floatingPointFormat)
                        texture = CreateAndCacheAtlas<float, float, 3, msdfGenerator>(fontName, (float)config.emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
                    else
                        texture = CreateAndCacheAtlas<byte, float, 3, msdfGenerator>(fontName, (float)config.emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
                    break;
                case ImageType::MTSDF:
                    if(floatingPointFormat)
                        texture = CreateAndCacheAtlas<float, float, 4, mtsdfGenerator>(fontName, (float)config.emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
                    else
                        texture = CreateAndCacheAtlas<byte, float, 4, mtsdfGenerator>(fontName, (float)config.emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
                    break;
                }

                m_TextureAtlas = texture;
            }
        }

        SharedPtr<Font> Font::s_DefaultFont;

        SharedPtr<Graphics::Texture2D> Font::GetFontAtlas() const
        {
            return m_TextureAtlas;
        }

        void Font::InitDefaultFont()
        {
            const unsigned int buf_decompressed_size = stb_decompress_length((unsigned char*)RobotoRegular_compressed_data);
            unsigned char* buf_decompressed_data     = (unsigned char*)malloc(buf_decompressed_size);
            stb_decompress(buf_decompressed_data, (unsigned char*)RobotoRegular_compressed_data, (unsigned int)RobotoRegular_compressed_size);

            s_DefaultFont = CreateSharedPtr<Font>((uint8_t*)buf_decompressed_data, uint32_t(buf_decompressed_size), "RobotoRegular");
            free(buf_decompressed_data);
        }

        void Font::ShutdownDefaultFont()
        {
            s_DefaultFont.reset();
        }

        SharedPtr<Font> Font::GetDefaultFont()
        {
            return s_DefaultFont;
        }

        Vec2 Font::CalculateTextSize(const std::string& text, float fontSize)
        {
            return CalculateTextSize(Str8StdS(text), fontSize);
        }
        Vec2 Font::CalculateTextSize(const String8& text, float fontSize)
        {
            float maxWidth     = 100.0f; // widget->size.x;// textComp.MaxWidth;
            auto colour        = Vec4(1.0f);
            float lineSpacing  = 0.0f;
            float kerning      = 0.0f;
            auto outlineColour = Vec4(1.0f);
            auto outlineWidth  = 0.0f;

            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            Mat4 transform         = Mat4::Scale(Vec3(fontSize, fontSize, fontSize));

            SharedPtr<Texture2D> fontAtlas = GetFontAtlas();
            if(!fontAtlas)
                ASSERT(false);

            auto& fontGeometry  = GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            Vec2 size = Vec2(0.0f);

            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.size; i++)
                {
                    char32_t character = text.str[i];

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = fontGeometry.getGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    {
                        /*  LUMOS_PROFILE_SCOPE("Set text buffer data");
                          TextVertexBufferPtr->vertex = transform * Vec4(pl, pb, 0.0f, 1.0f);
                          TextVertexBufferPtr->colour = colour;
                          TextVertexBufferPtr->uv = { l, b };
                          TextVertexBufferPtr->tid = Vec2(textureIndex, outlineWidth);
                          TextVertexBufferPtr->outlineColour = outlineColour;
                          TextVertexBufferPtr++;

                          TextVertexBufferPtr->vertex = transform * Vec4(pr, pb, 0.0f, 1.0f);
                          TextVertexBufferPtr->colour = colour;
                          TextVertexBufferPtr->uv = { r, b };
                          TextVertexBufferPtr->tid = Vec2(textureIndex, outlineWidth);
                          TextVertexBufferPtr->outlineColour = outlineColour;
                          TextVertexBufferPtr++;

                          TextVertexBufferPtr->vertex = transform * Vec4(pr, pt, 0.0f, 1.0f);
                          TextVertexBufferPtr->colour = colour;
                          TextVertexBufferPtr->uv = { r, t };
                          TextVertexBufferPtr->tid = Vec2(textureIndex, outlineWidth);
                          TextVertexBufferPtr->outlineColour = outlineColour;
                          TextVertexBufferPtr++;

                          TextVertexBufferPtr->vertex = transform * Vec4(pl, pt, 0.0f, 1.0f);
                          TextVertexBufferPtr->colour = colour;
                          TextVertexBufferPtr->uv = { l, t };
                          TextVertexBufferPtr->tid = Vec2(textureIndex, outlineWidth);
                          TextVertexBufferPtr->outlineColour = outlineColour;
                          TextVertexBufferPtr++;*/
                        Vec2 currentSize = (transform * Vec4((float)pr, (float)pt, 0.0f, 1.0f)).ToVector2();
                        size.x           = Maths::Max(size.x, currentSize.x);
                        size.y           = Maths::Max(size.y, currentSize.y);
                    }

                    double advance = glyph->getAdvance();
                    fontGeometry.getAdvance(advance, character, text.str[i + 1]);
                    x += fsScale * advance + kerningOffset;
                }
            }

            return size;
        }
    }
}
