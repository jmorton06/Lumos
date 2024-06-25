#include "Precompiled.h"
#include "Skeleton.h"
#include <glm/gtc/type_ptr.hpp>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/animation/offline/raw_skeleton.h>

#include <ozz/animation/offline/skeleton_builder.h>

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

namespace Lumos
{
    namespace Graphics
    {
        Skeleton::Skeleton(const std::string& filename)
            : m_FilePath(filename)
        {
            ozz::animation::offline::RawSkeleton rawSkeleton;
        }

        Skeleton::Skeleton(ozz::animation::Skeleton* skeleton)
            : m_Skeleton(skeleton)
        {
        }

        Skeleton::~Skeleton()
        {
        }
    }
}
