#pragma once
#include "lmpch.h"
#include "Renderer3D.h"
#include "Maths/Frustum.h"

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
			ForwardRenderer(u32 width, u32 height, bool renderToGBuffer = false, bool depthTest = true);
			~ForwardRenderer() override;
			void RenderScene(Scene* scene) override;

			u8* m_VSSystemUniformBuffer{};
			u32 m_VSSystemUniformBufferSize{};
			u8* m_PSSystemUniformBuffer{};
			u32 m_PSSystemUniformBufferSize{};

			std::vector<u32> m_VSSystemUniformBufferOffsets;
			std::vector<u32> m_PSSystemUniformBufferOffsets;

			void Init() override;
			void Begin() override;
			void BeginScene(Scene* scene) override;
        
            void BeginScene(const Maths::Matrix4& proj, const Maths::Matrix4& view);
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
			void EndScene() override;
			void End() override;
			void Present() override;
			void OnResize(u32 width, u32 height) override;

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
            void SetSystemUniforms(Shader* shader) const;
        
            Shader* GetShader() { return m_Shader; }

		private:


			Maths::Vector4 m_ClearColour;

			Texture2D* m_DefaultTexture;

			UniformBuffer* m_UniformBuffer;
			UniformBuffer* m_ModelUniformBuffer;

			std::vector<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;
			std::vector<Framebuffer*> m_Framebuffers;

			size_t m_DynamicAlignment;
			UniformBufferModel m_UBODataDynamic;

			Maths::Frustum m_Frustum;

			u32 m_CurrentBufferID = 0;
            bool m_DepthTest = false;

		};
	}
}
