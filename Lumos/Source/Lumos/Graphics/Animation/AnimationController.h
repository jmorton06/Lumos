#pragma once
#include "Core/Reference.h"
#include "Utilities/TimeStep.h"
#include "Animation.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
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

            std::vector<glm::vec3> LocalTranslations;
            std::vector<glm::vec3> LocalScales;
            std::vector<glm::quat> LocalRotations;

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

        // Controls which animation (or animations) is playing on a mesh.
        class AnimationController : public Asset
        {
            friend class Model;

        public:
            AnimationController();
            AnimationController(const AnimationController& copy);

            virtual ~AnimationController();

            void Update(float& animationTime, SamplingContext& context);

            void SetSkeleton(const SharedPtr<Skeleton>& skeleton);
            void SetCurrentState(size_t index) { m_StateIndex = index; };
            void SetCurrentState(const std::string& name);
            void AddState(const std::string_view name, const SharedPtr<Animation>& animation);
            void SetState(size_t index, const std::string_view name, const SharedPtr<Animation>& animation);

            size_t GetCurrentState() const { return m_StateIndex; }

            const SharedPtr<Skeleton>& GetSkeleton() const { return m_Skeleton; }
            const std::vector<std::string>& GetStateNames() const { return m_AnimationNames; }
            const std::vector<SharedPtr<Animation>>& GetAnimationStates() const { return m_AnimationStates; }
            SharedPtr<DescriptorSet> GetDescriptorSet();

            static AssetType GetStaticType() { return AssetType::AnimationController; }
            virtual AssetType GetAssetType() const override { return GetStaticType(); }

            std::vector<glm::mat4> GetJointMatrices();
            void DebugDraw(const glm::mat4& transform);

        private:
            void updateSampling(float ratio, SamplingContext& context);

        private:
            SharedPtr<Skeleton> m_Skeleton;
            std::vector<SharedPtr<Animation>> m_AnimationStates;
            std::vector<std::string> m_AnimationNames;

            ozz::vector<ozz::math::Float4x4> m_JointWorldMats;
            std::vector<glm::mat4> m_BindPoses;
            ozz::vector<uint16_t> m_JointRemap;

            SharedPtr<DescriptorSet> m_Descriptor;

            size_t m_StateIndex = 0;
        };
    }
}
