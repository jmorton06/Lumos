#pragma once
#include "LMeshFormat.h"
#include "Core/DataStructures/TDArray.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/RHIDefinitions.h"
#include "Core/String.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    struct ImportSettings;

    namespace Graphics
    {
        class Skeleton;
        class Animation;
    }

    struct LMeshWriteData
    {
        struct MeshEntry
        {
            std::string Name;
            TDArray<Graphics::Vertex> Vertices;
            TDArray<Graphics::AnimVertex> AnimVertices;
            TDArray<u32> Indices;
            bool Animated = false;
            u32 MaterialIndex = 0;
        };

        struct MaterialEntry
        {
            std::string Name;
            Graphics::MaterialProperties Properties;
            std::string TexturePaths[LMESH_MAX_TEXTURE_SLOTS]; // VFS paths
            Graphics::TextureWrap TextureWraps[LMESH_MAX_TEXTURE_SLOTS] = {};
            Graphics::TextureFilter TextureFilters[LMESH_MAX_TEXTURE_SLOTS] = {};
            u32 Flags = 0; // Material::RenderFlags
        };

        TDArray<MeshEntry> Meshes;
        TDArray<MaterialEntry> Materials;
        SharedPtr<Graphics::Skeleton> Skeleton;
        TDArray<SharedPtr<Graphics::Animation>> Animations;
        TDArray<Mat4> BindPoses;
        u64 AssetUUID = 0;
    };

    class LMeshWriter
    {
    public:
        static bool Write(const String8& outputPath, LMeshWriteData& data, const ImportSettings& settings);

    private:
        static void OptimizeMesh(LMeshWriteData::MeshEntry& mesh, const ImportSettings& settings);
        static void GenerateCollisionMesh(const LMeshWriteData& data, TDArray<Vec3>& outVerts, TDArray<u32>& outIndices, float simplifyRatio);
    };
}
