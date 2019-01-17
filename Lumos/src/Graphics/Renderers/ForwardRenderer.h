#pragma once
#include "LM.h"
#include "Renderer3D.h"
#include "SkyboxRenderer.h"

namespace Lumos
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
		void BeginScene(Scene* scene) override;
		void Submit(const RenderCommand& command) override;
		void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override;
		void EndScene() override;
		void End() override;
		void Present() override;
		void OnResize(uint width, uint height) override;

		void CreateGraphicsPipeline();

		struct UniformBufferObject
		{
			Lumos::maths::Matrix4 proj;
			Lumos::maths::Matrix4 view;
		};

		struct UniformBufferModel
		{
			Lumos::maths::Matrix4* model;
		};

		void SetCubeMap(Texture* cubeMap);
	private:

		void SetSystemUniforms(Shader* shader) const;

		maths::Vector4 m_ClearColour;

		graphics::api::DescriptorSet* m_DefaultDescriptorSet;
		Lumos::graphics::api::Pipeline* m_GraphicsPipeline;

		Lumos::Texture2D* m_DefaultTexture;
		Lumos::TextureDepth* m_DepthTexture;

		Lumos::graphics::api::UniformBuffer* m_UniformBuffer;
		Lumos::graphics::api::UniformBuffer* m_ModelUniformBuffer;

		std::vector<Lumos::graphics::api::CommandBuffer*> commandBuffers;

		size_t dynamicAlignment;
		UniformBufferModel uboDataDynamic;

		SkyboxRenderer* m_SkyboxRenderer;

	};
}
