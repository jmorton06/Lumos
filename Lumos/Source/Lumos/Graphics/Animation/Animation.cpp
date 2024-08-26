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

            LINFO("Loading animation: %s", m_FilePath.c_str());

            if(!skeleton.get() || !skeleton->IsValid())
            {
                LERROR("Invalid skeleton passed to animation asset for file '%s'", m_FilePath.c_str());
                SetFlag(AssetFlag::Invalid);
                return;
            }
        }

        Animation::Animation(const std::string& animationName, ozz::animation::Animation* animation, SharedPtr<Skeleton> skeleton)
            : m_Skeleton(skeleton)
            , m_AnimationName(animationName)
            , m_Animation(animation)
        {
            LINFO("Loading animation: %s", animationName.c_str());
        }

        Animation::~Animation()
        {
        }
    }
}
