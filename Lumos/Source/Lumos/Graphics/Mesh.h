#pragma once

#include "RHI/IndexBuffer.h"
#include "RHI/VertexBuffer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Maths/Maths.h"
#include "Maths/BoundingBox.h"
#include "Material.h"

#include <glm/gtx/hash.hpp>
#include <array>

namespace Lumos
{
    namespace Graphics
    {
        class Texture2D;

        struct LUMOS_EXPORT BasicVertex
        {
            glm::vec3 Position;
            glm::vec2 TexCoords;
        };

        struct LUMOS_EXPORT Vertex
        {
            Vertex()
                : Position(glm::vec3(0.0f))
                , Colours(glm::vec4(0.0f))
                , TexCoords(glm::vec2(0.0f))
                , Normal(glm::vec3(0.0f))
                , Tangent(glm::vec3(0.0f))
                , Bitangent(glm::vec3(0.0f))
            {
            }

            glm::vec3 Position;
            glm::vec4 Colours;
            glm::vec2 TexCoords;
            glm::vec3 Normal;
            glm::vec3 Tangent;
            glm::vec3 Bitangent;

            bool operator==(const Vertex& other) const
            {
                return Position == other.Position && TexCoords == other.TexCoords && Colours == other.Colours && Normal == other.Normal && Tangent == other.Tangent && Bitangent == other.Bitangent;
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
            Mesh(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, float optimiseThreshold = 0.95f);
            virtual ~Mesh();

            const SharedPtr<VertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
            const SharedPtr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
            const SharedPtr<Material>& GetMaterial() const { return m_Material; }
            const SharedPtr<Maths::BoundingBox>& GetBoundingBox() const { return m_BoundingBox; }

            void SetMaterial(const SharedPtr<Material>& material) { m_Material = material; }

            bool& GetActive() { return m_Active; }
            void SetName(const std::string& name) { m_Name = name; }
            const std::string& GetName() const { return m_Name; }

            static void GenerateNormals(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
            static void GenerateTangentsAndBitangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);

            void CalculateTriangles();

            const std::vector<Triangle>& GetTriangles()
            {
                if(m_Triangles.empty())
                    CalculateTriangles();

                return m_Triangles;
            }

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
            static glm::vec3 GenerateTangent(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec2& ta, const glm::vec2& tb, const glm::vec2& tc);

            static glm::vec3* GenerateNormals(uint32_t numVertices, glm::vec3* vertices, uint32_t* indices, uint32_t numIndices);
            static glm::vec3* GenerateTangents(uint32_t numVertices, glm::vec3* vertices, uint32_t* indices, uint32_t numIndices, glm::vec2* texCoords);

            SharedPtr<VertexBuffer> m_VertexBuffer;
            SharedPtr<IndexBuffer> m_IndexBuffer;
            SharedPtr<Material> m_Material;
            SharedPtr<Maths::BoundingBox> m_BoundingBox;

            std::string m_Name;

            bool m_Active = true;
            std::vector<uint32_t> m_Indices;
            std::vector<Vertex> m_Vertices;

            // Only calculated on request
            std::vector<Triangle> m_Triangles;

#ifndef LUMOS_PRODUCTION
            MeshStats m_Stats;
#endif
        };
    }
}

namespace std
{
    template <>
    struct hash<Lumos::Graphics::Vertex>
    {
        size_t operator()(Lumos::Graphics::Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec2>()(vertex.TexCoords) << 1) ^ (hash<glm::vec4>()(vertex.Colours) << 1) ^ (hash<glm::vec3>()(vertex.Normal) << 1) ^ (hash<glm::vec3>()(vertex.Tangent) << 1)));
        }
    };
}
