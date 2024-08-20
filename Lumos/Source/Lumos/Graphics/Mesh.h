#pragma once
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    namespace Maths
    {
        class BoundingBox;
    }
    namespace Graphics
    {
        class Texture2D;
        class Material;
        class VertexBuffer;
        class IndexBuffer;

        struct LUMOS_EXPORT BasicVertex
        {
            Vec3 Position;
            Vec2 TexCoords;
        };

        struct LUMOS_EXPORT Vertex
        {
            Vertex()
                : Position(Vec3(0.0f))
                , Colours(Vec4(0.0f))
                , TexCoords(Vec2(0.0f))
                , Normal(Vec3(0.0f))
                , Tangent(Vec3(0.0f))
                , Bitangent(Vec3(0.0f))
            {
            }

            Vec3 Position;
            Vec4 Colours;
            Vec2 TexCoords;
            Vec3 Normal;
            Vec3 Tangent;
            Vec3 Bitangent;

            bool operator==(const Vertex& other) const
            {
                return Position == other.Position && TexCoords == other.TexCoords && Colours == other.Colours && Normal == other.Normal && Tangent == other.Tangent && Bitangent == other.Bitangent;
            }
        };

        struct AnimVertex
        {
            AnimVertex()
                : Position(Vec3(0.0f))
                , Colours(Vec4(0.0f))
                , TexCoords(Vec2(0.0f))
                , Normal(Vec3(0.0f))
                , Tangent(Vec3(0.0f))
                , Bitangent(Vec3(0.0f))
            {
            }

            Vec3 Position;
            Vec4 Colours;
            Vec2 TexCoords;
            Vec3 Normal;
            Vec3 Tangent;
            Vec3 Bitangent;
            uint32_t BoneInfoIndices[4] = { 0, 0, 0, 0 };
            float Weights[4]            = { 0.0f, 0.0f, 0.0f, 0.0f };

            bool operator==(const AnimVertex& other) const
            {
                return Position == other.Position && TexCoords == other.TexCoords && Colours == other.Colours && Normal == other.Normal && Tangent == other.Tangent && Bitangent == other.Bitangent;
            }

            void AddBoneData(uint32_t boneInfoIndex, float weight)
            {
                if(weight < 0.0f || weight > 1.0f)
                {
                    weight = std::clamp(weight, 0.0f, 1.0f);
                }
                if(weight > 0.0f)
                {
                    for(size_t i = 0; i < 4; i++)
                    {
                        if(Weights[i] == 0.0f)
                        {
                            BoneInfoIndices[i] = boneInfoIndex;
                            Weights[i]         = weight;
                            return;
                        }
                    }
                }
            }

            void NormalizeWeights()
            {
                float sumWeights = 0.0f;
                for(size_t i = 0; i < 4; i++)
                {
                    sumWeights += Weights[i];
                }
                if(sumWeights > 0.0f)
                {
                    for(size_t i = 0; i < 4; i++)
                    {
                        Weights[i] /= sumWeights;
                    }
                }
            }
        };

        struct Triangle
        {
            Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
            {
                p0 = v0;
                p1 = v1;
                p2 = v2;
            }

            Triangle() = default;

            Vertex p0;
            Vertex p1;
            Vertex p2;
        };

        struct MeshStats
        {
            uint32_t TriangleCount;
            uint32_t VertexCount;
            uint32_t IndexCount;
            float OptimiseThreshold;
        };

        class LUMOS_EXPORT Mesh
        {
        public:
            Mesh();
            Mesh(const Mesh& mesh);
            Mesh(const TDArray<uint32_t>& indices, const TDArray<Vertex>& vertices);
            Mesh(const TDArray<uint32_t>& indices, const TDArray<AnimVertex>& vertices);
            virtual ~Mesh();

            const SharedPtr<VertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
            const SharedPtr<VertexBuffer>& GetAnimVertexBuffer() const { return m_AnimVertexBuffer; }
            const SharedPtr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
            const SharedPtr<Material>& GetMaterial() const { return m_Material; }
            const SharedPtr<Maths::BoundingBox>& GetBoundingBox() const { return m_BoundingBox; }

            void SetMaterial(const SharedPtr<Material>& material);
            void SetAndLoadMaterial(const std::string& filePath);

            void SetName(const std::string& name) { m_Name = name; }
            const std::string& GetName() const { return m_Name; }

            static void GenerateNormals(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
            static void GenerateTangentsAndBitangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);

#ifndef LUMOS_PRODUCTION
            const MeshStats& GetStats() const
            {
                return m_Stats;
            }
#else
            MeshStats GetStats()
            {
                return {};
            }
#endif

        protected:
            static Vec3 GenerateTangent(const Vec3& a, const Vec3& b, const Vec3& c, const Vec2& ta, const Vec2& tb, const Vec2& tc);

            static Vec3* GenerateNormals(uint32_t numVertices, Vec3* vertices, uint32_t* indices, uint32_t numIndices);
            static Vec3* GenerateTangents(uint32_t numVertices, Vec3* vertices, uint32_t* indices, uint32_t numIndices, Vec2* texCoords);

            SharedPtr<VertexBuffer> m_VertexBuffer;
            SharedPtr<VertexBuffer> m_AnimVertexBuffer;
            SharedPtr<IndexBuffer> m_IndexBuffer;
            SharedPtr<Material> m_Material;
            SharedPtr<Maths::BoundingBox> m_BoundingBox;

            std::string m_Name;

#ifndef LUMOS_PRODUCTION
            MeshStats m_Stats;
#endif
        };
    }
}
