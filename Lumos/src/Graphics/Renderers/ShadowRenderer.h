#pragma once
#include "LM.h"
#include "Maths/Maths.h"
#include "Renderer3D.h"

#define SHADOWMAP_MAX 16

namespace lumos
{
	class RenderList;
	class Scene;
	class Camera;
	class Mesh;

	namespace graphics
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
			ShadowRenderer(TextureDepthArray* texture = nullptr, uint shadowMapSize = 2048, uint numMaps = 4);
			~ShadowRenderer();

			ShadowRenderer(ShadowRenderer const&) = delete;
			ShadowRenderer& operator=(ShadowRenderer const&) = delete;

			void Init() override;
			void BeginScene(Scene* scene) override;
			void OnResize(uint width, uint height) override;

			void SetShadowMapNum(uint num);
			void SetShadowMapSize(uint size);

			void Begin() override;
			void Submit(const RenderCommand& command) override;
			void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override;
			void EndScene() override;
			void End() override;
			void Present() override;
			void RenderScene(RenderList* renderList, Scene* scene) override;

			maths::Vector4* GetSplitDepths() { return m_SplitDepth; }
			maths::Matrix4* GetShadowProjView() { return m_ShadowProjView; }

			inline uint GetShadowMapSize() const { return m_ShadowMapSize; }
			inline uint GetShadowMapNum()  const { return m_ShadowMapNum; }
			inline void SetShadowInvalid() { m_ShadowMapsInvalidated = true; }

			inline TextureDepthArray* GetTexture() const { return m_ShadowTex; }
			void ClearRenderLists();

			byte* m_VSSystemUniformBuffer;
			uint m_VSSystemUniformBufferSize;

			std::vector<uint> m_VSSystemUniformBufferOffsets;

			struct UniformBufferObject
			{
				lumos::maths::Matrix4 projView[SHADOWMAP_MAX];
			};

			struct UniformBufferModel
			{
				lumos::maths::Matrix4* model;
			};

			void CreateGraphicsPipeline(graphics::RenderPass* renderPass);
			void CreateFramebuffers();
			void CreateUniformBuffer();
			void UpdateCascades(Scene* scene);

			void SetLight(std::shared_ptr<graphics::Light>& light) { m_Light = light; }

		protected:

			void SetSystemUniforms(Shader* shader);

			TextureDepthArray* m_ShadowTex;
			uint		    m_ShadowMapNum;
			uint		    m_ShadowMapSize;
			bool		    m_ShadowMapsInvalidated;
			Framebuffer*    m_ShadowFramebuffer[SHADOWMAP_MAX];
			maths::Matrix4	m_ShadowProjView[SHADOWMAP_MAX];
			maths::Vector4  m_SplitDepth[SHADOWMAP_MAX];
			RenderList**	m_apShadowRenderLists;
			graphics::PushConstant* m_PushConstant = nullptr;
			bool			m_DeleteTexture = false;

			lumos::graphics::UniformBuffer* m_UniformBuffer;
			lumos::graphics::UniformBuffer* m_ModelUniformBuffer;
			lumos::graphics::CommandBuffer* m_CommandBuffer;

			std::shared_ptr<graphics::Light> m_Light;

			uint m_Layer = 0;

			size_t dynamicAlignment;
			UniformBufferModel uboDataDynamic;
		};
	}
}
