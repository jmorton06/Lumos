#pragma once
#include "Renderer3D.h"

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
			DeferredOffScreenRenderer(uint width, uint height);
			~DeferredOffScreenRenderer() override;

			void RenderScene(RenderList* renderList, Scene* scene) override;

			void Init() override;
			void Begin() override;
			void BeginScene(Scene* scene) override;
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
			void EndScene() override;
			void End() override;
			void Present() override;
			void OnResize(uint width, uint height) override;
			void PresentToScreen();

			void CreatePipeline();
			void CreateBuffer();
			void CreateFBO();

			int GetCommandBufferCount() const { return static_cast<int>(m_CommandBuffers.size()); }
			CommandBuffer* GetCommandBuffer(int id) const { return m_CommandBuffers[id]; }

		private:

			void SetSystemUniforms(Shader* shader) const;

			byte* m_VSSystemUniformBuffer;
			uint m_VSSystemUniformBufferSize;
			byte* m_PSSystemUniformBuffer;
			uint m_PSSystemUniformBufferSize;

			std::vector<uint> m_VSSystemUniformBufferOffsets;
			std::vector<uint> m_PSSystemUniformBufferOffsets;

			Maths::Vector4 m_ClearColour;

			Material* m_DefaultMaterial;

			UniformBuffer* m_UniformBuffer;
			UniformBuffer* m_ModelUniformBuffer;

			std::vector<CommandBuffer*> m_CommandBuffers;

			CommandBuffer* m_DeferredCommandBuffers;

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
