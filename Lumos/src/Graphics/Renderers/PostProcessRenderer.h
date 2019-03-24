#pragma once
/*
#include "LM.h"
#include "Graphics/Renderable2D.h"
#include "Maths/Matrix4.h"
#include "Graphics/API/BufferLayout.h"

namespace Lumos
{
	namespace graphics
	{
		namespace api
		{
			class RenderPass;
			class Pipeline;
			class DescriptorSet;
			class CommandBuffer;
			class UniformBuffer;
		}
	}

	class Renderable2D;
	class Framebuffer;
	class Texture;
	class Scene;
	class Shader;
	class IndexBuffer;
	class VertexArray;

	class LUMOS_EXPORT PostProcessRenderer
	{
	public:
		PostProcessRenderer(uint width, uint height);
		virtual ~PostProcessRenderer();

		virtual void Init();
		virtual void Begin();
		virtual void Present();
		virtual void End();
		virtual void OnResize(uint width, uint height);
		virtual void SetScreenBufferSize(uint width, uint height);
		virtual void SetRenderTarget(Texture* texture);
		virtual void SetRenderToGBufferTexture(bool set);

		void SetSystemUniforms(Shader* shader) const;
		float SubmitTexture(Texture* texture);

		struct UniformBufferObject
		{
			Lumos::maths::Matrix4 projView;
		};

		byte* m_VSSystemUniformBuffer{};
		uint m_VSSystemUniformBufferSize{};

		virtual void CreateGraphicsPipeline();
		virtual void CreateFramebuffers();
		virtual void UpdateDesciptorSet();

	private:
		std::vector<Renderable2D*> m_Sprites;
		uint m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

		graphics::api::RenderPass* m_RenderPass{};
		graphics::api::Pipeline* m_Pipeline{};
		graphics::api::DescriptorSet* m_DescriptorSet{};
		graphics::api::UniformBuffer* m_UniformBuffer{};
		std::vector<graphics::api::CommandBuffer*> m_CommandBuffers;
		std::vector<graphics::api::CommandBuffer*> m_SecondaryCommandBuffers;

		uint m_BatchDrawCallIndex = 0;

		std::vector<Framebuffer*> m_Framebuffers;

		Shader* m_Shader{};

		std::vector<VertexArray*> m_VertexArrays;
		IndexBuffer* m_IndexBuffer{};
		uint m_IndexCount;

		VertexData* m_Buffer{};

		std::vector<maths::Matrix4> m_TransformationStack;
		const maths::Matrix4* m_TransformationBack{};

		std::vector<Texture*> m_Textures;

		Texture* m_RenderTexture;
		bool m_RenderToGBufferTexture = false;
		uint m_CurrentBufferID = 0;
		maths::Vector4 m_ClearColour;

	};
}
*/