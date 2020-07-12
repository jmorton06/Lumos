#pragma once

#include "lmpch.h"
#include "Maths/Maths.h"
#include "Graphics/API/DescriptorSet.h"

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

		class LUMOS_EXPORT PointRenderer
		{
		public:
			PointRenderer(u32 width, u32 height, bool clear);
			~PointRenderer();

			void Init();
			void Submit(const Maths::Vector3& p1, float size, const Maths::Vector4& colour);
			void Begin();
			void BeginScene(Scene* scene, Camera* overrideCamera);
			void Present();
			void End();
			void RenderInternal(Scene* scene, Camera* overrideCamera);
			void OnResize(u32 width, u32 height);
			void PresentToScreen();
			void SetScreenBufferSize(u32 width, u32 height);
			void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer);

			void SetSystemUniforms(Graphics::Shader* shader) const;
			float SubmitTexture(Graphics::Texture* texture);

			struct UniformBufferObject
			{
				Maths::Matrix4 projView;
			};

			u8* m_VSSystemUniformBuffer{};
			u32 m_VSSystemUniformBufferSize{};

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void UpdateDesciptorSet() const;

			void SetCamera(Camera* camera)
			{
				m_Camera = camera;
			}

		protected:
			void FlushAndResetPoints();
			void SubmitInternal(PointInfo& pointInfo);

			u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

			Graphics::RenderPass* m_RenderPass{};
			Graphics::Pipeline* m_Pipeline{};
			Graphics::UniformBuffer* m_UniformBuffer{};
			std::vector<Graphics::CommandBuffer*> m_CommandBuffers;
			std::vector<Graphics::CommandBuffer*> m_SecondaryCommandBuffers;

			u32 m_BatchDrawCallIndex = 0;
			u32 PointIndexCount = 0;

			std::vector<Graphics::Framebuffer*> m_Framebuffers;

			Graphics::Shader* m_Shader{};

			std::vector<Graphics::VertexArray*> m_VertexArrays;
			Graphics::IndexBuffer* m_IndexBuffer{};
			u32 m_IndexCount;

			PointVertexData* m_Buffer{};

			Graphics::Texture* m_RenderTexture;
			u32 m_CurrentBufferID = 0;
			Maths::Vector4 m_ClearColour;
			bool m_Clear = false;

			Maths::Matrix4 m_ProjectionMatrix;

			std::vector<PointInfo> m_Points;
			Camera* m_Camera;
		};
	}
}
