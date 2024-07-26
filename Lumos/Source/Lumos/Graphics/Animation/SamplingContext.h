#pragma once
#include "Core/Reference.h"
#include "Utilities/TimeStep.h"
#include "Animation.h"

#include "Maths/Vector3.h"
#include "Maths/Quaternion.h"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

namespace Lumos
{
    namespace Graphics
    {
        class DescriptorSet;

        struct SamplingContext
        {
            SamplingContext();
            SamplingContext(const SamplingContext& other);

            ~SamplingContext();

            SamplingContext& operator=(const SamplingContext& other);

            std::vector<Vec3> LocalTranslations;
            std::vector<Vec3> LocalScales;
            std::vector<Quat> LocalRotations;

            ozz::vector<ozz::math::SoaTransform>& GetLocalTransforms() { return m_LocalSpaceSoaTransforms; }
            const ozz::vector<ozz::math::SoaTransform>& GetLocalTransforms() const { return m_LocalSpaceSoaTransforms; }

        private:
            void resize(uint32_t size);
            void resizeSao(uint32_t size);

        private:
            ozz::animation::SamplingJob::Context m_Context;
            ozz::vector<ozz::math::SoaTransform> m_LocalSpaceSoaTransforms;

            uint32_t m_SaoSize = 0;
            uint32_t m_Size    = 0;

            friend class AnimationController;
        };
    }
}
