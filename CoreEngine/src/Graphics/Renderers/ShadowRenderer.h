#pragma once
#include "JM.h"
#include "Maths/Maths.h"
#include "Renderer3D.h"

namespace jm
{

	class TextureDepthArray;
	class Framebuffer;
	class RenderList;
	class Scene;
	class Shader;
	class Camera;
	struct RenderCommand;
	class Mesh;

	typedef std::vector<RenderCommand> CommandQueue;

	namespace graphics
	{
		namespace api
		{
			class Pipeline;
			class DescriptorSet;
			class UniformBuffer;
			class CommandBuffer;
			class RenderPass;
		}
	}

#define SHADOWMAP_MAX 16

	class ShadowRenderer : public Renderer3D
	{
	public:
		ShadowRenderer(TextureDepthArray* texture = nullptr, uint shadowMapSize = 2048, uint numMaps = 4);
		~ShadowRenderer();

		void Init() override;
		void BeginScene(Camera* camera) override;
		void OnResize(uint width, uint height) override;

		void SetShadowMapNum(uint num);
		void SetShadowMapSize(uint size);

		void SortRenderLists(Scene* scene);

		void Begin() override;
		void Submit(const RenderCommand& command) override;
		void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) override;
		void EndScene() override;
		void End() override;
		void Present() override;
		void RenderScene(RenderList* renderList, Scene* scene) override;


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
			jm::maths::Matrix4 projView;
		};

		struct UniformBufferModel
		{
			jm::maths::Matrix4* model;
		};

		void CreateGraphicsPipeline(graphics::api::RenderPass* renderPass);
		void CreateFramebuffers();
		void CreateUniformBuffer();

	protected:

		void SetSystemUniforms(Shader* shader) const;

		TextureDepthArray* m_ShadowTex;
		uint		    m_ShadowMapNum;
		uint		    m_ShadowMapSize;
		bool		    m_ShadowMapsInvalidated;
		Framebuffer*    m_ShadowFramebuffer[SHADOWMAP_MAX];
		maths::Matrix4	m_ShadowProjView[SHADOWMAP_MAX];
		RenderList**	m_apShadowRenderLists;
		bool			m_DeleteTexture = false;

		jm::graphics::api::UniformBuffer* m_UniformBuffer;
		jm::graphics::api::UniformBuffer* m_ModelUniformBuffer;
		jm::graphics::api::CommandBuffer* m_CommandBuffer;

		uint m_Layer = 0;

		size_t dynamicAlignment;
		UniformBufferModel uboDataDynamic;
	};
}