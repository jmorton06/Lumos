#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"
#include "Renderer3D.h"

#include <entt/entt.hpp>
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

		class LUMOS_EXPORT ShadowRenderer : public Renderer3D
		{
		public:
			ShadowRenderer(TextureDepthArray* texture = nullptr, u32 shadowMapSize = 2048, u32 numMaps = 4);
			~ShadowRenderer();

			ShadowRenderer(ShadowRenderer const&) = delete;
			ShadowRenderer& operator=(ShadowRenderer const&) = delete;

			void Init() override;
			void BeginScene(Scene* scene) override;
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

			Maths::Vector4* GetSplitDepths() { return m_SplitDepth; }
			Maths::Matrix4* GetShadowProjView() { return m_ShadowProjView; }

			_FORCE_INLINE_ u32 GetShadowMapSize() const { return m_ShadowMapSize; }
			_FORCE_INLINE_ u32 GetShadowMapNum()  const { return m_ShadowMapNum; }
			_FORCE_INLINE_ void SetShadowInvalid() { m_ShadowMapsInvalidated = true; }

			_FORCE_INLINE_ TextureDepthArray* GetTexture() const { return m_ShadowTex; }

			u8* m_VSSystemUniformBuffer;
			u32 m_VSSystemUniformBufferSize;

			std::vector<u32> m_VSSystemUniformBufferOffsets;

			struct UniformBufferObject
			{
				Lumos::Maths::Matrix4 projView[SHADOWMAP_MAX];
			};

			struct UniformBufferModel
			{
				Lumos::Maths::Matrix4* model;
			};

			void CreateGraphicsPipeline(Graphics::RenderPass* renderPass);
			void CreateFramebuffers();
			void CreateUniformBuffer();
			void UpdateCascades(Scene* scene);

			void SetLightEntity(entt::entity entity) { m_LightEntity = entity; }

			void OnImGui() override;

		protected:

			void SetSystemUniforms(Shader* shader);

			TextureDepthArray* m_ShadowTex;
			u32		        m_ShadowMapNum;
			u32		        m_ShadowMapSize;
			bool		    m_ShadowMapsInvalidated;
			Framebuffer*    m_ShadowFramebuffer[SHADOWMAP_MAX];
			Maths::Matrix4	m_ShadowProjView[SHADOWMAP_MAX];
			Maths::Vector4  m_SplitDepth[SHADOWMAP_MAX];
			Graphics::PushConstant* m_PushConstant = nullptr;
			bool			m_DeleteTexture = false;

			Lumos::Graphics::UniformBuffer* m_UniformBuffer;
			Lumos::Graphics::UniformBuffer* m_ModelUniformBuffer;
			Lumos::Graphics::CommandBuffer* m_CommandBuffer;

			entt::entity m_LightEntity;

			u32 m_Layer = 0;

			size_t dynamicAlignment;
			UniformBufferModel uboDataDynamic;
		};
	}
}
