#pragma once
#include "lmpch.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/API/BufferLayout.h"
#include "Maths/Maths.h"

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
    
        struct TriangleInfo
        {
            Maths::Vector3 p1;
            Maths::Vector3 p2;
            Maths::Vector3 p3;
            Maths::Vector4 col;
        
            TriangleInfo(const Maths::Vector3& pos1,const Maths::Vector3& pos2, const Maths::Vector3& pos3, const Maths::Vector4& colour)
            {
                p1 = pos1;
                p2 = pos2;
                p3 = pos3;
                col = colour;
            }
        };

		class LUMOS_EXPORT Renderer2D
		{
		public:
			Renderer2D(u32 width, u32 height, bool renderToGBuffer = false, bool clear = true, bool triangleIndicies = false);
			virtual ~Renderer2D();

			virtual void Init(bool triangleIndicies = false);
			virtual void Submit(Renderable2D* renderable, const Maths::Matrix4& transform);
            virtual void SubmitTriangle(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector3& p3, const Maths::Vector4& colour);
			virtual void BeginSimple();
			virtual void BeginRenderPass();
			virtual void Begin();
			virtual void BeginScene(Scene* scene);
			virtual void Present();
			virtual void End();
			virtual void Render(Scene* scene);
			virtual void OnResize(u32 width, u32 height);
			virtual void PresentToScreen();
			virtual void SetScreenBufferSize(u32 width, u32 height);
			virtual void SetRenderTarget(Texture* texture);
			virtual void SetRenderToGBufferTexture(bool set);

			void SetSystemUniforms(Shader* shader) const;
			float SubmitTexture(Texture* texture);

			struct UniformBufferObject
			{
				Maths::Matrix4 projView;
			};

			u8* m_VSSystemUniformBuffer{};
			u32 m_VSSystemUniformBufferSize{};

			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void UpdateDesciptorSet() const;
        
            void FlushAndReset();

			Shader* GetShader() const { return m_Shader; }

		private:
        
            void SubmitInternal(const TriangleInfo& triangle);
        
			std::vector<Renderable2D*> m_Sprites;
			u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

			RenderPass* m_RenderPass{};
			Pipeline* m_Pipeline{};
			DescriptorSet* m_DescriptorSet{};
			UniformBuffer* m_UniformBuffer{};
			std::vector<CommandBuffer*> m_CommandBuffers;
			std::vector<CommandBuffer*> m_SecondaryCommandBuffers;

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
            Maths::Vector3 m_QuadPositions[4];
            bool m_Clear = false;
            Maths::Frustum m_Frustum;
        
            std::vector<TriangleInfo> m_Triangles;
		};
	}
}
