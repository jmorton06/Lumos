#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Skeleton.h"
#include "AnimationController.h"
#include "SamplingContext.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Shader.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

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
#include <glm/ext/matrix_float4x4.hpp>

namespace Lumos
{
    namespace Graphics
    {
        struct AnimationData
        {
            std::vector<SharedPtr<Animation>> m_AnimationStates;
            std::vector<std::string> m_AnimationNames;
            ozz::vector<ozz::math::Float4x4> m_JointWorldMats;
            std::vector<glm::mat4> m_BindPoses;
            ozz::vector<uint16_t> m_JointRemap;
        };

        glm::mat4 ConvertToGLM(const ozz::math::Float4x4& ozzMat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            glm::mat4 glmMat;

            // Assuming ozz::math::Float4x4 is column-major
            float matrix[16];
            for(int r = 0; r < 4; r++)
            {
                float result[4];
                std::memcpy(result, ozzMat.cols + r, sizeof(ozzMat.cols[r]));
                float dresult[4];
                for(int j = 0; j < 4; j++)
                {
                    dresult[j] = result[j];
                }
                //_mm_store_ps(result,p.cols[r]);
                std::memcpy(matrix + r * 4, dresult, sizeof(dresult));
            }

            glmMat = glm::make_mat4(matrix);
            return glmMat;
        }

        AnimationController::AnimationController()
        {
            m_Data = new AnimationData();
        }

        AnimationController::AnimationController(const AnimationController& copy) = default;

        AnimationController::~AnimationController()
        {
            delete m_Data;
        }

        void AnimationController::Update(float& animationTime, SamplingContext& context)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Data->m_AnimationStates.empty())
                return;
            m_Data->m_JointWorldMats = ozz::vector<ozz::math::Float4x4>();

            float ratio = animationTime / m_Data->m_AnimationStates[m_StateIndex]->GetAnimation().duration();
            if(ratio >= 1.0f)
            {
                animationTime = 0.0f;
                ratio         = 0.0f;
            }

