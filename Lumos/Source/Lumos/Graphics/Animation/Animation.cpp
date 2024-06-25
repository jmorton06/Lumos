#include "Precompiled.h"
#include "Animation.h"
#include "Skeleton.h"

#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

namespace Lumos
{
    namespace Graphics
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

        Animation::Animation(const std::string& animationName, ozz::animation::Animation* animation, SharedPtr<Skeleton> skeleton)
            : m_Skeleton(skeleton)
            , m_AnimationName(animationName)
            , m_Animation(animation)
        {
            LUMOS_LOG_INFO("Loading animation: {0}", animationName);
        }

        Animation::~Animation()
        {
        }
    }
}
