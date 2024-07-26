#include "Precompiled.h"
#include "Mesh.h"
#include "RHI/Renderer.h"
#include "RHI/VertexBuffer.h"
#include "RHI/IndexBuffer.h"
#include "Scene/Serialisation/SerialisationImplementation.h"
#include "Core/OS/FileSystem.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Vector3.h"

#include <cereal/archives/json.hpp>
#include <ModelLoaders/meshoptimizer/src/meshoptimizer.h>

namespace Lumos
{
    namespace Graphics
    {
        Mesh::Mesh()
            : m_VertexBuffer(nullptr)
            , m_IndexBuffer(nullptr)
            , m_BoundingBox(nullptr)
            , m_Indices()
            , m_Vertices()
        {
        }

        Mesh::Mesh(const Mesh& mesh)
            : m_VertexBuffer(mesh.m_VertexBuffer)
            , m_IndexBuffer(mesh.m_IndexBuffer)
            , m_BoundingBox(mesh.m_BoundingBox)
            , m_Name(mesh.m_Name)
            , m_Material(mesh.m_Material)
            , m_Indices(mesh.m_Indices)
            , m_Vertices(mesh.m_Vertices)
        {
        }

        Mesh::Mesh(const TDArray<uint32_t>& indices, const TDArray<Vertex>& vertices, bool optimise, float optimiseThreshold)
        {
            // int lod = 2;
            // float threshold = powf(0.7f, float(lod));
            m_Indices  = indices;
            m_Vertices = vertices;

            if(optimise)
            {
                size_t indexCount         = indices.Size();
                size_t target_index_count = size_t(indices.Size() * optimiseThreshold);

                float target_error = 1e-3f;
                float* resultError = nullptr;

                auto newIndexCount = meshopt_simplify(m_Indices.Data(), m_Indices.Data(), m_Indices.Size(), (const float*)(&m_Vertices[0]), m_Vertices.Size(), sizeof(Graphics::Vertex), target_index_count, target_error, resultError);

                auto newVertexCount = meshopt_optimizeVertexFetch( // return vertices (not vertex attribute values)
                    (m_Vertices.Data()),
                    (unsigned int*)(m_Indices.Data()),
                    newIndexCount, // total new indices (not faces)
                    (m_Vertices.Data()),
                    (size_t)m_Vertices.Size(), // total vertices (not vertex attribute values)
                    sizeof(Graphics::Vertex)   // vertex stride
                );

                m_Vertices.Resize(newVertexCount);
                m_Indices.Resize(newIndexCount);

                // LINFO("Mesh Optimizer - Before : {0} indices {1} vertices , After : {2} indices , {3} vertices", indexCount, m_Vertices.Size(), newIndexCount, newVertexCount);
            }

            m_BoundingBox = CreateSharedPtr<Maths::BoundingBox>();

            for(auto& vertex : m_Vertices)
            {
                m_BoundingBox->Merge(vertex.Position);
            }

            m_IndexBuffer  = SharedPtr<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create(m_Indices.Data(), (uint32_t)m_Indices.Size()));
            m_VertexBuffer = SharedPtr<VertexBuffer>(VertexBuffer::Create((uint32_t)(sizeof(Graphics::Vertex) * m_Vertices.Size()), m_Vertices.Data(), BufferUsage::STATIC));

#ifndef LUMOS_PRODUCTION
            m_Stats.VertexCount       = (uint32_t)m_Vertices.Size();
            m_Stats.TriangleCount     = m_Stats.VertexCount / 3;
            m_Stats.IndexCount        = (uint32_t)m_Indices.Size();
            m_Stats.OptimiseThreshold = optimiseThreshold;
#endif

            const bool storeData = true;
            if(!storeData)
            {
                m_Indices.Clear();
                m_Indices.Destroy();
                m_Vertices.Clear();
                m_Vertices.Destroy();
            }
        }

        Mesh::Mesh(const TDArray<uint32_t>& indices, const TDArray<AnimVertex>& vertices)
        {
            // int lod = 2;
            // float threshold = powf(0.7f, float(lod));
            m_Indices = indices;
            // m_Vertices = vertices;

            m_BoundingBox = CreateSharedPtr<Maths::BoundingBox>();

            for(auto& vertex : vertices)
            {
                m_BoundingBox->Merge(vertex.Position);
            }

            m_IndexBuffer      = SharedPtr<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create(m_Indices.Data(), (uint32_t)m_Indices.Size()));
            m_AnimVertexBuffer = SharedPtr<VertexBuffer>(VertexBuffer::Create((uint32_t)(sizeof(Graphics::AnimVertex) * vertices.Size()), vertices.Data(), BufferUsage::STATIC));

#ifndef LUMOS_PRODUCTION
            m_Stats.VertexCount       = (uint32_t)vertices.Size();
            m_Stats.TriangleCount     = m_Stats.VertexCount / 3;
            m_Stats.IndexCount        = (uint32_t)m_Indices.Size();
            m_Stats.OptimiseThreshold = 1.0f;
#endif

