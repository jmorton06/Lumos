#include "Precompiled.h"
#include "AnimationController.h"

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Lumos
{

    SamplingContext::SamplingContext()
    {
    }
    SamplingContext::SamplingContext(const SamplingContext& other)
        : LocalTranslations(other.LocalTranslations)
        , LocalScales(other.LocalScales)
        , LocalRotations(other.LocalRotations)
        , m_LocalSpaceSoaTransforms(other.m_LocalSpaceSoaTransforms)
    {
        m_Context.Resize(other.m_Context.max_tracks());
    }

    SamplingContext& SamplingContext::operator=(const SamplingContext& other)
    {
        LocalTranslations         = other.LocalTranslations;
        LocalScales               = other.LocalScales;
        LocalRotations            = other.LocalRotations;
        m_LocalSpaceSoaTransforms = other.m_LocalSpaceSoaTransforms;

        m_Context.Resize(other.m_Context.max_tracks());
        return *this;
    }

    void SamplingContext::resize(uint32_t size)
    {
        if(m_Size != size)
        {
            m_Size = size;
            m_Context.Resize(size);
            LocalTranslations.resize(size);
            LocalScales.resize(size);
            LocalRotations.resize(size);
        }
    }

    void SamplingContext::resizeSao(uint32_t size)
    {
        if(m_SaoSize != size)
        {
            m_SaoSize = size;
            m_LocalSpaceSoaTransforms.resize(size);
        }
    }

    void AnimationController::Update(float& animationTime, SamplingContext& context)
    {
        if(m_AnimationStates.empty())
            return;

        float ratio = animationTime / m_AnimationStates[m_StateIndex]->GetAnimation().duration();
        if(ratio >= 1.0f)
        {
            animationTime = 0.0f;
            ratio         = 0.0f;
        }

        if(m_Skeleton.get() && m_Skeleton->IsValid())
        {
            context.resize(m_Skeleton->GetSkeleton().num_joints());
            context.resizeSao(m_Skeleton->GetSkeleton().num_soa_joints());
            updateSampling(ratio, context);
        }
    }
    void AnimationController::SetSkeleton(const SharedPtr<Skeleton>& skeleton)
    {
        m_Skeleton = skeleton;
    }
    void AnimationController::SetCurrentState(const std::string& name)
    {
        for(size_t i = 0; i < m_AnimationNames.size(); ++i)
        {
            if(m_AnimationNames[i] == name)
            {
                m_StateIndex = i;
                return;
            }
        }
    }
    void AnimationController::AddState(const std::string_view name, const SharedPtr<Animation>& animation)
    {
        for(const auto& animName : m_AnimationNames)
        {
            if(animName == name)
            {
                LUMOS_LOG_ERROR("State with the same name {} already exists", name);
                return;
            }
        }
        m_AnimationNames.push_back(std::string(name));
        m_AnimationStates.push_back(animation);
    }
    void AnimationController::SetState(size_t index, const std::string_view name, const SharedPtr<Animation>& animation)
    {
        m_AnimationNames[index]  = name;
        m_AnimationStates[index] = animation;
    }

    void AnimationController::updateSampling(float ratio, SamplingContext& context)
    {
        ozz::animation::SamplingJob sampling_job;
        sampling_job.animation = &m_AnimationStates[m_StateIndex]->GetAnimation();
        sampling_job.context   = &context.m_Context;
        sampling_job.ratio     = ratio;
        sampling_job.output    = ozz::make_span(context.m_LocalSpaceSoaTransforms);
        if(!sampling_job.Run())
        {
            LUMOS_LOG_ERROR("ozz animation sampling job failed!");
        }

        for(int i = 0; i < context.m_LocalSpaceSoaTransforms.size(); ++i)
        {
            ozz::math::SimdFloat4 translations[4];
            ozz::math::SimdFloat4 scales[4];
            ozz::math::SimdFloat4 rotations[4];

            ozz::math::Transpose3x4(&context.m_LocalSpaceSoaTransforms[i].translation.x, translations);
            ozz::math::Transpose3x4(&context.m_LocalSpaceSoaTransforms[i].scale.x, scales);
            ozz::math::Transpose4x4(&context.m_LocalSpaceSoaTransforms[i].rotation.x, rotations);

            for(int j = 0; j < 4; ++j)
            {
                auto index = i * 4 + j;
                if(index >= context.LocalTranslations.size())
                    break;

                ozz::math::Store3PtrU(translations[j], glm::value_ptr(context.LocalTranslations[index]));
                ozz::math::Store3PtrU(scales[j], glm::value_ptr(context.LocalScales[index]));
                ozz::math::StorePtrU(rotations[j], glm::value_ptr(context.LocalRotations[index]));
            }
        }
    }
}