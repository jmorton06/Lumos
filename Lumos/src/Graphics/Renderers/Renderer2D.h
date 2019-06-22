#pragma once
#include "LM.h"
#include "Graphics/Renderable2D.h"
#include "Maths/Matrix4.h"
#include "Graphics/API/BufferLayout.h"

namespace Lumos
{
	class Scene;
	
	namespace Graphics
	{
		class RenderPass;
		class Pipeline;
		class DescriptorSet;
		class CommandBuffer;
		class UniformBuffer;
		class Renderable2D;
		class Framebuffer;
		class Texture;
		class Shader;
		class IndexBuffer;
		class VertexArray;

		class LUMOS_EXPORT Renderer2D
		{
		public:
			Renderer2D(uint width, uint height, bool renderToGBuffer = false);
			virtual ~Renderer2D();

			virtual void Init();
			virtual void Submit(Renderable2D* renderable, const Maths::Matrix4& transform);
			virtual void Begin();
			virtual void BeginScene(Scene* scene);
			virtual void Present();
			virtual void End();
			virtual void Render(Scene* scene);
			virtual void OnResize(uint width, uint height);
			virtual void PresentToScreen();
			virtual void SetScreenBufferSize(uint width, uint height);
			virtual void SetRenderTarget(Texture* texture);
			virtual void SetRenderToGBufferTexture(bool set);

			void SetSystemUniforms(Shader* shader) const;
			float SubmitTexture(Texture* texture);

			struct UniformBufferObject
			{
				Maths::Matrix4 projView;
			};

			byte* m_VSSystemUniformBuffer{};
			uint m_VSSystemUniformBufferSize{};

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void UpdateDesciptorSet();

		private:
			std::vector<Renderable2D*> m_Sprites;
			uint m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

			RenderPass* m_RenderPass{};
			Pipeline* m_Pipeline{};
			DescriptorSet* m_DescriptorSet{};
			UniformBuffer* m_UniformBuffer{};
			std::vector<CommandBuffer*> m_CommandBuffers;
			std::vector<CommandBuffer*> m_SecondaryCommandBuffers;

			uint m_BatchDrawCallIndex = 0;

			std::vector<Framebuffer*> m_Framebuffers;

			Shader* m_Shader{};

			std::vector<VertexArray*> m_VertexArrays;
			IndexBuffer* m_IndexBuffer{};
			uint m_IndexCount;

			VertexData* m_Buffer{};

			std::vector<Maths::Matrix4> m_TransformationStack;
			const Maths::Matrix4* m_TransformationBack{};

			std::vector<Texture*> m_Textures;

			Texture* m_RenderTexture;
			bool m_RenderToGBufferTexture = false;
			uint m_CurrentBufferID = 0;
			Maths::Vector4 m_ClearColour;

		};
	}
}
