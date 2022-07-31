#include "Precompiled.h"
#include "Font.h"
#include "MSDFData.h"
#include "Core/OS/FileSystem.h"
#include "Core/Buffer.h"
#include "RHI/Texture.h"
#include "Core/VFS.h"
#include "Core/Application.h"

#include <imgui/Plugins/ImGuiAl/fonts/RobotoRegular.inl>
#include <stb/stb.h>

using namespace msdf_atlas;

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
            return "Resources/Cache/FontAtlases";
        }

        static void CreateCacheDirectoryIfNeeded()
        {
            std::filesystem::path cacheDirectory = GetCacheDirectory();
            if(!std::filesystem::exists(cacheDirectory))
                std::filesystem::create_directories(cacheDirectory);
        }

        struct AtlasHeader
        {
            uint32_t Type = 0;
            uint32_t Width, Height;
        };

        static bool TryReadFontAtlasFromCache(const std::string& fontName, float fontSize, AtlasHeader& header, void*& pixels, Buffer& storageBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            std::string filename           = fmt::format("{0}-{1}.lfa", fontName, fontSize);
            std::filesystem::path filepath = GetCacheDirectory() / filename;

            if(std::filesystem::exists(filepath))
            {
                storageBuffer.Allocate(uint32_t(FileSystem::GetFileSize(filepath)));
                storageBuffer.Write(FileSystem::ReadFile(filepath), storageBuffer.Size);
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

            std::string filename           = fmt::format("{0}-{1}.lfa", fontName, fontSize);
            std::filesystem::path filepath = GetCacheDirectory() / filename;

            std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
            if(!stream)
            {
                stream.close();
                LUMOS_LOG_ERROR("Failed to cache font atlas to {0}", filepath.string());
                return;
            }

            stream.write((char*)&header, sizeof(AtlasHeader));
            stream.write((char*)pixels, header.Width * header.Height * sizeof(float) * 4);
        }

        static SharedPtr<Texture2D> CreateCachedAtlas(AtlasHeader header, void* pixels)
        {
            LUMOS_PROFILE_FUNCTION();
            TextureDesc param;
            param.minFilter       = TextureFilter::LINEAR;
            param.magFilter       = TextureFilter::LINEAR;
            param.format          = RHIFormat::R32G32B32A32_Float;
            param.srgb            = false;
            param.wrap            = TextureWrap::CLAMP;
            param.generateMipMaps = false;
            param.flags           = TextureFlags::Texture_Sampled;
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
            config.imageType                                 = ImageType::MSDF;
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
            double rangeValue                                          = 2.0;
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
                if(!VFS::Get().ResolvePhysicalPath(m_FilePath, outPath))
                    return;

                LUMOS_LOG_INFO("Loading Font {0}", m_FilePath);
                fontInput.fontFilename = outPath.c_str();
                m_FilePath             = outPath;

                if(!font.load(fontInput.fontFilename))
                {
                    LUMOS_LOG_ERROR("Failed to load font! - {0}", fontInput.fontFilename);
                    return;
                }
            }
            else
            {
                if(!font.load(m_FontData, m_FontDataSize))
                {
                    LUMOS_LOG_ERROR("Failed to load font from data!");
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
                for(int c = charsetRanges[range]; c <= charsetRanges[range + 1]; c++)
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

            LUMOS_ASSERT(glyphsLoaded >= 0, "");
            LUMOS_LOG_TRACE("Loaded geometry of {0} out of {1} glyphs", glyphsLoaded, (int)charset.size());
            // List missing glyphs
            if(glyphsLoaded < (int)charset.size())
            {
                LUMOS_LOG_WARN("Missing {0} {1}", (int)charset.size() - glyphsLoaded, fontInput.glyphIdentifierType == GlyphIdentifierType::UNICODE_CODEPOINT ? "codepoints" : "glyphs");
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
                LUMOS_LOG_ERROR("Error: Could not fit {0} out of {1} glyphs into the atlas.", remaining, (int)m_MSDFData->Glyphs.size());
            }
            atlasPacker.getDimensions(config.width, config.height);
            LUMOS_CORE_ASSERT(config.width > 0 && config.height > 0, "");
            config.emSize  = atlasPacker.getScale();
            config.pxRange = atlasPacker.getPixelRange();
            if(!fixedScale)
                LUMOS_LOG_TRACE("Glyph size: {0} pixels/EM", config.emSize);
            if(!fixedDimensions)
                LUMOS_LOG_TRACE("Atlas dimensions: {0} x {1}", config.width, config.height);

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
    }
}
