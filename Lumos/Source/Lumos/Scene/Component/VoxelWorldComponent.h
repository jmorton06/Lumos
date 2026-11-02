#pragma once
#include "Graphics/Voxel/VoxelWorld.h"
#include "Core/Core.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    struct VoxelWorldComponent
    {
        VoxelWorldComponent()
            : World(CreateUniquePtr<Graphics::VoxelWorld>())
        {
        }

        UniquePtr<Graphics::VoxelWorld> World;
        int ViewRadius = 6;

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("ViewRadius", ViewRadius));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("ViewRadius", ViewRadius));
            if(World)
                World->SetViewRadius(ViewRadius);
        }
    };
}
