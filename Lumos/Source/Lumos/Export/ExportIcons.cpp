#include "Precompiled.h"
#include "ExportIcons.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/MathsUtilities.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <stb/stb_image_resize2.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace Lumos::Export::Icons
{
    namespace
    {
        struct Image
        {
            int w = 0, h = 0;
            TDArray<u8> px; // RGBA
        };

        bool Load(const std::string& path, Image& img, std::string* error)
        {
            int n     = 0;
            u8* data  = stbi_load(path.c_str(), &img.w, &img.h, &n, 4);
            if(!data)
            {
                if(error)
                    *error = "Couldn't load icon " + path;
                return false;
            }
            img.px.Resize((u32)(img.w * img.h * 4));
            memcpy(img.px.Data(), data, img.px.Size());
            stbi_image_free(data);
            if(img.w != img.h)
                LWARN("Icon %s isn't square (%dx%d) - it'll be stretched", path.c_str(), img.w, img.h);
            return true;
        }

        Image Scaled(const Image& src, int size)
        {
            Image out;
            out.w = out.h = size;
            out.px.Resize((u32)(size * size * 4));
            stbir_resize_uint8_srgb(src.px.Data(), src.w, src.h, 0, out.px.Data(), size, size, 0, STBIR_RGBA);
            return out;
        }

        void AppendBytes(void* ctx, void* data, int size)
        {
            auto* buf = (TDArray<u8>*)ctx;
            u32 at    = buf->Size();
            buf->Resize(at + (u32)size);
            memcpy(buf->Data() + at, data, (size_t)size);
        }

        // macOS shows the whole canvas: Apple's grid puts the artwork in the middle 824 of 1024, so a
        // full bleed picture looks too big next to every other Dock icon. Already-padded ones are kept.
        Image MacArtwork(const Image& src)
        {
            int minX = src.w, minY = src.h, maxX = -1, maxY = -1;
            for(int y = 0; y < src.h; y++)
                for(int x = 0; x < src.w; x++)
                    if(src.px[(u32)(y * src.w + x) * 4 + 3] > 16)
                    {
                        minX = x < minX ? x : minX;
                        maxX = x > maxX ? x : maxX;
                        minY = y < minY ? y : minY;
                        maxY = y > maxY ? y : maxY;
                    }
            const bool fullBleed = maxX >= 0 && (maxX - minX + 1) > src.w * 0.92f && (maxY - minY + 1) > src.h * 0.92f;
            Image canvas         = Scaled(src, 1024);
            if(!fullBleed)
                return canvas;

            const int art = 824, offset = (1024 - art) / 2;
            Image inner   = Scaled(src, art);
            // a square picture gets the rounded corners other icons have (one already rounded is unchanged)
            const float radius = art * 0.2237f;
            for(int y = 0; y < art; y++)
                for(int x = 0; x < art; x++)
                {
                    const float dx = Maths::Max(Maths::Max(radius - (x + 0.5f), (x + 0.5f) - (art - radius)), 0.0f);
                    const float dy = Maths::Max(Maths::Max(radius - (y + 0.5f), (y + 0.5f) - (art - radius)), 0.0f);
                    const float d  = Maths::Sqrt(dx * dx + dy * dy) - radius;
                    const float cover = Maths::Clamp(0.5f - d, 0.0f, 1.0f);
                    u8& a             = inner.px[(u32)(y * art + x) * 4 + 3];
                    a                 = (u8)(a * cover);
                }
            memset(canvas.px.Data(), 0, canvas.px.Size());
            for(int y = 0; y < art; y++)
                memcpy(&canvas.px[(u32)((y + offset) * 1024 + offset) * 4], &inner.px[(u32)(y * art) * 4], (size_t)art * 4);
            return canvas;
        }

        TDArray<u8> EncodePng(const Image& img)
        {
            TDArray<u8> out;
            stbi_write_png_to_func(AppendBytes, &out, img.w, img.h, 4, img.px.Data(), img.w * 4);
            return out;
        }

        bool WriteAll(const std::string& path, const TDArray<u8>& bytes, std::string* error)
        {
            std::error_code ec;
            fs::create_directories(fs::path(path).parent_path(), ec);
            std::ofstream f(path, std::ios::binary);
            f.write((const char*)bytes.Data(), bytes.Size());
            if(!f)
            {
                if(error)
                    *error = "Couldn't write " + path;
                return false;
            }
            return true;
        }

        void PutBE32(TDArray<u8>& b, u32 v)
        {
            u8 x[4] = { (u8)(v >> 24), (u8)(v >> 16), (u8)(v >> 8), (u8)v };
            for(u8 c : x)
                b.PushBack(c);
        }
        void PutLE16(TDArray<u8>& b, u16 v)
        {
            b.PushBack((u8)v);
            b.PushBack((u8)(v >> 8));
        }
        void PutLE32(TDArray<u8>& b, u32 v)
        {
            PutLE16(b, (u16)v);
            PutLE16(b, (u16)(v >> 16));
        }
    }

    bool Inspect(const std::string& png, int& width, int& height, bool& transparent)
    {
        int n    = 0;
        u8* data = stbi_load(png.c_str(), &width, &height, &n, 4);
        if(!data)
            return false;
        transparent = false;
        if(n == 2 || n == 4)
        {
            size_t count = (size_t)width * (size_t)height;
            for(size_t i = 0; i < count && !transparent; i++)
                transparent = data[i * 4 + 3] < 250;
        }
        stbi_image_free(data);
        return true;
    }

    bool WriteIcns(const std::string& srcPng, const std::string& outIcns, std::string* error)
    {
        Image loaded;
        if(!Load(srcPng, loaded, error))
            return false;
        const Image src = MacArtwork(loaded);
        // PNG-backed entries; macOS picks what it needs
        struct Entry
        {
            const char* type;
            int size;
        };
        const Entry entries[] = { { "icp4", 16 }, { "icp5", 32 }, { "icp6", 64 }, { "ic07", 128 }, { "ic08", 256 }, { "ic09", 512 },
                                  { "ic10", 1024 }, { "ic11", 32 }, { "ic12", 64 }, { "ic13", 256 }, { "ic14", 512 } };
        TDArray<u8> body;
        for(auto& e : entries)
        {
            TDArray<u8> png = EncodePng(Scaled(src, e.size));
            for(int i = 0; i < 4; i++)
                body.PushBack((u8)e.type[i]);
            PutBE32(body, png.Size() + 8);
            for(u8 c : png)
                body.PushBack(c);
        }
        TDArray<u8> file;
        for(char c : { 'i', 'c', 'n', 's' })
            file.PushBack((u8)c);
        PutBE32(file, body.Size() + 8);
        for(u8 c : body)
            file.PushBack(c);
        return WriteAll(outIcns, file, error);
    }

    bool WriteIco(const std::string& srcPng, const std::string& outIco, std::string* error)
    {
        Image src;
        if(!Load(srcPng, src, error))
            return false;
        const int sizes[] = { 16, 24, 32, 48, 64, 128, 256 };
        const int count   = (int)(sizeof(sizes) / sizeof(sizes[0]));
        TDArray<TDArray<u8>> pngs;
        for(int s : sizes)
            pngs.PushBack(EncodePng(Scaled(src, s)));

        TDArray<u8> file;
        PutLE16(file, 0);
        PutLE16(file, 1); // icon
        PutLE16(file, (u16)count);
        u32 offset = 6 + 16 * count;
        for(int i = 0; i < count; i++)
        {
            file.PushBack((u8)(sizes[i] >= 256 ? 0 : sizes[i]));
            file.PushBack((u8)(sizes[i] >= 256 ? 0 : sizes[i]));
            file.PushBack(0); // palette
            file.PushBack(0);
            PutLE16(file, 1);  // planes
            PutLE16(file, 32); // bpp
            PutLE32(file, pngs[i].Size());
            PutLE32(file, offset);
            offset += pngs[i].Size();
        }
        for(auto& p : pngs)
            for(u8 c : p)
                file.PushBack(c);
        return WriteAll(outIco, file, error);
    }

    bool WriteIOSAssetCatalog(const std::string& srcPng, const std::string& outXcassets, std::string* error)
    {
        Image src;
        if(!Load(srcPng, src, error))
            return false;
        Image img = Scaled(src, 1024);
        // App Store rejects alpha: flatten onto black
        for(u32 i = 0; i < img.px.Size(); i += 4)
        {
            u32 a        = img.px[i + 3];
            img.px[i]     = (u8)(img.px[i] * a / 255);
            img.px[i + 1] = (u8)(img.px[i + 1] * a / 255);
            img.px[i + 2] = (u8)(img.px[i + 2] * a / 255);
            img.px[i + 3] = 255;
        }
        std::string set = outXcassets + "/AppIcon.appiconset/";
        if(!WriteAll(set + "AppIcon-1024.png", EncodePng(img), error))
            return false;
        const char* contents = R"({
  "images" : [ { "filename" : "AppIcon-1024.png", "idiom" : "universal", "platform" : "ios", "size" : "1024x1024" } ],
  "info" : { "author" : "lumos", "version" : 1 }
}
)";
        const char* root     = R"({ "info" : { "author" : "lumos", "version" : 1 } }
)";
        TDArray<u8> a, b;
        for(const char* c = contents; *c; c++)
            a.PushBack((u8)*c);
        for(const char* c = root; *c; c++)
            b.PushBack((u8)*c);
        return WriteAll(set + "Contents.json", a, error) && WriteAll(outXcassets + "/Contents.json", b, error);
    }

    bool WriteMacAssetCatalog(const std::string& srcPng, const std::string& outXcassets, std::string* error)
    {
        Image src;
        if(!Load(srcPng, src, error))
            return false;
        std::string set    = outXcassets + "/AppIcon.appiconset/";
        std::string images = "";
        for(int pt : { 16, 32, 128, 256, 512 })
            for(int scale : { 1, 2 })
            {
                int px           = pt * scale;
                std::string name = "icon_" + std::to_string(pt) + "x" + std::to_string(pt) + (scale == 2 ? "@2x" : "") + ".png";
                if(!WriteAll(set + name, EncodePng(Scaled(src, px)), error))
                    return false;
                if(!images.empty())
                    images += ",\n";
                images += "    { \"filename\" : \"" + name + "\", \"idiom\" : \"mac\", \"scale\" : \"" + std::to_string(scale) + "x\", \"size\" : \"" +
                          std::to_string(pt) + "x" + std::to_string(pt) + "\" }";
            }
        std::string contents = "{\n  \"images\" : [\n" + images + "\n  ],\n  \"info\" : { \"author\" : \"lumos\", \"version\" : 1 }\n}\n";
        std::string root     = "{ \"info\" : { \"author\" : \"lumos\", \"version\" : 1 } }\n";
        TDArray<u8> a, b;
        for(char c : contents)
            a.PushBack((u8)c);
        for(char c : root)
            b.PushBack((u8)c);
        return WriteAll(set + "Contents.json", a, error) && WriteAll(outXcassets + "/Contents.json", b, error);
    }

    bool WritePngSet(const std::string& srcPng, const std::string& outDir, std::string* error)
    {
        Image src;
        if(!Load(srcPng, src, error))
            return false;
        for(int s : { 16, 32, 48, 64, 128, 256, 512 })
            if(!WriteAll(outDir + "/icon_" + std::to_string(s) + ".png", EncodePng(Scaled(src, s)), error))
                return false;
        return true;
    }
}
