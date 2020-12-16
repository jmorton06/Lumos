#pragma once

#include "Maths/Maths.h"
#include "IRenderer.h"

#include <entt/entity/fwd.hpp>
#define SHADOWMAP_MAX 16

namespace Lumos
{
	class RenderList;
	class Scene;
	class Camera;
	class Mesh;

	namespace Graphics
	{
		struct Light;
		struct RenderCommand;
		class Shader;
		class TextureDepthArray;
		class Framebuffer;
		class Pipeline;
		class DescriptorSet;
		class UniformBuffer;
		class CommandBuffer;
		class RenderPass;

		typedef std::vector<RenderCommand> CommandQueue;

		class LUMOS_EXPORT ShadowRenderer : public IRenderer
		{
		public:
			ShadowRenderer(TextureDepthArray* texture = nullptr, u32 shadowMapSize = 2048, u32 numMaps = 4);
			~ShadowRenderer();

			ShadowRenderer(ShadowRenderer const&) = delete;
			ShadowRenderer& operator=(ShadowRenderer const&) = delete;

			void Init() override;
			void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
			void OnResize(u32 width, u32 height) override;

			void SetShadowMapNum(u32 num);
			void SetShadowMapSize(u32 size);

			void Begin() override;
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
			void EndScene() override;
			void End() override;
			void Present() override;
			void RenderScene(Scene* scene) override;
            void PresentToScreen() override {}

			Maths::Vector4* GetSplitDepths()
			{
				return m_SplitDepth;
			}
			Maths::Matrix4* GetShadowProjView()
			{
				return m_ShadowProjView;
			}

			_FORCE_INLINE_ u32 GetShadowMapSize() const
			{
				return m_ShadowMapSize;
			}
			_FORCE_INLINE_ u32 GetShadowMapNum() const
			{
				return m_ShadowMapNum;
			}
			_FORCE_INLINE_ void SetShadowInvalid()
			{
				m_ShadowMapsInvalidated = true;
			}

			_FORCE_INLINE_ TextureDepthArray* GetTexture() const
			{
				return m_ShadowTex;
			}

			u8* m_VSSystemUniformBuffer{};
			u32 m_VSSystemUniformBufferSize{};

			std::vector<u32> m_VSSystemUniformBufferOffsets;

			struct UniformBufferObject
			{
				Lumos::Maths::Matrix4 projView[SHADOWMAP_MAX];
			};

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void CreateUniformBuffer();
			void UpdateCascades(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform, Light* light);

            const Lumos::Maths::Matrix4& GetLightView() const { return m_LightMatrix; }
            
			void OnImGui() override;
            
            float GetLightSize() { return m_LightSize; }
            float GetMaxShadowDistance() { return m_MaxShadowDistance; }
            float GetShadowFade() { return m_ShadowFade; }
            float GetCascadeTransitionFade() { return m_CascadeTransitionFade; }
            float GetInitialBias() { return m_InitialBias; }

		protected:
			void SetSystemUniforms(Shader* shader);

			TextureDepthArray* m_ShadowTex;
			u32 m_ShadowMapNum;
			u32 m_ShadowMapSize;
			bool m_ShadowMapsInvalidated;
			Ref<Framebuffer> m_ShadowFramebuffer[SHADOWMAP_MAX]{};
			Maths::Matrix4 m_ShadowProjView[SHADOWMAP_MAX];
			Maths::Vector4 m_SplitDepth[SHADOWMAP_MAX];
            Maths::Matrix4 m_LightMatrix;

			Lumos::Graphics::UniformBuffer* m_UniformBuffer;
			Lumos::Graphics::CommandBuffer* m_CommandBuffer = nullptr;
			
			u32 m_Layer = 0;
            float m_CascadeSplitLambda;
            float m_SceneRadiusMultiplier;
            bool m_ShouldRender = false;
            
            float m_LightSize;
            float m_MaxShadowDistance;
            float m_ShadowFade;
            float m_CascadeTransitionFade;
            float m_InitialBias;
            
            std::vector<Graphics::PushConstant> m_PushConstants;
		};
	}
}
