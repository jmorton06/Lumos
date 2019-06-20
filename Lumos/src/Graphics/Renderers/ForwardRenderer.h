#pragma once
#include "LM.h"
#include "Renderer3D.h"
#include "SkyboxRenderer.h"

namespace Lumos
{
	class LightSetup;

	namespace Graphics
	{
		class DescriptorSet;
		class TextureDepth;

		class LUMOS_EXPORT ForwardRenderer : public Renderer3D
		{
		public:
			ForwardRenderer(uint width, uint height);
			~ForwardRenderer() override;
			void RenderScene(RenderList* renderList, Scene* scene) override;

			byte* m_VSSystemUniformBuffer{};
			uint m_VSSystemUniformBufferSize{};
			byte* m_PSSystemUniformBuffer{};
			uint m_PSSystemUniformBufferSize{};

			std::vector<uint> m_VSSystemUniformBufferOffsets;
			std::vector<uint> m_PSSystemUniformBufferOffsets;

			void Init() override;
			void InitScene(Scene* scene);
			void Begin() override;
			void BeginScene(Scene* scene) override;
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
			void EndScene() override;
			void End() override;
			void Present() override;
			void OnResize(uint width, uint height) override;

			void CreateGraphicsPipeline();
			void CreateFramebuffers();

			struct UniformBufferObject
			{
				Lumos::Maths::Matrix4 proj;
				Lumos::Maths::Matrix4 view;
			};

			struct UniformBufferModel
			{
				Lumos::Maths::Matrix4* model;
			};

			void SetRenderTarget(Texture* texture) override;
			void SetRenderToGBufferTexture(bool set) override;
		private:

			void SetSystemUniforms(Shader* shader) const;

			Maths::Vector4 m_ClearColour;

			DescriptorSet* m_DefaultDescriptorSet;
			Pipeline* m_GraphicsPipeline;

			Texture2D* m_DefaultTexture;

			UniformBuffer* m_UniformBuffer;
			UniformBuffer* m_ModelUniformBuffer;

			std::vector<Lumos::Graphics::CommandBuffer*> commandBuffers;
			std::vector<Framebuffer*> m_Framebuffers;

			size_t dynamicAlignment;
			UniformBufferModel uboDataDynamic;

			uint m_CurrentBufferID = 0;

		};
	}
}
