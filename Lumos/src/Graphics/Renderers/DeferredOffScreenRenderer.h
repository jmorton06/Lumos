#pragma once
#include "Renderer3D.h"
#include "Maths/Frustum.h"

namespace Lumos
{
	class LightSetup;
	class Material;

	namespace Graphics
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

		class LUMOS_EXPORT DeferredOffScreenRenderer : public Renderer3D
		{
		public:
			DeferredOffScreenRenderer(u32 width, u32 height);
			~DeferredOffScreenRenderer() override;

			void RenderScene(Scene* scene) override;

			void Init() override;
			void Begin() override;
			void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransforms) override;
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
			void EndScene() override;
			void End() override;
			void Present() override;
			void OnResize(u32 width, u32 height) override;
			void PresentToScreen();

			void CreatePipeline();
			void CreateBuffer();
			void CreateFBO();

			int GetCommandBufferCount() const
			{
				return static_cast<int>(m_CommandBuffers.size());
			}
			CommandBuffer* GetCommandBuffer(int id) const
			{
				return m_CommandBuffers[id];
			}

			void OnImGui() override;

		private:
			void SetSystemUniforms(Shader* shader);

			u8* m_VSSystemUniformBuffer;
			u32 m_VSSystemUniformBufferSize;
			u8* m_PSSystemUniformBuffer;
			u32 m_PSSystemUniformBufferSize;

			std::vector<u32> m_VSSystemUniformBufferOffsets;
			std::vector<u32> m_PSSystemUniformBufferOffsets;

			Maths::Vector4 m_ClearColour;

			Material* m_DefaultMaterial;

			UniformBuffer* m_UniformBuffer;
			UniformBuffer* m_ModelUniformBuffer;

			std::vector<CommandBuffer*> m_CommandBuffers;

			CommandBuffer* m_DeferredCommandBuffers;

			Maths::Frustum m_Frustum;

			struct UniformBufferModel
			{
				Maths::Matrix4* model;
			};

			UniformBufferModel m_UBODataDynamic;

			size_t m_DynamicAlignment;
			int m_CommandBufferIndex = 0;
		};
	}
}
