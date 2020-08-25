#pragma once
#include "lmpch.h"
#include "IRenderer.h"
#include "Maths/Maths.h"
#include "Graphics/API/DescriptorSet.h"
#include "Maths/Transform.h"

#define RENDERER2DPOINT_VERTEX_SIZE sizeof(PointVertexData)

namespace Lumos
{
	class Scene;
	class Camera;

	namespace Graphics
	{
		struct PointInfo
		{
			Maths::Vector3 p1;
			Maths::Vector4 col;
			float size;

			PointInfo(const Maths::Vector3& pos1, float s, const Maths::Vector4& colour)
			{
				p1 = pos1;
				size = s;
				col = colour;
			}
		};

		struct LUMOS_EXPORT PointVertexData
		{
			Maths::Vector3 vertex;
			Maths::Vector4 color;
			Maths::Vector2 size;
			Maths::Vector2 uv;

			bool operator==(const PointVertexData& other) const
			{
				return vertex == other.vertex && color == other.color && size == other.size && uv == other.uv;
			}

			static std::array<Graphics::VertexInputDescription, 4> getAttributeDescriptions()
			{
				std::array<Graphics::VertexInputDescription, 4> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = Graphics::Format::R32G32B32_FLOAT;
				attributeDescriptions[0].offset = offsetof(PointVertexData, vertex);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = Graphics::Format::R32G32B32A32_FLOAT;
				attributeDescriptions[1].offset = offsetof(PointVertexData, color);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = Graphics::Format::R32G32_FLOAT;
				attributeDescriptions[2].offset = offsetof(PointVertexData, size);

				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = Graphics::Format::R32G32_FLOAT;
				attributeDescriptions[3].offset = offsetof(PointVertexData, uv);

				return attributeDescriptions;
			}
		};

		class RenderPass;
		class Pipeline;
		class DescriptorSet;
		class CommandBuffer;
		class UniformBuffer;
		class Renderable2D;
		class Framebuffer;
		class Texture;
		class Shader;
		class IndexBuffer;
		class VertexArray;

		class LUMOS_EXPORT PointRenderer : public IRenderer
		{
		public:
			PointRenderer(u32 width, u32 height, bool clear);
			~PointRenderer();

			void Init() override;
			void Begin() override;
			void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
			void Present() override;
			void End() override;
			void EndScene() override {};
			void OnResize(u32 width, u32 height) override;
			void SetScreenBufferSize(u32 width, u32 height) override;
			void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer) override;
			void RenderScene(Scene* scene) override {};

			void RenderInternal(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) ;
			void PresentToScreen() override;
			void Submit(const Maths::Vector3& p1, float size, const Maths::Vector4& colour);
			void SetSystemUniforms(Graphics::Shader* shader) const;
			float SubmitTexture(Graphics::Texture* texture);

			struct UniformBufferObject
			{
				Maths::Matrix4 projView;
			};

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void UpdateDesciptorSet() const;
	
		protected:
			void FlushAndResetPoints();
			void SubmitInternal(PointInfo& pointInfo);

			PointVertexData* m_Buffer = nullptr;
			Graphics::IndexBuffer* m_IndexBuffer = nullptr;
			Graphics::UniformBuffer* m_UniformBuffer = nullptr;
			std::vector<Graphics::CommandBuffer*> m_SecondaryCommandBuffers;
			std::vector<Graphics::VertexArray*> m_VertexArrays;
			std::vector<PointInfo> m_Points;

			u32 m_BatchDrawCallIndex = 0;
			u32 PointIndexCount = 0;
			u32 m_IndexCount = 0;
			u32 m_CurrentBufferID = 0;
			bool m_Clear = false;
		};
	}
}