            const bool storeData = false;
            if(!storeData)
            {
                m_Indices.Clear();
                m_Indices.Destroy();
                m_Vertices.Clear();
                m_Vertices.Destroy();
            }
        }

        Mesh::~Mesh()
        {
        }

        void Mesh::GenerateNormals(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount)
        {
            Vec3* normals = new Vec3[vertexCount];

            for(uint32_t i = 0; i < vertexCount; ++i)
            {
                normals[i] = Vec3();
            }

            if(indices)
            {
                for(uint32_t i = 0; i < indexCount; i += 3)
                {
                    const int a = indices[i];
                    const int b = indices[i + 1];
                    const int c = indices[i + 2];

                    const Vec3 _normal = Maths::Cross((vertices[b].Position - vertices[a].Position), (vertices[c].Position - vertices[a].Position));

                    normals[a] += _normal;
                    normals[b] += _normal;
                    normals[c] += _normal;
                }
            }
            else
            {
                // It's just a list of triangles, so generate face normals
                for(uint32_t i = 0; i < vertexCount - 3; i += 3)
                {
                    Vec3& a = vertices[i].Position;
                    Vec3& b = vertices[i + 1].Position;
                    Vec3& c = vertices[i + 2].Position;

                    const Vec3 _normal = Maths::Cross(b - a, c - a);

                    normals[i]     = _normal;
                    normals[i + 1] = _normal;
                    normals[i + 2] = _normal;
                }
            }

            for(uint32_t i = 0; i < vertexCount; ++i)
            {
                vertices[i].Normal = (normals[i]).Normalised();
            }

            delete[] normals;
        }

