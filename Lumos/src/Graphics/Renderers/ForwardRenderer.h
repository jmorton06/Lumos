#pragma once
#include "JM.h"
#include "Renderer3D.h"
#include "SkyboxRenderer.h"

namespace jm
{
	class LightSetup;
	class DescriptorSet;
	class TextureDepth;

	class ForwardRenderer : public Renderer3D
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
		void BeginScene(Camera* camera) override;
		void Submit(const RenderCommand& command) override;
		void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override;
		void EndScene() override;
		void End() override;
		void Present() override;
		void OnResize(uint width, uint height) override;

		void CreateGraphicsPipeline();

		struct UniformBufferObject
		{
			jm::maths::Matrix4 proj;
			jm::maths::Matrix4 view;
		};

		struct UniformBufferModel
		{
			jm::maths::Matrix4* model;
		};

		void SetCubeMap(Texture* cubeMap);
	private:

		void SetSystemUniforms(Shader* shader) const;

		maths::Vector4 m_ClearColour;

		graphics::api::DescriptorSet* m_DefaultDescriptorSet;
		jm::graphics::api::Pipeline* m_GraphicsPipeline;

		jm::Texture2D* m_DefaultTexture;
		jm::TextureDepth* m_DepthTexture;

		jm::graphics::api::UniformBuffer* m_UniformBuffer;
		jm::graphics::api::UniformBuffer* m_ModelUniformBuffer;

		std::vector<jm::graphics::api::CommandBuffer*> commandBuffers;

		size_t dynamicAlignment;
		UniformBufferModel uboDataDynamic;

		SkyboxRenderer* m_SkyboxRenderer;

	};
}
