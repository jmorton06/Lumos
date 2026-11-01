#pragma once
#include "Core/Reference.h"
#include "Utilities/TimeStep.h"
#include "Animation.h"
#include "Maths/Vector3.h"
#include "Maths/Matrix4.h"
#include "Core/DataStructures/TDArray.h"

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
            AnimationController(const AnimationController& copy) = delete;

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

            // Layered playback for one character: any number of clips at once, each with its own
            // clock, speed and weight, blended together. Tick() advances and poses it.
            int FindClip(const std::string& name) const;
            float GetClipDuration(int clip) const;
            // fade this clip in and every other one out
            void Play(int clip, float fadeSeconds, bool loop = true, float speed = 1.0f, bool restart = false);
            // set a clip's weight directly (blend spaces); 0 removes it
            void SetLayerWeight(int clip, float weight, bool loop = true);
            void FadeLayer(int clip, float targetWeight, float seconds);
            void SetLayerSpeed(int clip, float speed);
            void SetLayerTime(int clip, float seconds);
            float GetLayerTime(int clip) const;   // -1 when not playing
            float GetLayerWeight(int clip) const; // 0 when not playing
            bool IsLayerFinished(int clip) const; // a non-looping clip that reached its end
            bool HasLayers() const;
            void Tick(float dt);

            // Root motion: the named joint's horizontal travel is taken out of the pose (the character
            // stays over its origin) and summed here instead, for the game to move the body by.
            // lockRotation: the joint's turning is taken out too (held at the clip's first frame) and
            // summed as a yaw angle, for clips that turn the character round
            void SetRootMotionJoint(int joint, bool lockRotation = false);
            Vec3 ConsumeRootMotion();   // model space, since the last call
            float ConsumeRootRotation(); // degrees about the model's up axis, since the last call

            int FindJoint(const std::string& name) const;
            // joint transform in the model's space (multiply by the entity's world matrix)
            bool GetJointModelMatrix(int joint, Mat4& out) const;

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
