#pragma once
#include "Graphics/RHI/Texture.h"
#include "Maths/Vector4.h"
#include "Core/OS/FileSystem.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    // Projected box decal. The box is the entity's transform over the unit cube (-1..1, like the
    // cube primitive: scale = half size) and it projects down its local -Y onto whatever is inside.
    // Drawn in the forward pass before lighting, so it is lit and shadowed with the surface.
    struct DecalComponent
    {
        SharedPtr<Graphics::Texture2D> Texture;
        std::string TexturePath;
        Vec4 Colour     = Vec4(1.0f); // a = opacity
        float AngleFade = 0.5f;       // cos of the angle the fade starts at; surfaces turned further away get less
        float Roughness = -1.0f;      // < 0 keeps the surface's own
        float Emissive  = 0.0f;
        int Order       = 0;          // higher draws over lower

        void SetTexture(const std::string& path)
        {
            TexturePath = path;
            Texture     = path.empty() ? nullptr : SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("decal", path));
        }
    };

    template <typename Archive>
    void save(Archive& archive, const DecalComponent& decal)
    {
        ArenaTemp temp = ScratchBegin(nullptr, 0);
        String8 vfsPath;
        std::string path = decal.TexturePath;
        if(!path.empty() && FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(path), vfsPath))
            path = ToStdString(vfsPath);
        archive(cereal::make_nvp("Texture", path), cereal::make_nvp("Colour", decal.Colour), cereal::make_nvp("AngleFade", decal.AngleFade),
                cereal::make_nvp("Roughness", decal.Roughness), cereal::make_nvp("Emissive", decal.Emissive), cereal::make_nvp("Order", decal.Order));
        ScratchEnd(temp);
    }

    template <typename Archive>
    void load(Archive& archive, DecalComponent& decal)
    {
        std::string path;
        archive(cereal::make_nvp("Texture", path), cereal::make_nvp("Colour", decal.Colour), cereal::make_nvp("AngleFade", decal.AngleFade),
                cereal::make_nvp("Roughness", decal.Roughness), cereal::make_nvp("Emissive", decal.Emissive), cereal::make_nvp("Order", decal.Order));
        decal.SetTexture(path);
    }
}
