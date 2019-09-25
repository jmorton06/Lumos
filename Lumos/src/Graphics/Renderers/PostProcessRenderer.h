#pragma once
/*
#include "lmpch.h"
#include "Graphics/Renderable2D.h"
#include "Maths/Matrix4.h"
#include "Graphics/API/BufferLayout.h"

namespace Lumos
{
	namespace Graphics
	{
		
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
		PostProcessRenderer(u32 width, u32 height);
		virtual ~PostProcessRenderer();

		virtual void Init();
		virtual void Begin();
		virtual void Present();
		virtual void End();
		virtual void OnResize(u32 width, u32 height);
		virtual void SetScreenBufferSize(u32 width, u32 height);
		virtual void SetRenderTarget(Texture* texture);
		virtual void SetRenderToGBufferTexture(bool set);

		void SetSystemUniforms(Shader* shader) const;
		float SubmitTexture(Texture* texture);

		struct UniformBufferObject
		{
			Lumos::Maths::Matrix4 projView;
		};

		u8* m_VSSystemUniformBuffer{};
		u32 m_VSSystemUniformBufferSize{};

		virtual void CreateGraphicsPipeline();
		virtual void CreateFramebuffers();
		virtual void UpdateDesciptorSet();

	private:
		std::vector<Renderable2D*> m_Sprites;
		u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

		Graphics::RenderPass* m_RenderPass{};
		Graphics::Pipeline* m_Pipeline{};
		Graphics::DescriptorSet* m_DescriptorSet{};
		Graphics::UniformBuffer* m_UniformBuffer{};
		std::vector<Graphics::CommandBuffer*> m_CommandBuffers;
		std::vector<Graphics::CommandBuffer*> m_SecondaryCommandBuffers;

		u32 m_BatchDrawCallIndex = 0;

		std::vector<Framebuffer*> m_Framebuffers;

		Shader* m_Shader{};

		std::vector<VertexArray*> m_VertexArrays;
		IndexBuffer* m_IndexBuffer{};
		u32 m_IndexCount;

		VertexData* m_Buffer{};

		std::vector<Maths::Matrix4> m_TransformationStack;
		const Maths::Matrix4* m_TransformationBack{};

		std::vector<Texture*> m_Textures;

		Texture* m_RenderTexture;
		bool m_RenderToGBufferTexture = false;
		u32 m_CurrentBufferID = 0;
		Maths::Vector4 m_ClearColour;

	};
}
*/