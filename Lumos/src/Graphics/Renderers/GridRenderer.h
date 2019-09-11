#pragma once
#include "lmpch.h"
#include "Renderer3D.h"

namespace Lumos
{
	namespace Graphics
	{
		class Shader;

		class LUMOS_EXPORT GridRenderer : public Renderer3D
		{
		public:
			GridRenderer(u32 width, u32 height, bool renderToGBuffer = false);
			~GridRenderer();

			void Init() override;
			void BeginScene(Scene* scene) override;
			void OnResize(u32 width, u32 height) override;
			void CreateGraphicsPipeline();
			void UpdateUniformBuffer();

			void Begin() override;
			void Submit(const RenderCommand& command) override {};
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override {};
			void EndScene() override {};
			void End() override;
			void Present() override {};
			void RenderScene(RenderList* renderList, Scene* scene) override;

			void CreateFramebuffers();

			struct UniformBufferObject
			{
				Lumos::Maths::Matrix4 mvp;
			};

			struct UniformBufferObjectFrag
			{
				float scale;
				float res;
				float p0;
				float p1;
			};

			void SetRenderTarget(Texture* texture) override;
			void SetRenderToGBufferTexture(bool set) override;

		private:
			void SetSystemUniforms(Shader* shader) const;

			u8* m_VSSystemUniformBuffer{};
			u32 m_VSSystemUniformBufferSize{};
			u8* m_PSSystemUniformBuffer{};
			u32 m_PSSystemUniformBufferSize{};

			std::vector<u32> m_VSSystemUniformBufferOffsets;
			std::vector<u32> m_PSSystemUniformBufferOffsets;

			Lumos::Graphics::UniformBuffer* m_UniformBuffer;
			Lumos::Graphics::UniformBuffer* m_UniformBufferFrag;
			std::vector<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;
			std::vector<Framebuffer*> m_Framebuffers;

			u32 m_CurrentBufferID = 0;
			Mesh* m_Quad;
		};
	}
}
