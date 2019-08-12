#pragma once
#include "LM.h"

#include "API/IndexBuffer.h"
#include "API/VertexArray.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/DescriptorSet.h"

#include <array>

namespace Lumos
{
	namespace Maths
	{
		class BoundingSphere;
	}
	
	namespace Graphics
	{
		class Texture2D;

		struct LUMOS_EXPORT BasicVertex
		{
			Maths::Vector3 Position;
			Maths::Vector3 color;
			Maths::Vector2 TexCoords;
		};

		struct LUMOS_EXPORT Vertex
		{
			Vertex()
				: Position(Maths::Vector3(0.0f))
				, Colours(Maths::Vector4(0.0f))
				, TexCoords(Maths::Vector2(0.0f))
				, Normal(Maths::Vector3(0.0f))
				, Tangent(Maths::Vector3(0.0f))
			{

			}

			Maths::Vector3 Position;
			Maths::Vector4 Colours;
			Maths::Vector2 TexCoords;
			Maths::Vector3 Normal;
			Maths::Vector3 Tangent;

			bool operator==(const Vertex& other) const
			{
				return Position == other.Position  && TexCoords == other.TexCoords && Colours == other.Colours && Normal == other.Normal && Tangent == other.Tangent;
			}

			static std::array<Graphics::VertexInputDescription, 5> getAttributeDescriptions()
			{
				std::array<Graphics::VertexInputDescription, 5> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = Graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[0].offset = offsetof(Vertex, Position);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = Graphics::Format::R32G32B32A32_FLOAT;
				attributeDescriptions[1].offset = offsetof(Vertex, Colours);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = Graphics::Format::R32G32_FLOAT;
				attributeDescriptions[2].offset = offsetof(Vertex, TexCoords);

				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = Graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[3].offset = offsetof(Vertex, Normal);

				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 4;
				attributeDescriptions[4].format = Graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[4].offset = offsetof(Vertex, Tangent);

				return attributeDescriptions;
			}
		};

		class LUMOS_EXPORT Mesh
		{
		public:

			Mesh();
			Mesh(const Mesh& mesh);
			Mesh(Ref<VertexArray>& vertexArray, Ref<IndexBuffer>& indexBuffer, const Ref<Maths::BoundingSphere>& boundingSphere);

			virtual ~Mesh();
			virtual void Draw();

			void Init();
			Ref<VertexArray> GetVertexArray() const { return m_VertexArray; }
			Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
			Ref<Maths::BoundingSphere> GetBoundingSphere() const { return m_BoundingSphere; }

			std::vector<Graphics::CommandBuffer*> m_CMDBuffers;

			Graphics::CommandBuffer* GetCommandBuffer(int id) const { return m_CMDBuffers[id]; }

		protected:

			static Maths::Vector3 GenerateTangent(const Maths::Vector3 &a, const Maths::Vector3 &b, const Maths::Vector3 &c, const Maths::Vector2 &ta, const Maths::Vector2 &tb, const Maths::Vector2 &tc);

			static Maths::Vector3* GenerateNormals(u32 numVertices, Maths::Vector3* vertices, u32* indices, u32 numIndices);
			static Maths::Vector3* GenerateTangents(u32 numVertices, Maths::Vector3* vertices, u32* indices, u32 numIndices, Maths::Vector2* texCoords);

			Ref<VertexArray> m_VertexArray;
			Ref<IndexBuffer> m_IndexBuffer;

			Ref<Maths::BoundingSphere> m_BoundingSphere;

			bool m_ArrayCleanUp;
			bool m_TextureCleanUp;
		};
	}
}

namespace std
{
	template<> struct hash<Lumos::Graphics::Vertex>
	{
		size_t operator()(Lumos::Graphics::Vertex const& vertex) const
		{
			return ((hash<Lumos::Maths::Vector3>()(vertex.Position) ^
				(hash<Lumos::Maths::Vector2>()(vertex.TexCoords) << 1) ^
				(hash<Lumos::Maths::Vector4>()(vertex.Colours) << 1) ^
				(hash<Lumos::Maths::Vector3>()(vertex.Normal) << 1) ^
				(hash<Lumos::Maths::Vector3>()(vertex.Tangent) << 1)));
		}

	};
}

