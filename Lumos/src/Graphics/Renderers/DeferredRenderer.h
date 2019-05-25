#pragma once
#include "Renderer3D.h"

namespace lumos
{
	class LightSetup;

	namespace graphics
	{
		class Pipeline;
		class DescriptorSet;
		class GBuffer;
		class Texture2D;
		class TextureDepth;
		class TextureDepthArray;
		class SkyboxRenderer;
		class Shader;
		class ShadowRenderer;
		class Framebuffer;

		class LUMOS_EXPORT DeferredRenderer : public Renderer3D
		{
		public:
			DeferredRenderer(uint width, uint height);
			~DeferredRenderer() override;

			void RenderScene(RenderList* renderList, Scene* scene) override;

			byte* m_VSSystemUniformBuffer;
			uint m_VSSystemUniformBufferSize;
			byte* m_PSSystemUniformBuffer;
			uint m_PSSystemUniformBufferSize;

			std::vector<uint> m_VSSystemUniformBufferOffsets;
			std::vector<uint> m_PSSystemUniformBufferOffsets;

			void Init() override;
			void Begin() override { };
			void Begin(int commandBufferID);
			void BeginScene(Scene* scene) override;
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override;
			void SubmitLightSetup(Scene* scene);
			void EndScene() override;
			void End() override;
			void Present() override;
			void OnResize(uint width, uint height) override;
			void PresentToScreen();

			void BeginOffscreen();
			void PresentOffScreen();
			void EndOffScreen();

			void LightPass();


			void CreateOffScreenPipeline();
			void CreateDeferredPipeline();
			void CreateOffScreenBuffer();
			void CreateOffScreenFBO();
			void CreateLightBuffer();
			void CreateFramebuffers();
			void CreateDefaultDescriptorSet();
			void CreateScreenDescriptorSet();
			void SetCubeMap(Texture* cubeMap);

			int GetCommandBufferCount() const { return static_cast<int>(m_CommandBuffers.size()); }
			CommandBuffer* GetCommandBuffer(int id) const { return m_CommandBuffers[id]; }

			void SetRenderTarget(Texture* texture) override;
			void SetRenderToGBufferTexture(bool set) override;

		private:

			void SetSystemUniforms(Shader* shader) const;

			maths::Vector4 m_ClearColour;

			DescriptorSet* m_DefaultDescriptorSet;
			DescriptorSet* m_DeferredDescriptorSet;

			Pipeline* m_OffScreenPipeline;
			Pipeline* m_DeferredPipeline;

			Texture2D* m_DefaultTexture;

			Shader* m_OffScreenShader;
			Shader* m_DeferredShader;

			RenderPass* m_DeferredRenderpass;
			RenderPass* m_OffScreenRenderpass;

			UniformBuffer* m_UniformBuffer;
			UniformBuffer* m_ModelUniformBuffer;
			UniformBuffer* m_LightUniformBuffer;
			UniformBuffer* m_DefaultMaterialDataUniformBuffer;

			std::vector<Framebuffer*> m_Framebuffers;
			std::vector<CommandBuffer*> m_CommandBuffers;

			CommandBuffer* m_DeferredCommandBuffers;

			LightSetup* m_LightSetup;


			size_t dynamicAlignment;

			struct UniformBufferModel
			{
				maths::Matrix4* model;
			};

			UniformBufferModel uboDataDynamic;

			Mesh* m_ScreenQuad = nullptr;

			std::unique_ptr<Texture2D> m_PreintegratedFG;

			int m_CommandBufferIndex = 0;

			Texture* m_CubeMap = nullptr;
		};
	}
}
