#pragma once
#include "MeshFactory.h"
#include "Core/Asset/Asset.h"
#include "Utilities/TimeStep.h"
#include "Maths/Matrix4.h"
#include "Core/DataStructures/TDArray.h"

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

            // LOD0 is GetMeshes(). Extra levels are used once the model's bounding sphere
            // covers less than ScreenSize of the view height.
            struct LODLevel
            {
                TDArray<SharedPtr<Mesh>> Meshes;
                float ScreenSize = 0.0f;
                float Error      = 0.0f;
            };
            // LOD levels made for every .lmesh as it loads (0 = off). Needs CPU vertices, so loads are a bit slower.
            static inline u32 s_AutoLODLevels = 0;
            static inline float s_AutoLODRatio       = 0.5f;  // each level keeps this share of the last
            static inline float s_AutoLODError       = 0.02f; // first level's error, relative to the mesh; doubles each level
            static inline float s_AutoLODFirstScreen = 0.3f;  // share of the half view height the first level starts below
            static inline bool s_AutoLODSloppyLast   = false; // last level ignores topology (much smaller, for far away)
            // static batching for every .lmesh loaded from now on: cell size in model units (0 = off)
            static inline float s_AutoMergeCell = 0.0f;
            u32 GetLODCount() const { return 1 + (u32)m_LODs.Size(); }
            const TDArray<SharedPtr<Mesh>>& GetLODMeshes(u32 lod) const;
            LODLevel* GetLODLevel(u32 lod);
            void AddLOD(const TDArray<SharedPtr<Mesh>>& meshes, float screenSize, float error = 0.0f);
            void ClearLODs();
            // screenSize = bounding sphere radius / (distance * tan(fov/2))
            u32 SelectLOD(float screenSize) const;
            void SetForcedLOD(i32 lod) { m_ForcedLOD = lod; }
            i32 GetForcedLOD() const { return m_ForcedLOD; }
            u32 GetLastSelectedLOD() const { return m_LastSelectedLOD; }
            // Local-space bounding sphere over LOD0.
            void GetBoundingSphere(Vec3& centre, float& radius) const;

            SharedPtr<Skeleton> GetSkeleton() const;
            const TDArray<SharedPtr<Animation>>& GetAnimations() const;
            const TDArray<Mat4>& GetBindPoses() const;
            SharedPtr<SamplingContext> GetSamplingContext() const;
            SharedPtr<AnimationController> GetAnimationController() const;

            void SetSkeleton(SharedPtr<Skeleton> skeleton);
            void SetAnimations(const TDArray<SharedPtr<Animation>>& animations);
            void SetBindPoses(const TDArray<Mat4>& bindPoses);

            uint32_t GetCurrentAnimationIndex() const { return m_CurrentAnimation; }
            void SetCurrentAnimationIndex(uint32_t index) { m_CurrentAnimation = index; }

            const std::string& GetFilePath() const { return m_FilePath; }
            PrimitiveType GetPrimitiveType() { return m_PrimitiveType; }
            void SetPrimitiveType(PrimitiveType type) { m_PrimitiveType = type; }
            SET_ASSET_TYPE(AssetType::Model);

            void UpdateAnimation(const TimeStep& dt);
            void UpdateAnimation(const TimeStep& dt, float overrideTime);
            // a controller of its own for one entity: its own clips' clocks, weights and pose
            SharedPtr<AnimationController> CreateAnimator() const;

            TDArray<Mat4> GetJointMatrices();

            Model(const Model&);
            Model& operator=(const Model&);
            Model(Model&&);
            Model& operator=(Model&&);

            bool IsLoading() const { return m_Loading; }

            void LoadModelAsync(const std::string& path);

        private:
            // Move to Animation Component
            PrimitiveType m_PrimitiveType = PrimitiveType::None;
            TDArray<SharedPtr<Mesh>> m_Meshes;
            TDArray<LODLevel> m_LODs;
            i32 m_ForcedLOD = -1;
            mutable u32 m_LastSelectedLOD = 0;
            std::string m_FilePath;
            TDArray<String8> m_AnimFilePaths;
            bool m_Loading = false;

            SharedPtr<Skeleton> m_Skeleton;
            TDArray<SharedPtr<Animation>> m_Animation;
            SharedPtr<SamplingContext> m_SamplingContext;
            SharedPtr<AnimationController> m_AnimationController;

            uint32_t m_CurrentAnimation = 0;
            float m_AnimationTime       = 0.0f;

            TDArray<Mat4> m_BindPoses;

            void LoadOBJ(const std::string& path);
            void LoadGLTF(const std::string& path);
            void LoadFBX(const std::string& path);
            bool LoadLMesh(const std::string& path);
            void AutoGenerateLODs(const std::string& cachePath, u64 cacheKey);
            bool LoadLODCache(const std::string& cachePath, u64 cacheKey);

        public:
            bool LoadLAnim(const std::string& path);
            void LoadModel(const std::string& path);
            // Reads the source file (obj / gltf / fbx) only, never an imported .lmesh. The importer uses it.
            bool LoadFromSource(const std::string& path);
        };
    }
}
