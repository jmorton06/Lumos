#include "Precompiled.h"
#include "MeshOptimiser.h"
#include "Model.h"
#include "Material.h"
#include "Maths/MathsUtilities.h"

#include <ModelLoaders/meshoptimizer/src/meshoptimizer.h>
#include <cstdio>

namespace Lumos
{
    namespace Graphics
    {
        namespace
        {
            SharedPtr<Mesh> MakeMesh(const Mesh& src, const TDArray<u32>& indices, const TDArray<Vertex>& vertices)
            {
                // Derived meshes always keep CPU data so they can be re-simplified/exported
                bool prev = Mesh::GetRetainCPUVertices();
                Mesh::SetRetainCPUVertices(true);
                SharedPtr<Mesh> mesh = CreateSharedPtr<Mesh>(indices, vertices);
                Mesh::SetRetainCPUVertices(prev);

                mesh->SetName(src.GetName());
                mesh->SetMaterial(src.GetMaterial());
                return mesh;
            }

            // Drop vertices the index buffer no longer references and remap indices.
            void CompactVertices(TDArray<u32>& indices, const TDArray<Vertex>& srcVerts, TDArray<Vertex>& outVerts)
            {
                TDArray<u32> remap;
                remap.Resize(srcVerts.Size());
                size_t unique = meshopt_optimizeVertexFetchRemap(remap.Data(), indices.Data(), indices.Size(), srcVerts.Size());

                outVerts.Resize((u32)unique);
                meshopt_remapVertexBuffer(outVerts.Data(), srcVerts.Data(), srcVerts.Size(), sizeof(Vertex), remap.Data());
                meshopt_remapIndexBuffer(indices.Data(), indices.Data(), indices.Size(), remap.Data());
            }
        }

        void MeshOptimiser::WeldArrays(TDArray<u32>& indices, TDArray<Vertex>& vertices)
        {
            if(vertices.Empty())
                return;

            // Compare real fields only - struct padding is uninitialised
            meshopt_Stream streams[] = {
                { &vertices[0].Position, sizeof(Vec3), sizeof(Vertex) },
                { &vertices[0].TexCoords, sizeof(Vec2), sizeof(Vertex) },
                { &vertices[0].Normal, sizeof(Vec3), sizeof(Vertex) },
                { &vertices[0].Colours, sizeof(Vec4), sizeof(Vertex) },
            };

            const bool hasIndices = !indices.Empty();
            size_t indexCount     = hasIndices ? indices.Size() : vertices.Size();

            TDArray<u32> remap;
            remap.Resize(vertices.Size());
            size_t unique = meshopt_generateVertexRemapMulti(remap.Data(), hasIndices ? indices.Data() : nullptr, indexCount,
                                                             vertices.Size(), streams, sizeof(streams) / sizeof(streams[0]));

            TDArray<u32> newIndices;
            newIndices.Resize((u32)indexCount);
            meshopt_remapIndexBuffer(newIndices.Data(), hasIndices ? indices.Data() : nullptr, indexCount, remap.Data());

            TDArray<Vertex> newVerts;
            newVerts.Resize((u32)unique);
            meshopt_remapVertexBuffer(newVerts.Data(), vertices.Data(), vertices.Size(), sizeof(Vertex), remap.Data());

            indices  = newIndices;
            vertices = newVerts;
        }

        void MeshOptimiser::OptimiseArrays(TDArray<u32>& indices, TDArray<Vertex>& vertices)
        {
            if(indices.Empty() || vertices.Empty())
                return;

            meshopt_optimizeVertexCache(indices.Data(), indices.Data(), indices.Size(), vertices.Size());
            meshopt_optimizeOverdraw(indices.Data(), indices.Data(), indices.Size(), &vertices[0].Position.x,
                                     vertices.Size(), sizeof(Vertex), 1.05f);

            TDArray<Vertex> fetched;
            fetched.Resize(vertices.Size());
            size_t used = meshopt_optimizeVertexFetch(fetched.Data(), indices.Data(), indices.Size(), vertices.Data(),
                                                      vertices.Size(), sizeof(Vertex));
            fetched.Resize((u32)used);
            vertices = fetched;
        }

