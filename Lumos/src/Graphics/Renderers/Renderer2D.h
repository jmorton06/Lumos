#pragma once
#include "lmpch.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/API/BufferLayout.h"
#include "Maths/Maths.h"

namespace Lumos
{
	class Scene;
	class Camera;

	namespace Graphics
	{
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

		struct TriangleInfo
		{
			Maths::Vector3 p1;
			Maths::Vector3 p2;
			Maths::Vector3 p3;
			Maths::Vector4 col;

			TriangleInfo(const Maths::Vector3& pos1, const Maths::Vector3& pos2, const Maths::Vector3& pos3, const Maths::Vector4& colour)
			{
				p1 = pos1;
				p2 = pos2;
				p3 = pos3;
				col = colour;
			}
		};

		struct Render2DLimits
		{
			u32 MaxQuads = 10000;
			u32 QuadsSize = RENDERER2D_VERTEX_SIZE * 4;
			u32 BufferSize = 10000 * RENDERER2D_VERTEX_SIZE * 4;
			u32 IndiciesSize = 10000 * 6;
			u32 MaxTextures = 16;
			u32 MaxBatchDrawCalls = 100;

			void SetMaxQuads(u32 quads)
			{
				MaxQuads = quads;
				BufferSize = MaxQuads * RENDERER2D_VERTEX_SIZE * 4;
				IndiciesSize = MaxQuads * 6;
			}
		};

		class LUMOS_EXPORT Renderer2D
		{
		public:
			Renderer2D(u32 width, u32 height, bool clear = true, bool triangleIndicies = false, bool renderToDepthTexture = true);
			virtual ~Renderer2D();

			virtual void Init(bool triangleIndicies = false);
			virtual void Submit(Renderable2D* renderable, const Maths::Matrix4& transform);
			virtual void SubmitTriangle(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector3& p3, const Maths::Vector4& colour);
			virtual void BeginSimple();
			virtual void BeginRenderPass();
			virtual void Begin();
			virtual void BeginScene(Scene* scene, Camera* overrideCamera);
			virtual void Present();
			virtual void End();
			virtual void Render(Scene* scene);
			virtual void OnResize(u32 width, u32 height);
			virtual void PresentToScreen();
			virtual void SetScreenBufferSize(u32 width, u32 height);
			virtual void SetRenderTarget(Texture* texture, bool rebuildFrameBuffer = true);

			void SetSystemUniforms(Shader* shader) const;
			float SubmitTexture(Texture* texture);

			u8* m_VSSystemUniformBuffer{};
			u32 m_VSSystemUniformBufferSize{};

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void UpdateDesciptorSet() const;

			void FlushAndReset();
			void SubmitTriangles();

			Shader* GetShader() const
			{
				return m_Shader;
			}
			void SetCamera(Camera* camera)
			{
				m_Camera = camera;
			}
			Texture* GetRenderTarget() const
			{
				return m_RenderTexture;
			}

		private:
			void SubmitInternal(const TriangleInfo& triangle);

			std::vector<Renderable2D*> m_Sprites;
			u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

			RenderPass* m_RenderPass{};
			Pipeline* m_Pipeline{};
			DescriptorSet* m_DescriptorSet{};
			UniformBuffer* m_UniformBuffer{};
			std::vector<CommandBuffer*> m_CommandBuffers;
			std::vector<CommandBuffer*> m_SecondaryCommandBuffers;

			u32 m_BatchDrawCallIndex = 0;

			std::vector<Framebuffer*> m_Framebuffers;

			Shader* m_Shader{};

			std::vector<VertexArray*> m_VertexArrays;
			IndexBuffer* m_IndexBuffer{};
			u32 m_IndexCount;

			VertexData* m_Buffer{};

			std::vector<Maths::Matrix4> m_TransformationStack;
			const Maths::Matrix4* m_TransformationBack{};

			std::vector<Texture*> m_Textures;

			Texture* m_RenderTexture;
			u32 m_CurrentBufferID = 0;
			Maths::Vector4 m_ClearColour;
			Maths::Vector3 m_QuadPositions[4];
			bool m_Clear = false;
			Maths::Frustum m_Frustum;

			std::vector<TriangleInfo> m_Triangles;

			bool m_RenderToDepthTexture;
			Render2DLimits m_Limits;
			Camera* m_Camera = nullptr;
            bool m_Empty = false;
		};
	}
}
