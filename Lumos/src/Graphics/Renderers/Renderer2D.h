#pragma once

#include "IRenderer.h"
#include "Graphics/Renderable2D.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

#define MAX_BOUND_TEXTURES 16

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
		class VertexBuffer;

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

		class LUMOS_EXPORT Renderer2D : public IRenderer
		{
		public:
			Renderer2D(u32 width, u32 height, bool clear = true, bool triangleIndicies = false, bool renderToDepthTexture = true);
			virtual ~Renderer2D();

			virtual void Init() override;
			virtual void Begin() override;
			virtual void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
			virtual void Present() override;
			virtual void EndScene() override {};
			virtual void End() override;
			virtual void OnResize(u32 width, u32 height) override;
			virtual void SetScreenBufferSize(u32 width, u32 height) override;
			virtual void SetRenderTarget(Texture* texture, bool rebuildFrameBuffer = true) override;
			virtual void RenderScene(Scene* scene) override;

			virtual void SubmitTriangle(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector3& p3, const Maths::Vector4& colour);
			virtual void Submit(Renderable2D* renderable, const Maths::Matrix4& transform);
			virtual void BeginSimple();
			virtual void PresentToScreen() override;
			virtual void BeginRenderPass();

			float SubmitTexture(Texture* texture);

			void SetSystemUniforms(Shader* shader) const;

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void UpdateDesciptorSet();

			void FlushAndReset();
			void SubmitTriangles();

		private:
			void SubmitInternal(const TriangleInfo& triangle);

			std::vector<Renderable2D*> m_Sprites;
			std::vector<CommandBuffer*> m_SecondaryCommandBuffers;
			std::vector<VertexBuffer*> m_VertexBuffers;

			u32 m_BatchDrawCallIndex = 0;
			u32 m_IndexCount = 0;

			Render2DLimits m_Limits;

			UniformBuffer* m_UniformBuffer = nullptr;
			IndexBuffer* m_IndexBuffer = nullptr;
			VertexData* m_Buffer = nullptr;

			std::vector<Maths::Matrix4> m_TransformationStack;
			const Maths::Matrix4* m_TransformationBack{};

			Texture* m_Textures[MAX_BOUND_TEXTURES];
            u32 m_TextureCount;

			u32 m_CurrentBufferID = 0;
			Maths::Vector3 m_QuadPositions[4];

			std::vector<TriangleInfo> m_Triangles;

			bool m_Clear = false;
			bool m_RenderToDepthTexture;
            bool m_Empty = false;
			bool m_TriangleIndicies = false;
		};
	}
}