        SharedPtr<Mesh> MeshOptimiser::Simplify(const Mesh& src, const SimplifySettings& settings, float* outError)
        {
            LUMOS_PROFILE_FUNCTION();
            if(!src.HasCPUVertices())
            {
                LWARN("MeshOptimiser::Simplify: '%s' has no CPU vertices (load with RetainCPUVertices on)", src.GetName().c_str());
                return nullptr;
            }

            // Loaders like OBJ emit one vertex per corner - nothing can collapse until those are welded
            TDArray<Vertex> verts = src.GetCPUVertices();
            TDArray<u32> indices  = src.GetCPUIndices();
            WeldArrays(indices, verts);
            if(indices.Size() < 3)
                return nullptr;

            size_t target = (size_t)((float)indices.Size() * Maths::Clamp(settings.TargetRatio, 0.0f, 1.0f));
            target        = Maths::Max<size_t>(3, target - target % 3);

            TDArray<u32> out;
            out.Resize(indices.Size());
            float error      = 0.0f;
            size_t outCount  = 0;
            const float* pos = &verts[0].Position.x;

            if(settings.Sloppy)
            {
                outCount = meshopt_simplifySloppy(out.Data(), indices.Data(), indices.Size(), pos, verts.Size(), sizeof(Vertex),
                                                  nullptr, target, settings.TargetError, &error);
            }
            else
            {
                unsigned int options = 0;
                if(settings.LockBorder)
                    options |= meshopt_SimplifyLockBorder;
                if(settings.Prune)
                    options |= meshopt_SimplifyPrune;
                if(settings.Regularize)
                    options |= meshopt_SimplifyRegularize;

                if(settings.UseAttributes)
                {
                    // uv.xy, normal.xyz packed tight - Vertex has padding between fields
                    TDArray<float> attribs;
                    attribs.Resize(verts.Size() * 5);
                    for(u32 i = 0; i < verts.Size(); i++)
                    {
                        float* a = &attribs[i * 5];
                        a[0]     = verts[i].TexCoords.x;
                        a[1]     = verts[i].TexCoords.y;
                        a[2]     = verts[i].Normal.x;
                        a[3]     = verts[i].Normal.y;
                        a[4]     = verts[i].Normal.z;
                    }
                    const float weights[5] = { 1.0f, 1.0f, 0.5f, 0.5f, 0.5f };
                    outCount = meshopt_simplifyWithAttributes(out.Data(), indices.Data(), indices.Size(), pos, verts.Size(), sizeof(Vertex),
                                                              attribs.Data(), sizeof(float) * 5, weights, 5, nullptr,
                                                              target, settings.TargetError, options, &error);
                }
                else
                {
                    outCount = meshopt_simplify(out.Data(), indices.Data(), indices.Size(), pos, verts.Size(), sizeof(Vertex),
                                                target, settings.TargetError, options, &error);
                }
            }

            if(outCount < 3)
            {
                LWARN("MeshOptimiser::Simplify: '%s' collapsed to nothing", src.GetName().c_str());
                return nullptr;
            }

            out.Resize((u32)outCount);
            TDArray<Vertex> outVerts;
            CompactVertices(out, verts, outVerts);

            if(settings.RecomputeNormals)
                Mesh::GenerateNormals(outVerts.Data(), outVerts.Size(), out.Data(), out.Size());

            meshopt_optimizeVertexCache(out.Data(), out.Data(), out.Size(), outVerts.Size());

            if(outError)
                *outError = error;
            return MakeMesh(src, out, outVerts);
        }

        SharedPtr<Mesh> MeshOptimiser::Optimise(const Mesh& src)
        {
            LUMOS_PROFILE_FUNCTION();
            if(!src.HasCPUVertices())
                return nullptr;

            TDArray<u32> indices   = src.GetCPUIndices();
            TDArray<Vertex> verts  = src.GetCPUVertices();
            WeldArrays(indices, verts);
            OptimiseArrays(indices, verts);
            return MakeMesh(src, indices, verts);
        }

