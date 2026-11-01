#include "Precompiled.h"
#include "Skeleton.h"
#include "AnimationController.h"
#include "SamplingContext.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Shader.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Maths/MathsUtilities.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/base/memory/allocator.h>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/base/span.h>
#include <memory>
#include <vector>

#include "Maths/Quaternion.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    namespace Graphics
    {
        // ozz's default heap allocator asserts on live blocks when it is destroyed at exit,
        // before the asset caches holding skeletons and clips let them go
        class OzzAllocator : public ozz::memory::Allocator
        {
            // same header as ozz's own heap allocator, so a block from either frees with either
            struct Header
            {
                void* Unaligned;
                size_t Size;
            };

        public:
            void* Allocate(size_t size, size_t alignment) override
            {
                char* raw = (char*)malloc(size + sizeof(Header) + alignment - 1);
                if(!raw)
                    return nullptr;
                char* aligned = (char*)(((uintptr_t)raw + sizeof(Header) + alignment - 1) & ~(uintptr_t)(alignment - 1));
                Header* header    = (Header*)(aligned - sizeof(Header));
                header->Unaligned = raw;
                header->Size      = size;
                return aligned;
            }
            void Deallocate(void* block) override
            {
                if(block)
                    free(((Header*)((char*)block - sizeof(Header)))->Unaligned);
            }
        };

        void InstallOzzAllocator()
        {
            static OzzAllocator* allocator = new OzzAllocator(); // never destroyed: blocks outlive static teardown
            ozz::memory::SetDefaulAllocator(allocator);
        }

        struct AnimationLayer
        {
            int Clip         = 0;
            float Time       = 0.0f;
            float Speed      = 1.0f;
            float Weight     = 0.0f;
            float Target     = 0.0f;
            float FadeRate   = 0.0f; // weight per second towards Target; 0 = set directly
            bool Loop        = true;
            bool Finished    = false;
            bool HasRoot     = false;
            Vec3 LastRoot    = Vec3(0.0f);
            bool Wrapped     = false;
            float LockRot[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // the root's rotation it is held at
            float LastRot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            std::unique_ptr<ozz::animation::SamplingJob::Context> Context;
            ozz::vector<ozz::math::SoaTransform> Locals;
        };

        struct AnimationData
        {
            TDArray<SharedPtr<Animation>> m_AnimationStates;
            TDArray<std::string> m_AnimationNames;
            ozz::vector<ozz::math::Float4x4> m_JointWorldMats;
            TDArray<Mat4> m_BindPoses;
            ozz::vector<uint16_t> m_JointRemap;

            std::vector<AnimationLayer> m_Layers;
            ozz::vector<ozz::math::SoaTransform> m_Blended;
            int m_RootJoint   = -1;
            bool m_RootLockRotation = false;
            Vec3 m_RootMotion = Vec3(0.0f);
            float m_RootYaw   = 0.0f;

            AnimationLayer* Find(int clip)
            {
                for(auto& layer : m_Layers)
                    if(layer.Clip == clip)
                        return &layer;
                return nullptr;
            }
            AnimationLayer& Get(int clip)
            {
                if(AnimationLayer* layer = Find(clip))
                    return *layer;
                m_Layers.emplace_back();
                m_Layers.back().Clip = clip;
                return m_Layers.back();
            }
        };

        Mat4 ConvertToGLM(const ozz::math::Float4x4& ozzMat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Mat4 glmMat;

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

            glmMat = Mat4(matrix);
            return glmMat;
        }

        AnimationController::AnimationController()
        {
            m_Data = new AnimationData();
        }

        AnimationController::~AnimationController()
        {
            delete m_Data;
        }

        void AnimationController::Update(float& animationTime, SamplingContext& context)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Data->m_AnimationStates.Empty())
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
                    LERROR("Failed to run ozz LocalToModelJob");
                }
            }
        }

        void AnimationController::SetSkeleton(const SharedPtr<Skeleton>& skeleton)
        {
            m_Skeleton = skeleton;
        }

        void AnimationController::SetCurrentState(const std::string& name)
        {
            for(size_t i = 0; i < m_Data->m_AnimationNames.Size(); ++i)
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
                    LERROR("State with the same name {} already exists", name);
                    return;
                }
            }
            m_Data->m_AnimationNames.PushBack(std::string(name));
            m_Data->m_AnimationStates.PushBack(animation);
        }

        void AnimationController::SetState(size_t index, const std::string_view name, const SharedPtr<Animation>& animation)
        {
            m_Data->m_AnimationNames[index]  = name;
            m_Data->m_AnimationStates[index] = animation;
        }

        const TDArray<std::string>& AnimationController::GetStateNames() const
        {
            return m_Data->m_AnimationNames;
        }
        const TDArray<SharedPtr<Animation>>& AnimationController::GetAnimationStates() const
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
                descriptorDesc.shader      = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBRAnim")).As<Graphics::Shader>();
                m_Descriptor               = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }

            auto test = GetJointMatrices();
            m_Descriptor->SetUniformBufferData(0, test.Data(), static_cast<uint32_t>(sizeof(Mat4) * test.Size()));
            m_Descriptor->Update();

            return m_Descriptor;
        }

        TDArray<Mat4> AnimationController::GetJointMatrices()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            TDArray<Mat4> glmMats;
            for(size_t i = 0; i < m_Data->m_JointWorldMats.size(); i++)
            {
                ozz::math::Float4x4 skin_matrix = m_Data->m_JointWorldMats[i];
                glmMats.PushBack(ConvertToGLM(skin_matrix) * m_Data->m_BindPoses[i]);
            }

            if(m_Data->m_JointWorldMats.empty())
            {
                LINFO("Using identy for joint matrices");
                for(size_t i = 0; i < 256; i++)
                    glmMats.PushBack(Mat4(1.0f));
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

        void AnimationController::DebugDraw(const Mat4& transform)
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
                Vec3 parentPos = transform * Vec4(Vec3(bonevalues), 1.0f);
                math::StorePtrU(current.cols[3], bonevalues);
                Vec3 currentPos = transform * Vec4(Vec3(bonevalues), 1.0f);

                DebugRenderer::DebugDrawBone(parentPos, currentPos);

                // Next instance.
                ++instances;
                // uniform += 16;

                // Only the joint is rendered for leaves, the bone model isn't.
                if(IsLeaf(m_Skeleton->GetSkeleton(), i))
                {
                    // Copy current joint's raw matrix.
                    DebugRenderer::DebugDrawSphere(0.1f, currentPos, Vec4(0.0f, 1.0f, 0.0f, 1.0f));
                }
            }
        }

        void AnimationController::SetBindPoses(const TDArray<Mat4>& mats)
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
                LERROR("ozz animation sampling job failed!");
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

                    ozz::math::Store3PtrU(translations[j], Maths::ValuePtr(context.LocalTranslations[index]));
                    ozz::math::Store3PtrU(scales[j], Maths::ValuePtr(context.LocalScales[index]));
                    ozz::math::StorePtrU(rotations[j], Maths::ValuePtr(context.LocalRotations[index]));
                }
            }
        }
            int AnimationController::FindClip(const std::string& name) const
        {
            for(u32 i = 0; i < m_Data->m_AnimationNames.Size(); i++)
                if(m_Data->m_AnimationNames[i] == name)
                    return (int)i;
            return -1;
        }

        float AnimationController::GetClipDuration(int clip) const
        {
            if(clip < 0 || clip >= (int)m_Data->m_AnimationStates.Size())
                return 0.0f;
            return m_Data->m_AnimationStates[clip]->GetAnimation().duration();
        }

        void AnimationController::Play(int clip, float fadeSeconds, bool loop, float speed, bool restart)
        {
            if(clip < 0 || clip >= (int)m_Data->m_AnimationStates.Size())
                return;
            const float rate = fadeSeconds > 0.0f ? 1.0f / fadeSeconds : 0.0f;
            for(auto& layer : m_Data->m_Layers)
            {
                if(layer.Clip == clip)
                    continue;
                layer.Target   = 0.0f;
                layer.FadeRate = rate;
                if(rate == 0.0f)
                    layer.Weight = 0.0f;
            }
            const bool fresh      = m_Data->Find(clip) == nullptr;
            AnimationLayer& layer = m_Data->Get(clip);
            if(fresh || restart || layer.Finished)
            {
                layer.Time     = 0.0f;
                layer.Finished = false;
            }
            layer.Loop     = loop;
            layer.Speed    = speed;
            layer.Target   = 1.0f;
            layer.FadeRate = rate;
            if(rate == 0.0f || m_Data->m_Layers.size() == 1)
                layer.Weight = 1.0f;
            m_StateIndex = (size_t)clip;
        }

        void AnimationController::SetLayerWeight(int clip, float weight, bool loop)
        {
            if(clip < 0 || clip >= (int)m_Data->m_AnimationStates.Size())
                return;
            AnimationLayer& layer = m_Data->Get(clip);
            layer.Weight          = weight;
            layer.Target          = weight;
            layer.FadeRate        = 0.0f;
            layer.Loop            = loop;
        }

        void AnimationController::FadeLayer(int clip, float targetWeight, float seconds)
        {
            if(clip < 0 || clip >= (int)m_Data->m_AnimationStates.Size())
                return;
            AnimationLayer& layer = m_Data->Get(clip);
            layer.Target          = targetWeight;
            layer.FadeRate        = seconds > 0.0f ? 1.0f / seconds : 0.0f;
            if(layer.FadeRate == 0.0f)
                layer.Weight = targetWeight;
        }

        void AnimationController::SetLayerSpeed(int clip, float speed)
        {
            if(AnimationLayer* layer = m_Data->Find(clip))
                layer->Speed = speed;
        }

        void AnimationController::SetLayerTime(int clip, float seconds)
        {
            if(AnimationLayer* layer = m_Data->Find(clip))
            {
                layer->Time     = seconds;
                layer->Finished = false;
            }
        }

        float AnimationController::GetLayerTime(int clip) const
        {
            AnimationLayer* layer = m_Data->Find(clip);
            return layer ? layer->Time : -1.0f;
        }

        float AnimationController::GetLayerWeight(int clip) const
        {
            AnimationLayer* layer = m_Data->Find(clip);
            return layer ? layer->Weight : 0.0f;
        }

        bool AnimationController::IsLayerFinished(int clip) const
        {
            AnimationLayer* layer = m_Data->Find(clip);
            return layer && layer->Finished;
        }

        bool AnimationController::HasLayers() const
        {
            return !m_Data->m_Layers.empty();
        }

        void AnimationController::Tick(float dt)
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_Skeleton || !m_Skeleton->IsValid() || m_Data->m_AnimationStates.Empty())
                return;

            const ozz::animation::Skeleton& skeleton = m_Skeleton->GetSkeleton();
            const int numJoints                      = skeleton.num_joints();
            const int numSoa                         = skeleton.num_soa_joints();

            // clocks and fades
            auto& layers = m_Data->m_Layers;
            for(auto& layer : layers)
            {
                const float duration = GetClipDuration(layer.Clip);
                layer.Time += dt * layer.Speed;
                if(duration > 0.0f)
                {
                    if(layer.Loop)
                    {
                        layer.Wrapped = layer.Time >= duration || layer.Time < 0.0f;
                        layer.Time    = fmodf(layer.Time, duration);
                        if(layer.Time < 0.0f)
                            layer.Time += duration;
                    }
                    else if(layer.Time >= duration)
                    {
                        layer.Time     = duration;
                        layer.Finished = true;
                    }
                    else if(layer.Time < 0.0f)
                        layer.Time = 0.0f;
                }
                if(layer.FadeRate > 0.0f)
                {
                    const float step = layer.FadeRate * dt;
                    if(layer.Weight < layer.Target)
                        layer.Weight = Maths::Min(layer.Target, layer.Weight + step);
                    else
                        layer.Weight = Maths::Max(layer.Target, layer.Weight - step);
                }
            }
            layers.erase(std::remove_if(layers.begin(), layers.end(), [](const AnimationLayer& l)
                                        { return l.Target <= 0.0f && l.Weight <= 0.0f; }),
                         layers.end());
            if(layers.empty())
                return;

            // sample every clip that shows
            ozz::vector<ozz::animation::BlendingJob::Layer> blendLayers;
            const ozz::math::SoaTransform* only = nullptr;
            float total                         = 0.0f;
            Vec3 rootDelta(0.0f);
            float rootYaw = 0.0f;
            for(auto& layer : layers)
            {
                if(layer.Weight <= 0.0f)
                    continue;
                if(!layer.Context)
                    layer.Context = std::make_unique<ozz::animation::SamplingJob::Context>();
                layer.Context->Resize(numJoints);
                layer.Locals.resize(numSoa);

                const ozz::animation::Animation& clip = m_Data->m_AnimationStates[layer.Clip]->GetAnimation();
                ozz::animation::SamplingJob sampling;
                sampling.animation = &clip;
                sampling.context   = layer.Context.get();
                sampling.ratio     = clip.duration() > 0.0f ? Maths::Clamp(layer.Time / clip.duration(), 0.0f, 1.0f) : 0.0f;
                sampling.output    = ozz::make_span(layer.Locals);
                if(!sampling.Run())
                    continue;

                if(m_Data->m_RootJoint >= 0 && m_Data->m_RootJoint < numJoints)
                {
                    ozz::math::SoaTransform& soa = layer.Locals[m_Data->m_RootJoint / 4];
                    const int lane               = m_Data->m_RootJoint % 4;
                    float xs[4], zs[4];
                    ozz::math::StorePtrU(soa.translation.x, xs);
                    ozz::math::StorePtrU(soa.translation.z, zs);
                    const Vec3 root(xs[lane], 0.0f, zs[lane]);
                    // a loop jumps back to its start: that frame's travel is lost rather than reversed
                    if(layer.HasRoot && !layer.Wrapped)
                        rootDelta += (root - layer.LastRoot) * layer.Weight;
                    layer.LastRoot = root;
                    xs[lane]       = 0.0f;
                    zs[lane]       = 0.0f;
                    soa.translation.x = ozz::math::simd_float4::LoadPtrU(xs);
                    soa.translation.z = ozz::math::simd_float4::LoadPtrU(zs);

                    if(m_Data->m_RootLockRotation)
                    {
                        float q[4][4];
                        ozz::math::StorePtrU(soa.rotation.x, q[0]);
                        ozz::math::StorePtrU(soa.rotation.y, q[1]);
                        ozz::math::StorePtrU(soa.rotation.z, q[2]);
                        ozz::math::StorePtrU(soa.rotation.w, q[3]);
                        const float cur[4] = { q[0][lane], q[1][lane], q[2][lane], q[3][lane] };
                        if(!layer.HasRoot)
                            std::copy(cur, cur + 4, layer.LockRot);
                        else if(!layer.Wrapped)
                        {
                            // cur * conjugate(last): the turn since last frame, as an angle about up
                            const float* l = layer.LastRot;
                            const float dw = cur[3] * l[3] + cur[0] * l[0] + cur[1] * l[1] + cur[2] * l[2];
                            const float dy = -cur[3] * l[1] + cur[1] * l[3] - cur[2] * l[0] + cur[0] * l[2];
                            rootYaw += 2.0f * atan2f(dy, dw) * layer.Weight;
                        }
                        std::copy(cur, cur + 4, layer.LastRot);
                        for(int c = 0; c < 4; c++)
                            q[c][lane] = layer.LockRot[c];
                        soa.rotation.x = ozz::math::simd_float4::LoadPtrU(q[0]);
                        soa.rotation.y = ozz::math::simd_float4::LoadPtrU(q[1]);
                        soa.rotation.z = ozz::math::simd_float4::LoadPtrU(q[2]);
                        soa.rotation.w = ozz::math::simd_float4::LoadPtrU(q[3]);
                    }
                    layer.HasRoot = true;
                }
                layer.Wrapped = false;

                ozz::animation::BlendingJob::Layer blend;
                blend.weight    = layer.Weight;
                blend.transform = ozz::make_span(layer.Locals);
                blendLayers.push_back(blend);
                only = layer.Locals.data();
                total += layer.Weight;
            }
            if(blendLayers.empty())
                return;
            if(total > 0.0f)
            {
                m_Data->m_RootMotion += rootDelta / Maths::Max(total, 1.0f);
                m_Data->m_RootYaw += rootYaw / Maths::Max(total, 1.0f);
            }

            m_Data->m_Blended.resize(numSoa);
            if(blendLayers.size() == 1 && total >= 0.999f)
                std::copy(only, only + numSoa, m_Data->m_Blended.begin());
            else
            {
                ozz::animation::BlendingJob blending;
                blending.threshold = 0.01f;
                blending.layers    = ozz::make_span(blendLayers);
                blending.rest_pose = skeleton.joint_rest_poses();
                blending.output    = ozz::make_span(m_Data->m_Blended);
                if(!blending.Run())
                {
                    LERROR("ozz blending job failed");
                    return;
                }
            }

            m_Data->m_JointWorldMats.resize(numJoints);
            ozz::animation::LocalToModelJob ltm;
            ltm.skeleton = &skeleton;
            ltm.input    = ozz::make_span(m_Data->m_Blended);
            ltm.output   = ozz::make_span(m_Data->m_JointWorldMats);
            if(!ltm.Run())
                LERROR("Failed to run ozz LocalToModelJob");
        }

        void AnimationController::SetRootMotionJoint(int joint, bool lockRotation)
        {
            m_Data->m_RootJoint        = joint;
            m_Data->m_RootLockRotation = lockRotation;
            for(auto& layer : m_Data->m_Layers)
                layer.HasRoot = false;
        }

        Vec3 AnimationController::ConsumeRootMotion()
        {
            const Vec3 motion    = m_Data->m_RootMotion;
            m_Data->m_RootMotion = Vec3(0.0f);
            return motion;
        }

        float AnimationController::ConsumeRootRotation()
        {
            const float yaw   = m_Data->m_RootYaw;
            m_Data->m_RootYaw = 0.0f;
            return Maths::ToDegrees(yaw);
        }

        int AnimationController::FindJoint(const std::string& name) const
        {
            if(!m_Skeleton || !m_Skeleton->IsValid())
                return -1;
            const auto names = m_Skeleton->GetSkeleton().joint_names();
            for(size_t i = 0; i < names.size(); i++)
                if(name == names[i])
                    return (int)i;
            return -1;
        }

        bool AnimationController::GetJointModelMatrix(int joint, Mat4& out) const
        {
            if(joint < 0 || joint >= (int)m_Data->m_JointWorldMats.size())
                return false;
            out = ConvertToGLM(m_Data->m_JointWorldMats[joint]);
            return true;
        }
    }
}
