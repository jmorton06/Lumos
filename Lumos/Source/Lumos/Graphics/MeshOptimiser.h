#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include "Graphics/Mesh.h"
#include <string>

namespace Lumos
{
    namespace Graphics
    {
        class Model;

        struct SimplifySettings
        {
            float TargetRatio   = 0.5f;  // fraction of triangles to keep
            float TargetError   = 0.01f; // max deviation, relative to mesh extents
            bool LockBorder     = false; // keep open edges fixed (needed for meshes that tile/stitch)
            bool Sloppy         = false; // ignore topology - much faster/more aggressive, lower quality
            bool UseAttributes  = true;  // weigh UVs + normals so seams survive
            bool Prune          = false; // drop small disconnected pieces
            bool Regularize     = false; // favour evenly sized triangles (good for deforming meshes)
            bool RecomputeNormals = false;
        };

        struct MeshAnalysis
        {
            u32 VertexCount   = 0;
            u32 IndexCount    = 0;
            u32 TriangleCount = 0;
            float ACMR        = 0.0f; // avg cache miss ratio; lower is better (0.5 best)
            float ATVR        = 0.0f; // avg transformed vertex ratio; 1.0 best
            float Overdraw    = 0.0f; // shaded/covered pixels; 1.0 best
            float Overfetch   = 0.0f; // fetched bytes / vb size; 1.0 best
        };

        namespace MeshOptimiser
        {
            // All mesh ops need the source's CPU vertices - load with Mesh::SetRetainCPUVertices(true).
            SharedPtr<Mesh> Simplify(const Mesh& src, const SimplifySettings& settings, float* outError = nullptr);

            // Weld duplicate vertices, then reorder for vertex cache, overdraw and fetch.
            SharedPtr<Mesh> Optimise(const Mesh& src);

            MeshAnalysis Analyse(const Mesh& mesh);

            // In-place helpers for callers holding raw arrays.
            void OptimiseArrays(TDArray<u32>& indices, TDArray<Vertex>& vertices);
            void WeldArrays(TDArray<u32>& indices, TDArray<Vertex>& vertices);

            // Replaces model LODs 1..count with successively simplified copies of LOD0.
            // Each level keeps ratioStep of the previous level's triangles. Returns levels made.
            u32 GenerateLODs(Model& model, u32 count, float ratioStep, const SimplifySettings& base, float firstScreenSize = 1.0f, bool sloppyLast = false);

            // Static batching: meshes sharing a material whose box centres fall in the same
            // cellSize cube become one mesh. Fewer draws, culling stays per cell. Skinned meshes
            // and meshes without CPU vertices are left alone. Returns the model's mesh count.
            u32 MergeByMaterial(Model& model, float cellSize);

            // Wavefront OBJ with positions, UVs and normals; one object per mesh.
            bool ExportOBJ(const TDArray<SharedPtr<Mesh>>& meshes, const std::string& path);
        }
    }
}