        MeshAnalysis MeshOptimiser::Analyse(const Mesh& mesh)
        {
            MeshAnalysis result;
            const TDArray<u32>& indices    = mesh.GetCPUIndices();
            const TDArray<Vec3>& positions = mesh.GetCPUPositions();
            result.IndexCount              = indices.Size();
            result.TriangleCount           = indices.Size() / 3;
            result.VertexCount             = positions.Size();
            if(indices.Empty() || positions.Empty())
                return result;

            auto vc          = meshopt_analyzeVertexCache(indices.Data(), indices.Size(), positions.Size(), 16, 0, 0);
            auto vf          = meshopt_analyzeVertexFetch(indices.Data(), indices.Size(), positions.Size(), sizeof(Vertex));
            auto od          = meshopt_analyzeOverdraw(indices.Data(), indices.Size(), &positions[0].x, positions.Size(), sizeof(Vec3));
            result.ACMR      = vc.acmr;
            result.ATVR      = vc.atvr;
            result.Overfetch = vf.overfetch;
            result.Overdraw  = od.overdraw;
            return result;
        }

        u32 MeshOptimiser::GenerateLODs(Model& model, u32 count, float ratioStep, const SimplifySettings& base, float firstScreenSize, bool sloppyLast)
        {
            LUMOS_PROFILE_FUNCTION();
            model.ClearLODs();
            const auto& lod0 = model.GetMeshes();

            TDArray<SharedPtr<Mesh>> prev = lod0;
            float ratio      = 1.0f;
            float screenSize = firstScreenSize * 2.0f; // halved before the first level
            u32 made         = 0;

            for(u32 level = 1; level <= count; level++)
            {
                ratio *= ratioStep;
                screenSize *= 0.5f;

                SimplifySettings settings = base;
                settings.TargetRatio      = ratio;
                // loosen error budget as we go so coarse levels can actually hit their target
                settings.TargetError = base.TargetError * (float)(1u << (level - 1));
                if(sloppyLast && level == count)
                    settings.Sloppy = true;

                TDArray<SharedPtr<Mesh>> meshes;
                float maxError = 0.0f;
                bool progressed = false;
                for(u32 i = 0; i < lod0.Size(); i++)
                {
                    float err            = 0.0f;
                    SharedPtr<Mesh> mesh = lod0[i] ? Simplify(*lod0[i], settings, &err) : nullptr;
                    // too small to reduce further, or not by enough to be worth a copy: reuse the level before
                    if(!mesh || mesh->GetCPUIndices().Size() * 100 > prev[i]->GetCPUIndices().Size() * 85)
                        mesh = prev[i];
                    else
                        progressed = true;
                    maxError = Maths::Max(maxError, err);
                    meshes.PushBack(mesh);
                }

                if(!progressed)
                    break;

                model.AddLOD(meshes, screenSize, maxError);
                prev = meshes;
                made++;
            }
            return made;
        }

