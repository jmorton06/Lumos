#pragma once

#include "Core/Asset.h"
#include "Skeleton.h"

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

namespace Lumos
{

    class Animation : public Asset
    {
    public:
        Animation(const std::string& filename, const std::string& animationName, SharedPtr<Skeleton> skeleton);

        virtual ~Animation() = default;

        const std::string& GetFilePath() const { return m_FilePath; }
        const std::string& GetName() const { return m_AnimationName; }
        const SharedPtr<Skeleton>& GetSkeleton() const { return m_Skeleton; }

        static AssetType GetStaticType() { return AssetType::Animation; }
        virtual AssetType GetAssetType() const override { return GetStaticType(); }

        const ozz::animation::Animation& GetAnimation() const
        {
            LUMOS_ASSERT(m_Animation, "Attempted to access null animation!");
            return *m_Animation;
        }

    private:
        SharedPtr<Skeleton> m_Skeleton;
        std::string m_FilePath;
        std::string m_AnimationName;
        ozz::unique_ptr<ozz::animation::Animation> m_Animation;
    };
}