#define CHECK_VEC3(Vec3) Maths::IsInf(Vec3.x) || Maths::IsInf(Vec3.y) || Maths::IsInf(Vec3.z) || Maths::IsNaN(Vec3.x) || Maths::IsNaN(Vec3.y) || Maths::IsNaN(Vec3.z)

        void Mesh::GenerateTangentsAndBitangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t numIndices)
        {
            for(uint32_t i = 0; i < vertexCount; i++)
            {
                vertices[i].Tangent   = Vec3(0.0f);
                vertices[i].Bitangent = Vec3(0.0f);
            }

            for(uint32_t i = 0; i < numIndices; i += 3)
            {
                Vec3 v0 = vertices[indices[i]].Position;
                Vec3 v1 = vertices[indices[i + 1]].Position;
                Vec3 v2 = vertices[indices[i + 2]].Position;

                Vec2 uv0 = vertices[indices[i]].TexCoords;
                Vec2 uv1 = vertices[indices[i + 1]].TexCoords;
                Vec2 uv2 = vertices[indices[i + 2]].TexCoords;

                Vec3 n0 = vertices[indices[i]].Normal;
                Vec3 n1 = vertices[indices[i + 1]].Normal;
                Vec3 n2 = vertices[indices[i + 2]].Normal;

                Vec3 edge1 = v1 - v0;
                Vec3 edge2 = v2 - v0;

                Vec2 deltaUV1 = uv1 - uv0;
                Vec2 deltaUV2 = uv2 - uv0;

                float den = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                if(den < Maths::M_EPSILON)
                    den = 1.0f;

                float f = 1.0f / den;

                Vec3 tangent   = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
                Vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

                // Store tangent and bitangent for each vertex of the triangle
                vertices[indices[i]].Tangent += tangent;
                vertices[indices[i + 1]].Tangent += tangent;
                vertices[indices[i + 2]].Tangent += tangent;

                vertices[indices[i]].Bitangent += bitangent;
                vertices[indices[i + 1]].Bitangent += bitangent;
                vertices[indices[i + 2]].Bitangent += bitangent;
            }

            // Normalize the tangent and bitangent vectors
            for(uint32_t i = 0; i < vertexCount; i++)
            {
                if(Maths::Length2(vertices[i].Tangent) > Maths::M_EPSILON)
                    vertices[i].Tangent = vertices[i].Tangent.Normalised();
                else
                    vertices[i].Tangent = Vec3(0.0f);

                if(Maths::Length2(vertices[i].Tangent) > Maths::M_EPSILON)
                    vertices[i].Bitangent = vertices[i].Bitangent.Normalised();
                else
                    vertices[i].Bitangent = Vec3(0.0f);

                ASSERT(!CHECK_VEC3(vertices[i].Tangent));
                ASSERT(!CHECK_VEC3(vertices[i].Bitangent));
            }
        }

        Vec3 Mesh::GenerateTangent(const Vec3& a, const Vec3& b, const Vec3& c, const Vec2& ta, const Vec2& tb, const Vec2& tc)
        {
            const Vec2 coord1 = tb - ta;
            const Vec2 coord2 = tc - ta;

            const Vec3 vertex1 = b - a;
            const Vec3 vertex2 = c - a;

            const Vec3 axis = Vec3(vertex1 * coord2.y - vertex2 * coord1.y);

            const float factor = 1.0f / (coord1.x * coord2.y - coord2.x * coord1.y);

            return axis * factor;
        }

        Vec3* Mesh::GenerateNormals(uint32_t numVertices, Vec3* vertices, uint32_t* indices, uint32_t numIndices)
        {
            Vec3* normals = new Vec3[numVertices];

            for(uint32_t i = 0; i < numVertices; ++i)
            {
                normals[i] = Vec3();
            }

            if(indices)
            {
                for(uint32_t i = 0; i < numIndices; i += 3)
                {
                    const int a = indices[i];
                    const int b = indices[i + 1];
                    const int c = indices[i + 2];

                    const Vec3 _normal = Maths::Cross((vertices[b] - vertices[a]), (vertices[c] - vertices[a]));

                    normals[a] += _normal;
                    normals[b] += _normal;
                    normals[c] += _normal;
                }
            }
            else
            {
                // It's just a list of triangles, so generate face normals
                for(uint32_t i = 0; i < numVertices - 3; i += 3)
                {
                    Vec3& a = vertices[i];
                    Vec3& b = vertices[i + 1];
                    Vec3& c = vertices[i + 2];

                    const Vec3 _normal = Maths::Cross(b - a, c - a);

                    normals[i]     = _normal;
                    normals[i + 1] = _normal;
                    normals[i + 2] = _normal;
                }
            }

            for(uint32_t i = 0; i < numVertices; ++i)
            {
                normals[i].Normalise();
            }

            return normals;
        }

        Vec3* Mesh::GenerateTangents(uint32_t numVertices, Vec3* vertices, uint32_t* indices, uint32_t numIndices, Vec2* texCoords)
        {
            if(!texCoords)
            {
                return nullptr;
            }

            Vec3* tangents = new Vec3[numVertices];

            for(uint32_t i = 0; i < numVertices; ++i)
            {
                tangents[i] = Vec3();
            }

            if(indices)
            {
                for(uint32_t i = 0; i < numIndices; i += 3)
                {
                    int a = indices[i];
                    int b = indices[i + 1];
                    int c = indices[i + 2];

                    const Vec3 tangent = GenerateTangent(vertices[a], vertices[b], vertices[c], texCoords[a], texCoords[b], texCoords[c]);

                    tangents[a] += tangent;
                    tangents[b] += tangent;
                    tangents[c] += tangent;
                }
            }
            else
            {
                for(uint32_t i = 0; i < numVertices; i += 3)
                {
                    const Vec3 tangent = GenerateTangent(vertices[i], vertices[i + 1], vertices[i + 2], texCoords[i], texCoords[i + 1],
                                                         texCoords[i + 2]);

                    tangents[i] += tangent;
                    tangents[i + 1] += tangent;
                    tangents[i + 2] += tangent;
                }
            }
            for(uint32_t i = 0; i < numVertices; ++i)
            {
                tangents[i].Normalise();
            }

            return tangents;
        }

        void Mesh::CalculateTriangles()
        {
            for(size_t i = 0; i < m_Indices.Size(); i += 3)
            {
                m_Triangles.EmplaceBack(m_Vertices[m_Indices[i + 0]], m_Vertices[m_Indices[i + 1]], m_Vertices[m_Indices[i + 2]]);
            }
        }

        void Mesh::SetMaterial(const SharedPtr<Material>& material)
        {
            m_Material = material;
        }

        void Mesh::SetAndLoadMaterial(const std::string& filePath)
        {
            std::string Data = FileSystem::Get().ReadTextFileVFS(filePath);
            std::istringstream istr;
            istr.str(Data);
            cereal::JSONInputArchive input(istr);
            auto material = std::make_unique<Graphics::Material>();
            Lumos::Graphics::load(input, *material.get());
            m_Material = SharedPtr<Material>(material.get());
            material.release();
        }
    }
}