        u32 MeshOptimiser::MergeByMaterial(Model& model, float cellSize)
        {
            LUMOS_PROFILE_FUNCTION();
            struct Bucket
            {
                Material* Mat;
                i32 Cell[3];
                TDArray<u32> Members;
                u64 Vertices;
            };

            auto& meshes = model.GetMeshesRef();
            const float inv = 1.0f / Maths::Max(0.001f, cellSize);
            TDArray<Bucket> buckets;
            TDArray<SharedPtr<Mesh>> result;
            for(u32 i = 0; i < meshes.Size(); i++)
            {
                const SharedPtr<Mesh>& mesh = meshes[i];
                if(!mesh || mesh->GetAnimVertexBuffer() || !mesh->HasCPUVertices())
                {
                    result.PushBack(mesh);
                    continue;
                }
                const Vec3 c     = mesh->GetBoundingBox().Center();
                const i32 cell[3] = { (i32)floorf(c.x * inv), (i32)floorf(c.y * inv), (i32)floorf(c.z * inv) };
                const u64 count  = mesh->GetCPUVertices().Size();
                Bucket* found    = nullptr;
                for(auto& b : buckets)
                    if(b.Mat == mesh->GetMaterial().get() && b.Cell[0] == cell[0] && b.Cell[1] == cell[1] && b.Cell[2] == cell[2]
                       && b.Vertices + count < 0xFFFFFFFFull)
                    {
                        found = &b;
                        break;
                    }
                if(!found)
                {
                    buckets.EmplaceBack();
                    found = &buckets.Back();
                    found->Mat = mesh->GetMaterial().get();
                    memcpy(found->Cell, cell, sizeof(cell));
                    found->Vertices = 0;
                }
                found->Members.PushBack(i);
                found->Vertices += count;
            }

            for(auto& b : buckets)
            {
                if(b.Members.Size() == 1)
                {
                    result.PushBack(meshes[b.Members[0]]);
                    continue;
                }
                TDArray<Vertex> vertices;
                TDArray<u32> indices;
                vertices.Reserve((u32)b.Vertices);
                for(u32 m : b.Members)
                {
                    const Mesh& src = *meshes[m];
                    const u32 base  = (u32)vertices.Size();
                    for(const Vertex& v : src.GetCPUVertices())
                        vertices.PushBack(v);
                    for(u32 idx : src.GetCPUIndices())
                        indices.PushBack(base + idx);
                }
                auto merged = CreateSharedPtr<Mesh>(indices, vertices);
                merged->SetMaterial(meshes[b.Members[0]]->GetMaterial());
                merged->SetName(meshes[b.Members[0]]->GetName());
                result.PushBack(merged);
            }

            LINFO("MergeByMaterial: %u meshes -> %u (cell %.1f)", (u32)meshes.Size(), (u32)result.Size(), cellSize);
            meshes = std::move(result);
            model.ClearLODs();
            return (u32)meshes.Size();
        }

        bool MeshOptimiser::ExportOBJ(const TDArray<SharedPtr<Mesh>>& meshes, const std::string& path)
        {
            FILE* f = fopen(path.c_str(), "w");
            if(!f)
            {
                LERROR("ExportOBJ: can't open %s", path.c_str());
                return false;
            }

            fprintf(f, "# Exported by Lumos\n");
            u32 base    = 1; // OBJ indices are 1-based and global
            u32 written = 0;
            for(auto& mesh : meshes)
            {
                if(!mesh)
                    continue;

                const TDArray<u32>& indices = mesh->GetCPUIndices();
                const bool full             = mesh->HasCPUVertices();
                const u32 vcount            = full ? mesh->GetCPUVertices().Size() : mesh->GetCPUPositions().Size();

                fprintf(f, "o %s\n", mesh->GetName().empty() ? "mesh" : mesh->GetName().c_str());
                for(u32 i = 0; i < vcount; i++)
                {
                    Vec3 p = full ? mesh->GetCPUVertices()[i].Position : mesh->GetCPUPositions()[i];
                    fprintf(f, "v %f %f %f\n", p.x, p.y, p.z);
                }
                if(full)
                {
                    for(u32 i = 0; i < vcount; i++)
                    {
                        const Vertex& v = mesh->GetCPUVertices()[i];
                        fprintf(f, "vt %f %f\n", v.TexCoords.x, 1.0f - v.TexCoords.y);
                    }
                    for(u32 i = 0; i < vcount; i++)
                    {
                        const Vertex& v = mesh->GetCPUVertices()[i];
                        fprintf(f, "vn %f %f %f\n", v.Normal.x, v.Normal.y, v.Normal.z);
                    }
                }

                for(u32 i = 0; i + 2 < indices.Size(); i += 3)
                {
                    u32 a = indices[i] + base, b = indices[i + 1] + base, c = indices[i + 2] + base;
                    if(full)
                        fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n", a, a, a, b, b, b, c, c, c);
                    else
                        fprintf(f, "f %u %u %u\n", a, b, c);
                }
                base += vcount;
                written++;
            }
            fclose(f);
            LINFO("ExportOBJ: wrote %u meshes to %s", written, path.c_str());
            return written > 0;
        }
    }
}
