#pragma once
#include "Renderer3D.h"

namespace jm
{
	class Texture2D;
	class TextureDepth;
	class TextureDepthArray;
	class LightSetup;
	class Framebuffer;
	class SkyboxRenderer;
	class Shader;
	class ShadowRenderer;
	class GBuffer;

	namespace graphics
	{
		namespace api
		{
			class Pipeline;
			class DescriptorSet;
		}
	}

	class DeferredRenderer : public Renderer3D
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
		void InitScene(Scene* scene, bool newScene = true);
		void Begin() override { };
		void Begin(int commandBufferID);
		void BeginScene(Camera* camera) override;
		void Submit(const RenderCommand& command) override;
		void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override;
		void SubmitLightSetup(const LightSetup& lightSetup, Scene* scene);
		void EndScene() override;
		void End() override;
		void Present() override;
		void OnResize(uint width,uint height) override;
		void PresentToScreen();

		void BeginOffscreen();
		void PresentOffScreen();
		void EndOffScreen();

		struct UniformBufferObject
		{
			jm::maths::Matrix4 projView;
		};

		struct UniformBufferModel
		{
			jm::maths::Matrix4* model;
		};

		struct UniformBufferLight
		{
			jm::maths::Vector4 position;
			jm::maths::Vector4 direction;
			jm::maths::Vector4 cameraPosition;
			jm::maths::Matrix4 uShadowTransform[16];
    		jm::maths::Vector2 uShadowSinglePixel;
		};

		void CreateOffScreenPipeline();
		void CreateDeferredPipeline();
		void CreateOffScreenBuffer();
		void CreateOffScreenFBO();
		void CreateLightBuffer();
		void SetCubeMap(Texture* cubeMap);

		int GetCommandBufferCount() const { return (int)m_CommandBuffers.size(); }
		jm::graphics::api::CommandBuffer* GetCommandBuffer(int id) const { return m_CommandBuffers[id]; }

		GBuffer* GetGBuffer() const { return m_GBuffer.get(); }

	private:

		void SetSystemUniforms(Shader* shader) const;

		maths::Vector4 m_ClearColour;

		graphics::api::DescriptorSet* m_DefaultDescriptorSet;
		graphics::api::DescriptorSet* m_DeferredDescriptorSet;

		jm::graphics::api::Pipeline* m_OffScreenPipeline;
		jm::graphics::api::Pipeline* m_DeferredPipeline;

		jm::Texture2D* m_DefaultTexture;
		jm::TextureDepth* m_DepthTexture;

		jm::Shader* m_OffScreenShader;
		jm::Shader* m_DeferredShader;

		jm::graphics::api::RenderPass* m_DeferredRenderpass;
		jm::graphics::api::RenderPass* m_OffScreenRenderpass;

		jm::graphics::api::UniformBuffer* m_UniformBuffer;
		jm::graphics::api::UniformBuffer* m_ModelUniformBuffer;
		jm::graphics::api::UniformBuffer* m_LightUniformBuffer;
		jm::graphics::api::UniformBuffer* m_DefaultMaterialDataUniformBuffer;

		std::unique_ptr<GBuffer> m_GBuffer;

		std::vector<jm::graphics::api::CommandBuffer*> m_CommandBuffers;

		jm::graphics::api::CommandBuffer* m_DeferredCommandBuffers;

		size_t dynamicAlignment;
		UniformBufferModel uboDataDynamic;

		jm::Mesh* m_ScreenQuad = nullptr;

		SkyboxRenderer* m_SkyboxRenderer;
		ShadowRenderer* m_ShadowRenderer;

		std::unique_ptr<Texture2D> m_PreintegratedFG;

		int m_CommandBufferIndex = 0;

		std::unique_ptr<TextureDepthArray> m_ShadowTexture;
	};

}