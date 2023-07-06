#pragma once

#include "Core/Asset.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/animation/offline/raw_skeleton.h>

#include <set>

namespace Lumos
{
    class Skeleton : public Asset
    {
    public:
        Skeleton(const std::string& filename);

        virtual ~Skeleton() = default;

        const std::string& GetFilePath() const { return m_FilePath; }

        static AssetType GetStaticType() { return AssetType::Skeleton; }
        virtual AssetType GetAssetType() const override { return GetStaticType(); }

        const ozz::animation::Skeleton& GetSkeleton() const
        {
            LUMOS_ASSERT(m_Skeleton, "Attempted to access null skeleton!");
            return *m_Skeleton;
        }

    private:
        std::string m_FilePath;
        ozz::unique_ptr<ozz::animation::Skeleton> m_Skeleton;
    };
}