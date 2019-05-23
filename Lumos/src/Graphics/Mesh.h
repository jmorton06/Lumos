#pragma once
#include "LM.h"

#include "API/IndexBuffer.h"
#include "API/VertexArray.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/DescriptorSet.h"

#include <array>

namespace lumos
{
	namespace maths
	{
		class BoundingSphere;
	}
		
	class Material;
	
	namespace graphics
	{
		class Texture2D;
		class Shader;

		struct LUMOS_EXPORT BasicVertex
		{
			maths::Vector3 Position;
			maths::Vector3 color;
			maths::Vector2 TexCoords;
		};

		struct LUMOS_EXPORT Vertex
		{
			Vertex()
				: Position(maths::Vector3(0.0f))
				, Colours(maths::Vector4(0.0f))
				, TexCoords(maths::Vector2(0.0f))
				, Normal(maths::Vector3(0.0f))
				, Tangent(maths::Vector3(0.0f))
			{

			}

			maths::Vector3 Position;
			maths::Vector4 Colours;
			maths::Vector2 TexCoords;
			maths::Vector3 Normal;
			maths::Vector3 Tangent;

			bool operator==(const Vertex& other) const
			{
				return Position == other.Position  && TexCoords == other.TexCoords && Colours == other.Colours && Normal == other.Normal && Tangent == other.Tangent;
			}

			static std::array<graphics::VertexInputDescription, 5> getAttributeDescriptions()
			{
				std::array<graphics::VertexInputDescription, 5> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[0].offset = offsetof(Vertex, Position);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[1].offset = offsetof(Vertex, Colours);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = graphics::Format::R32G32_FLOAT;
				attributeDescriptions[2].offset = offsetof(Vertex, TexCoords);

				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[3].offset = offsetof(Vertex, Normal);

				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 4;
				attributeDescriptions[4].format = graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[4].offset = offsetof(Vertex, Tangent);

				return attributeDescriptions;
			}
		};

		class LUMOS_EXPORT Mesh
		{
		public:

			Mesh();
			Mesh(const Mesh& mesh);
			Mesh(std::shared_ptr<VertexArray>& vertexArray, std::shared_ptr<IndexBuffer>& indexBuffer, const std::shared_ptr<Material>& material, const std::shared_ptr<maths::BoundingSphere>& boundingSphere);

			virtual ~Mesh();
			virtual void Draw();
			virtual void Draw(bool bindMaterial);

			void Init();
			void SetMaterial(const std::shared_ptr<Material>& material) { m_Material = material; }
			std::shared_ptr<Material> GetMaterial() const { return m_Material; }
			std::shared_ptr<VertexArray> GetVertexArray() const { return m_VertexArray; }
			std::shared_ptr<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
			std::shared_ptr<maths::BoundingSphere> GetBoundingSphere() const { return m_BoundingSphere; }

			std::vector<graphics::CommandBuffer*> m_CMDBuffers;

			graphics::CommandBuffer* GetCommandBuffer(int id) const { return m_CMDBuffers[id]; }

		protected:

			static maths::Vector3 GenerateTangent(const maths::Vector3 &a, const maths::Vector3 &b, const maths::Vector3 &c, const maths::Vector2 &ta, const maths::Vector2 &tb, const maths::Vector2 &tc);

			static maths::Vector3* GenerateNormals(uint numVertices, maths::Vector3* vertices, uint* indices, uint numIndices);
			static maths::Vector3* GenerateTangents(uint numVertices, maths::Vector3* vertices, uint* indices, uint numIndices, maths::Vector2* texCoords);

			std::shared_ptr<VertexArray> m_VertexArray;
			std::shared_ptr<IndexBuffer> m_IndexBuffer;
			std::shared_ptr<Material> m_Material;

			std::shared_ptr<maths::BoundingSphere> m_BoundingSphere;

			bool m_ArrayCleanUp;
			bool m_TextureCleanUp;
		};
	}
}

namespace std
{
	template<> struct hash<lumos::graphics::Vertex>
	{
		size_t operator()(lumos::graphics::Vertex const& vertex) const
		{
			return ((hash<lumos::maths::Vector3>()(vertex.Position) ^
				(hash<lumos::maths::Vector2>()(vertex.TexCoords) << 1) ^
				(hash<lumos::maths::Vector4>()(vertex.Colours) << 1) ^
				(hash<lumos::maths::Vector3>()(vertex.Normal) << 1) ^
				(hash<lumos::maths::Vector3>()(vertex.Tangent) << 1)));
		}

	};
}

