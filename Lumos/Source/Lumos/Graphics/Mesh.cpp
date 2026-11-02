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
        static inline int16_t PackSnorm16(float v)
        {
            v = v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
            return (int16_t)lroundf(v * 32767.0f);
        }

        static inline uint8_t PackUnorm8(float v)
        {
            v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            return (uint8_t)lroundf(v * 255.0f);
        }

        void PackVertices(const Vertex* in, uint32_t count, PackedVertex* out)
        {
            for(uint32_t i = 0; i < count; i++)
            {
                const Vertex& v = in[i];
                PackedVertex& o = out[i];
                o.Position[0] = v.Position.x;
                o.Position[1] = v.Position.y;
                o.Position[2] = v.Position.z;

                Vec3 n           = v.Normal;
                const float nLen = Maths::Length(n);
                n                = nLen > 1e-6f ? n / nLen : Vec3(0.0f, 1.0f, 0.0f);
                Vec3 t           = v.Tangent - n * Maths::Dot(v.Tangent, n);
                float tLen       = Maths::Length(t);
                if(tLen < 1e-6f) // no tangent: any perpendicular keeps the shader's normalize finite
                {
                    t    = Maths::Cross(n, fabsf(n.y) < 0.99f ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f));
                    tLen = Maths::Length(t);
                }
                t                 = t / tLen;
                const float sign  = Maths::Dot(Maths::Cross(n, t), v.Bitangent) < 0.0f ? -1.0f : 1.0f;

                o.Normal[0]  = PackSnorm16(n.x);
                o.Normal[1]  = PackSnorm16(n.y);
                o.Normal[2]  = PackSnorm16(n.z);
                o.Normal[3]  = 0;
                o.Tangent[0] = PackSnorm16(t.x);
                o.Tangent[1] = PackSnorm16(t.y);
                o.Tangent[2] = PackSnorm16(t.z);
                o.Tangent[3] = PackSnorm16(sign);
                o.TexCoords[0] = v.TexCoords.x;
                o.TexCoords[1] = v.TexCoords.y;
                o.Colour[0]    = PackUnorm8(v.Colours.x);
                o.Colour[1]    = PackUnorm8(v.Colours.y);
                o.Colour[2]    = PackUnorm8(v.Colours.z);
                o.Colour[3]    = PackUnorm8(v.Colours.w);
            }
        }

        Vertex UnpackVertex(const PackedVertex& p)
        {
            Vertex v;
            v.Position  = Vec3(p.Position[0], p.Position[1], p.Position[2]);
            v.Normal    = Vec3(p.Normal[0], p.Normal[1], p.Normal[2]) / 32767.0f;
            v.Tangent   = Vec3(p.Tangent[0], p.Tangent[1], p.Tangent[2]) / 32767.0f;
            v.Bitangent = Maths::Cross(v.Normal, v.Tangent) * (p.Tangent[3] < 0 ? -1.0f : 1.0f);
            v.TexCoords = Vec2(p.TexCoords[0], p.TexCoords[1]);
            v.Colours   = Vec4(p.Colour[0], p.Colour[1], p.Colour[2], p.Colour[3]) / 255.0f;
            return v;
        }

        VertexBuffer* CreatePackedVertexBuffer(const Vertex* vertices, uint32_t count)
        {
            TDArray<PackedVertex> packed;
            packed.Resize(count);
            PackVertices(vertices, count, packed.Data());
            return VertexBuffer::Create((uint32_t)(sizeof(PackedVertex) * count), packed.Data(), BufferUsage::STATIC);
        }

        Mesh::Mesh()
            : m_VertexBuffer(nullptr)
            , m_IndexBuffer(nullptr)
            , m_BoundingBox()
        {
        }

        Mesh::Mesh(const Mesh& mesh)
            : m_VertexBuffer(mesh.m_VertexBuffer)
            , m_AnimVertexBuffer(mesh.m_AnimVertexBuffer)
            , m_IndexBuffer(mesh.m_IndexBuffer)
            , m_Material(mesh.m_Material)
            , m_BoundingBox(mesh.m_BoundingBox)
            , m_CPUPositions(mesh.m_CPUPositions)
            , m_CPUIndices(mesh.m_CPUIndices)
            , m_CPUVertices(mesh.m_CPUVertices)
            , m_Name(mesh.m_Name)
        {
#ifndef LUMOS_PRODUCTION
            m_Stats = mesh.m_Stats;
#endif
        }

        Mesh::Mesh(const TDArray<uint32_t>& indices, const TDArray<Vertex>& vertices)
        {
            m_BoundingBox = {};

            m_CPUPositions.Reserve((uint32_t)vertices.Size());
            for(auto& vertex : vertices)
            {
                m_BoundingBox.Merge(vertex.Position);
                m_CPUPositions.PushBack(vertex.Position);
            }
            m_CPUIndices = indices;
            if(s_RetainCPUVertices)
                m_CPUVertices = vertices;

            m_IndexBuffer  = SharedPtr<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create((uint32_t*)indices.Data(), (uint32_t)indices.Size()));
            m_VertexBuffer = SharedPtr<VertexBuffer>(CreatePackedVertexBuffer(vertices.Data(), (uint32_t)vertices.Size()));

#ifndef LUMOS_PRODUCTION
            m_Stats.VertexCount   = (uint32_t)vertices.Size();
            m_Stats.TriangleCount = (uint32_t)indices.Size() / 3;
            m_Stats.IndexCount    = (uint32_t)indices.Size();
#endif
        }

        Mesh::Mesh(const TDArray<uint32_t>& indices, const TDArray<AnimVertex>& vertices)
        {
            m_BoundingBox = {};

            for(auto& vertex : vertices)
            {
                m_BoundingBox.Merge(vertex.Position);
            }

            m_IndexBuffer      = SharedPtr<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create((u32*)indices.Data(), (uint32_t)indices.Size()));
            m_AnimVertexBuffer = SharedPtr<VertexBuffer>(VertexBuffer::Create((uint32_t)(sizeof(Graphics::AnimVertex) * vertices.Size()), vertices.Data(), BufferUsage::STATIC));

#ifndef LUMOS_PRODUCTION
            m_Stats.VertexCount   = (uint32_t)vertices.Size();
            m_Stats.TriangleCount = (uint32_t)indices.Size() / 3;
            m_Stats.IndexCount    = (uint32_t)indices.Size();
#endif
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
                if(Maths::Abs(den) < Maths::M_EPSILON)
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

                if(Maths::Length2(vertices[i].Bitangent) > Maths::M_EPSILON)
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

        void Mesh::SetMaterial(const SharedPtr<Material>& material)
        {
            m_Material = material;
        }

        void Mesh::SetAndLoadMaterial(const std::string& filePath)
        {
            ArenaTemp Scratch = ScratchBegin(0, 0);

            String8 Data = FileSystem::Get().ReadTextFileVFS(Scratch.arena, Str8StdS(filePath));
            if(Data.size == 0 || !Data.str)
            {
                LERROR("Mesh::SetAndLoadMaterial: failed to read %s", filePath.c_str());
                ScratchEnd(Scratch);
                return;
            }

            try
            {
                std::istringstream istr;
                istr.str(std::string((const char*)Data.str, Data.size));
                cereal::JSONInputArchive input(istr);
                auto material = std::make_unique<Graphics::Material>();
                Lumos::Graphics::load(input, *material.get());
                m_Material = SharedPtr<Material>(material.get());
                material.release();
            }
            catch(...)
            {
                LERROR("Mesh::SetAndLoadMaterial: failed to parse %s", filePath.c_str());
            }

            ScratchEnd(Scratch);
        }
    }
}
