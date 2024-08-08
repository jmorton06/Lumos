#pragma once
#include "MeshFactory.h"
#include "Core/Asset/Asset.h"
#include "Utilities/TimeStep.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    class AStar;
    namespace Graphics
    {
        class Skeleton;
        class Animation;
        class AnimationController;
        struct SamplingContext;
        class Mesh;

        class Model : public Asset
        {
            template <typename Archive>
            friend void save(Archive& archive, const Model& model);

            template <typename Archive>
            friend void load(Archive& archive, Model& model);

        public:
            Model();
            Model(const std::string& filePath);
            Model(const SharedPtr<Mesh>& mesh, PrimitiveType type);
            Model(PrimitiveType type);

            ~Model();

            TDArray<SharedPtr<Mesh>>& GetMeshesRef();
            const TDArray<SharedPtr<Mesh>>& GetMeshes() const;
            void AddMesh(SharedPtr<Mesh> mesh);

            SharedPtr<Skeleton> GetSkeleton() const;
            const TDArray<SharedPtr<Animation>>& GetAnimations() const;
            SharedPtr<SamplingContext> GetSamplingContext() const;
            SharedPtr<AnimationController> GetAnimationController() const;

            uint32_t GetCurrentAnimationIndex() const { return m_CurrentAnimation; }
            void SetCurrentAnimationIndex(uint32_t index) { m_CurrentAnimation = index; }

            const std::string& GetFilePath() const { return m_FilePath; }
            PrimitiveType GetPrimitiveType() { return m_PrimitiveType; }
            void SetPrimitiveType(PrimitiveType type) { m_PrimitiveType = type; }
            SET_ASSET_TYPE(AssetType::Model);

            void UpdateAnimation(const TimeStep& dt);
            void UpdateAnimation(const TimeStep& dt, float overrideTime);

            TDArray<Mat4> GetJointMatrices();

            Model(const Model&);
            Model& operator=(const Model&);
            Model(Model&&);
            Model& operator=(Model&&);

        private:
            PrimitiveType m_PrimitiveType = PrimitiveType::None;
            TDArray<SharedPtr<Mesh>> m_Meshes;
            std::string m_FilePath;
            TDArray<String8> m_AnimFilePaths;

            SharedPtr<Skeleton> m_Skeleton;
            TDArray<SharedPtr<Animation>> m_Animation;
            SharedPtr<SamplingContext> m_SamplingContext;
            SharedPtr<AnimationController> m_AnimationController;

            uint32_t m_CurrentAnimation = 0;

            TDArray<Mat4> m_BindPoses;

            void LoadOBJ(const std::string& path);
            void LoadGLTF(const std::string& path);
            void LoadFBX(const std::string& path);

        public:
            void LoadModel(const std::string& path);
        };
    }
}
