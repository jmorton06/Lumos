#pragma once
#include "Core/Reference.h"
#include "Utilities/TimeStep.h"
#include "Animation.h"

namespace Lumos
{
    namespace Graphics
    {
        class DescriptorSet;
        struct SamplingContext;
        struct AnimationData;

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
            const TDArray<std::string>& GetStateNames() const;
            const TDArray<SharedPtr<Animation>>& GetAnimationStates() const;
            SharedPtr<DescriptorSet> GetDescriptorSet();

            static AssetType GetStaticType() { return AssetType::AnimationController; }
            virtual AssetType GetAssetType() const override { return GetStaticType(); }

            TDArray<Mat4> GetJointMatrices();
            void DebugDraw(const Mat4& transform);

            void SetBindPoses(const TDArray<Mat4>& mats);

        private:
            void UpdateSampling(float ratio, SamplingContext& context);

        private:
            SharedPtr<Skeleton> m_Skeleton;
            SharedPtr<DescriptorSet> m_Descriptor;
            AnimationData* m_Data;

            size_t m_StateIndex = 0;
        };
    }
}
