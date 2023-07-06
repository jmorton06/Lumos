#include "Precompiled.h"
#include "Skeleton.h"
#include <glm/gtc/type_ptr.hpp>

#include <ozz/animation/offline/skeleton_builder.h>

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

namespace Lumos
{

    Skeleton::Skeleton(const std::string& filename)
        : m_FilePath(filename)
    {
        ozz::animation::offline::RawSkeleton rawSkeleton;
    }

}