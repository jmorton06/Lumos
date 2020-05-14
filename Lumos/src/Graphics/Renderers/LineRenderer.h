#pragma once

#include "lmpch.h"
#include "Maths/Maths.h"
#include "Graphics/API/DescriptorSet.h"

#define RENDERER2DLINE_VERTEX_SIZE	sizeof(LineVertexData)

namespace Lumos {

    class Scene;

    namespace Graphics {
    
    struct LUMOS_EXPORT LineVertexData
    {
        Maths::Vector3 vertex;
        Maths::Vector4 color;

        bool operator==(const LineVertexData& other) const
        {
            return vertex == other.vertex  && color == other.color;
        }

        static std::array<Graphics::VertexInputDescription, 2> getAttributeDescriptions()
        {
            std::array<Graphics::VertexInputDescription, 2> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = Graphics::Format::R32G32B32_FLOAT;
            attributeDescriptions[0].offset = offsetof(LineVertexData, vertex);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = Graphics::Format::R32G32B32A32_FLOAT;
            attributeDescriptions[1].offset = offsetof(LineVertexData, color);

            return attributeDescriptions;
        }
    };


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
    class Renderer2D;
	class Texture2D;
	class Material;
        
    struct LineInfo
    {
        Maths::Vector3 p1;
        Maths::Vector3 p2;
        Maths::Vector4 col;
    
        LineInfo(const Maths::Vector3& pos1,const Maths::Vector3& pos2,const Maths::Vector4& colour)
        {
            p1 = pos1;
            p2 = pos2;
            col = colour;
        }
    };

	class LUMOS_EXPORT LineRenderer
	{
	public:
     
        LineRenderer(u32 width, u32 height, bool renderToGBuffer, bool clear);
        ~LineRenderer();
	
        void Init();
        void Submit(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector4& colour);
        void Begin();
        void BeginScene(Scene* scene);
        void Present();
        void End();
        void RenderInternal(Scene* scene);
        void OnResize(u32 width, u32 height);
        void PresentToScreen();
        void SetScreenBufferSize(u32 width, u32 height);
        void SetRenderTarget(Graphics::Texture* texture);
        void SetRenderToGBufferTexture(bool set);

        void SetSystemUniforms(Graphics::Shader* shader) const;
        float SubmitTexture(Graphics::Texture* texture);

        struct UniformBufferObject
        {
            Maths::Matrix4 projView;
        };

        u8* m_VSSystemUniformBuffer{};
        u32 m_VSSystemUniformBufferSize{};

        void CreateGraphicsPipeline();
        void CreateFramebuffers();
    
        void FlushAndResetLines();

	protected:
    
        void SubmitInternal(const LineInfo& info);

        u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};

        Graphics::RenderPass* m_RenderPass{};
        Graphics::Pipeline* m_Pipeline{};
        Graphics::UniformBuffer* m_UniformBuffer{};
        std::vector<Graphics::CommandBuffer*> m_CommandBuffers;
        std::vector<Graphics::CommandBuffer*> m_SecondaryCommandBuffers;

        u32 m_BatchDrawCallIndex = 0;
		u32 LineIndexCount = 0;

        std::vector<Graphics::Framebuffer*> m_Framebuffers;

        Graphics::Shader* m_Shader{};

        std::vector<Graphics::VertexArray*> m_VertexArrays;
        Graphics::IndexBuffer* m_IndexBuffer{};

        LineVertexData* m_Buffer{};

        Graphics::Texture* m_RenderTexture;
        bool m_RenderToGBufferTexture = false;
        u32 m_CurrentBufferID = 0;
        Maths::Vector4 m_ClearColour;
        bool m_Clear = false;
    
        std::vector<LineInfo> m_Lines;
	};
    }
}
