#include "Precompiled.h"
#include "Animation.h"

#include <ozz/animation/offline/skeleton_builder.h>

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

namespace Lumos
{
    Animation::Animation(const std::string& filename, const std::string& animationName, SharedPtr<Skeleton> skeleton)
        : m_Skeleton(skeleton)
        , m_FilePath(filename)
        , m_AnimationName(animationName)
    {

        LUMOS_LOG_INFO("Loading animation: {0}", m_FilePath);

        if(!skeleton.get() || !skeleton->IsValid())
        {
            LUMOS_LOG_ERROR("Invalid skeleton passed to animation asset for file '{0}'", m_FilePath);
            SetFlag(AssetFlag::Invalid);
            return;
        }
    }
}
