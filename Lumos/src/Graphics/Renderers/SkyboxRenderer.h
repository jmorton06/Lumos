#pragma once

#include "IRenderer.h"

namespace Lumos
{
	namespace Graphics
	{
		class TextureDepth;
		class Texture;
		class Shader;

		class LUMOS_EXPORT SkyboxRenderer : public IRenderer
		{
		public:
			SkyboxRenderer(u32 width, u32 height);
			~SkyboxRenderer();

			void Init() override;
			void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
			void OnResize(u32 width, u32 height) override;
			void CreateGraphicsPipeline();
			void SetCubeMap(Texture* cubeMap);
			void UpdateUniformBuffer();

			void Begin() override;
			void Submit(const RenderCommand& command) override{};
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override {};
			void EndScene() override{};
			void End() override;
			void Present() override{};
			void RenderScene() override;
            void PresentToScreen() override {}

			void CreateFramebuffers();

			struct UniformBufferObject
			{
				Lumos::Maths::Matrix4 invprojview;
			};

			void SetRenderTarget(Texture* texture, bool rebuildFramebuffer) override;

			void OnImGui() override;

		private:
			void SetSystemUniforms(Shader* shader) const;

			Lumos::Graphics::UniformBuffer* m_UniformBuffer;

			u32 m_CurrentBufferID = 0;

			Mesh* m_Skybox;
			Texture* m_CubeMap;
		};
	}
}