            if(m_Skeleton.get() && m_Skeleton->IsValid())
            {
                context.resize(m_Skeleton->GetSkeleton().num_joints());
                context.resizeSao(m_Skeleton->GetSkeleton().num_soa_joints());
                UpdateSampling(ratio, context);

                if(m_Data->m_JointWorldMats.size() != m_Skeleton->GetSkeleton().num_joints())
                    m_Data->m_JointWorldMats.resize(m_Skeleton->GetSkeleton().num_joints());

                // Setup local-to-model conversion job.
                ozz::animation::LocalToModelJob ltmJob;
                ltmJob.skeleton = &m_Skeleton->GetSkeleton();
                ltmJob.input    = ozz::make_span(context.GetLocalTransforms());
                ltmJob.output   = ozz::make_span(m_Data->m_JointWorldMats);

                // Runs ltm job.
                if(!ltmJob.Run())
                {
                    LUMOS_LOG_ERROR("Failed to run ozz LocalToModelJob");
                }
            }
        }

        void AnimationController::SetSkeleton(const SharedPtr<Skeleton>& skeleton)
        {
            m_Skeleton = skeleton;
        }

        void AnimationController::SetCurrentState(const std::string& name)
        {
            for(size_t i = 0; i < m_Data->m_AnimationNames.size(); ++i)
            {
                if(m_Data->m_AnimationNames[i] == name)
                {
                    m_StateIndex = i;
                    return;
                }
            }
        }
        void AnimationController::AddState(const std::string_view name, const SharedPtr<Animation>& animation)
        {
            for(const auto& animName : m_Data->m_AnimationNames)
            {
                if(animName == name)
                {
                    LUMOS_LOG_ERROR("State with the same name {} already exists", name);
                    return;
                }
            }
            m_Data->m_AnimationNames.push_back(std::string(name));
            m_Data->m_AnimationStates.push_back(animation);
        }

        void AnimationController::SetState(size_t index, const std::string_view name, const SharedPtr<Animation>& animation)
        {
            m_Data->m_AnimationNames[index]  = name;
            m_Data->m_AnimationStates[index] = animation;
        }

        const std::vector<std::string>& AnimationController::GetStateNames() const
        {
            return m_Data->m_AnimationNames;
        }
        const std::vector<SharedPtr<Animation>>& AnimationController::GetAnimationStates() const
        {
            return m_Data->m_AnimationStates;
        }

        SharedPtr<DescriptorSet> AnimationController::GetDescriptorSet()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(!m_Descriptor)
            {
                Graphics::DescriptorDesc descriptorDesc {};
                descriptorDesc.layoutIndex = 3;
                descriptorDesc.shader      = Application::Get().GetAssetManager()->GetAssetData("ForwardPBRAnim").As<Graphics::Shader>();
                m_Descriptor               = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }

            auto test = GetJointMatrices();
            m_Descriptor->SetUniform("BoneTransforms", "BoneTransforms", test.data(), (uint32_t)(sizeof(glm::mat4) * test.size()));
            m_Descriptor->Update();

            return m_Descriptor;
        }

        std::vector<glm::mat4> AnimationController::GetJointMatrices()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            std::vector<glm::mat4> glmMats;
            for(size_t i = 0; i < m_Data->m_JointWorldMats.size(); i++)
            {
                ozz::math::Float4x4 skin_matrix = m_Data->m_JointWorldMats[i];
                glmMats.push_back(ConvertToGLM(skin_matrix) * m_Data->m_BindPoses[i]);
            }

            if(m_Data->m_JointWorldMats.empty())
            {
                LUMOS_LOG_INFO("Using identy for joint matrices");
                for(size_t i = 0; i < 100; i++)
                    glmMats.push_back(glm::mat4(1.0f));
            }
            return glmMats;
        }

        bool IsLeaf(const ozz::animation::Skeleton& _skeleton, int _joint)
        {
            const int num_joints = _skeleton.num_joints();
            assert(_joint >= 0 && _joint < num_joints && "_joint index out of range");
            const ozz::span<const int16_t>& parents = _skeleton.joint_parents();
            const int next                          = _joint + 1;
            return next == num_joints || parents[next] != _joint;
        }

        void AnimationController::DebugDraw(const glm::mat4& transform)
        {
            using namespace ozz;
            const int num_joints               = m_Skeleton->GetSkeleton().num_joints();
            const span<const int16_t>& parents = m_Skeleton->GetSkeleton().joint_parents();

            const int _max_instances = animation::Skeleton::kMaxJoints * 2;
            int instances            = 0;
            for(int i = 0; i < num_joints && instances < _max_instances; ++i)
            {
                // Root isn't rendered.
                const int16_t parent_id = parents[i];
                if(parent_id == ozz::animation::Skeleton::kNoParent)
                {
                    continue;
                }

                // Selects joint matrices.
                const math::Float4x4& parent  = m_Data->m_JointWorldMats[parent_id];
                const math::Float4x4& current = m_Data->m_JointWorldMats[i];

                // Copy parent joint's raw matrix, to render a bone between the parent
                // and current matrix.
                // float* uniform = _uniforms + instances * 16;
                // std::memcpy(uniform, parent.cols, 16 * sizeof(float));

                // Set bone direction (bone_dir). The shader expects to find it at index
                // [3,7,11] of the matrix.
                // Index 15 is used to store whether a bone should be rendered,
                // otherwise it's a leaf.
                // float bone_dir[4];
                // math::StorePtrU(current.cols[3] - parent.cols[3], bone_dir);
                //			uniform[3] = bone_dir[0];
                //			uniform[7] = bone_dir[1];
                //			uniform[11] = bone_dir[2];
                //			uniform[15] = 1.f;  // Enables bone rendering.

                float bonevalues[4];
                math::StorePtrU(parent.cols[3], bonevalues);
                glm::vec3 parentPos = transform * glm::vec4(glm::make_vec3(bonevalues), 1.0f);
                math::StorePtrU(current.cols[3], bonevalues);
                glm::vec3 currentPos = transform * glm::vec4(glm::make_vec3(bonevalues), 1.0f);

                DebugRenderer::DebugDrawBone(parentPos, currentPos);

                // Next instance.
                ++instances;
                // uniform += 16;

                // Only the joint is rendered for leaves, the bone model isn't.
                if(IsLeaf(m_Skeleton->GetSkeleton(), i))
                {
                    // Copy current joint's raw matrix.
                    DebugRenderer::DebugDrawSphere(0.1f, currentPos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
                }
            }
        }

        void AnimationController::SetBindPoses(const std::vector<glm::mat4>& mats)
        {
            m_Data->m_BindPoses = mats;
        }

        void AnimationController::UpdateSampling(float ratio, SamplingContext& context)
        {
            LUMOS_PROFILE_FUNCTION();
            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = &m_Data->m_AnimationStates[m_StateIndex]->GetAnimation();
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
}
