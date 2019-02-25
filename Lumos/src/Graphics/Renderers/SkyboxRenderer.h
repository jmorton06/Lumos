#pragma once
#include "LM.h"
#include "Renderer3D.h"

namespace Lumos
{
	class LightSetup;
	class TextureDepth;
    class Texture;
	class Shader;

	class LUMOS_EXPORT SkyboxRenderer : public Renderer3D
	{
	public:
		SkyboxRenderer(uint width, uint height, Texture* cubeMap);
		~SkyboxRenderer();
		//void Render(graphics::api::CommandBuffer* commandBuffer, Scene* scene, int framebufferId);

		void Init() override;
		void BeginScene(Scene* scene) override;
		void OnResize(uint width, uint height) override;
		void CreateGraphicsPipeline();
		void SetCubeMap(Texture* cubeMap);
		void UpdateUniformBuffer();
		//void SetRenderInfo(graphics::api::RenderPass* renderPass) { m_RenderPass = renderPass; }

		void Begin() override;
		void Submit(const RenderCommand& command) override {};
		void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override {};
		void EndScene() override {};
		void End() override;
		void Present() override {};
        void RenderScene(RenderList* renderList, Scene* scene) override;

		void CreateFramebuffers();

		struct UniformBufferObject
		{
			Lumos::maths::Matrix4 invprojview;
		};

		void SetRenderTarget(Texture* texture) override;

	private:

		void SetSystemUniforms(Shader* shader) const;

		byte* m_VSSystemUniformBuffer{};
		uint m_VSSystemUniformBufferSize{};
		byte* m_PSSystemUniformBuffer{};
		uint m_PSSystemUniformBufferSize{};

		std::vector<uint> m_VSSystemUniformBufferOffsets;
		std::vector<uint> m_PSSystemUniformBufferOffsets;

		Lumos::graphics::api::UniformBuffer* m_UniformBuffer;
		std::vector<Lumos::graphics::api::CommandBuffer*> m_CommandBuffers;
		std::vector<Framebuffer*> m_Framebuffers;
        
        uint m_CurrentBufferID = 0;

		Mesh* m_Skybox;
		Texture* m_CubeMap;
	};
}
