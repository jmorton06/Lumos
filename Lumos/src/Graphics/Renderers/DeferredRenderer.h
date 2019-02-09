#pragma once
#include "Renderer3D.h"

namespace Lumos
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
		void SubmitLightSetup(const LightSetup& lightSetup, Scene* scene);
		void EndScene() override;
		void End() override;
		void Present() override;
		void OnResize(uint width,uint height) override;
		void PresentToScreen();

		void BeginOffscreen();
		void PresentOffScreen();
		void EndOffScreen();

		void LightPass();

		struct UniformBufferObject
		{
			Lumos::maths::Matrix4 projView;
		};

		struct UniformBufferModel
		{
			Lumos::maths::Matrix4* model;
		};

		struct UniformBufferLight
		{
			Lumos::maths::Vector4 position;
			Lumos::maths::Vector4 direction;
			Lumos::maths::Vector4 cameraPosition;
			Lumos::maths::Matrix4 viewMatrix;
			Lumos::maths::Matrix4 uShadowTransform[16];
    		Lumos::maths::Vector4 uSplitDepth[16];
		};

		void CreateOffScreenPipeline();
		void CreateDeferredPipeline();
		void CreateOffScreenBuffer();
		void CreateOffScreenFBO();
		void CreateLightBuffer();
		void CreateFramebuffers();
		void CreateDefaultDescriptorSet();
		void CreateScreenDescriptorSet();
		void SetCubeMap(Texture* cubeMap);

		int GetCommandBufferCount() const { return (int)m_CommandBuffers.size(); }
		Lumos::graphics::api::CommandBuffer* GetCommandBuffer(int id) const { return m_CommandBuffers[id]; }

		GBuffer* GetGBuffer() const { return m_GBuffer.get(); }

	private:

		void SetSystemUniforms(Shader* shader) const;

		maths::Vector4 m_ClearColour;

		graphics::api::DescriptorSet* m_DefaultDescriptorSet;
		graphics::api::DescriptorSet* m_DeferredDescriptorSet;

		Lumos::graphics::api::Pipeline* m_OffScreenPipeline;
		Lumos::graphics::api::Pipeline* m_DeferredPipeline;

		Lumos::Texture2D* m_DefaultTexture;

		Lumos::Shader* m_OffScreenShader;
		Lumos::Shader* m_DeferredShader;

		Lumos::graphics::api::RenderPass* m_DeferredRenderpass;
		Lumos::graphics::api::RenderPass* m_OffScreenRenderpass;

		Lumos::graphics::api::UniformBuffer* m_UniformBuffer;
		Lumos::graphics::api::UniformBuffer* m_ModelUniformBuffer;
		Lumos::graphics::api::UniformBuffer* m_LightUniformBuffer;
		Lumos::graphics::api::UniformBuffer* m_DefaultMaterialDataUniformBuffer;

		std::vector<Framebuffer*> m_Framebuffers;
		std::vector<Lumos::graphics::api::CommandBuffer*> m_CommandBuffers;

		Lumos::graphics::api::CommandBuffer* m_DeferredCommandBuffers;

		size_t dynamicAlignment;
		UniformBufferModel uboDataDynamic;

		Lumos::Mesh* m_ScreenQuad = nullptr;

		std::unique_ptr<Texture2D> m_PreintegratedFG;

		int m_CommandBufferIndex = 0;

		std::unique_ptr<GBuffer> m_GBuffer;
		std::unique_ptr<ShadowRenderer> m_ShadowRenderer;
		std::unique_ptr<TextureDepthArray> m_ShadowTexture;
		SkyboxRenderer* m_SkyboxRenderer;
		Texture* m_CubeMap = nullptr;
	};

}