#pragma once
#include "LM.h"
#include "RenderCommand.h"


namespace Lumos
{
	class RenderList;
	class Shader;
	class Scene;
	class Shadow;
	struct DirectionalLight;
	class Camera;
	class Framebuffer;
	class TextureCube;

	typedef std::vector<RenderCommand> CommandQueue;
	typedef std::vector<RendererUniform> SystemUniformList;

	namespace graphics
	{
		namespace api
		{
			class Pipeline;
			class DescriptorSet;
			class RenderPass;
		}
	}

	class LUMOS_EXPORT Renderer3D
	{
	public:

		virtual	~Renderer3D() {}

		virtual void RenderScene(RenderList* renderList, Scene* scene) = 0;
		Framebuffer* GetFBO() const { return m_FBO; }

	protected:
		Framebuffer* m_FBO;
		Shader* m_Shader;

		Lumos::graphics::api::RenderPass* m_RenderPass;
		Lumos::graphics::api::Pipeline* m_Pipeline;
		graphics::api::DescriptorSet* m_DescriptorSet;

		uint m_ScreenBufferWidth, m_ScreenBufferHeight;
		CommandQueue m_CommandQueue;
		SystemUniformList m_SystemUniforms;

	public:

		virtual void Init() = 0;
		virtual void Begin() = 0;
		virtual void BeginScene(Scene* scene) = 0;
		virtual void Submit(const RenderCommand& command) = 0;
		virtual void SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix) = 0;
		virtual void EndScene() = 0;
		virtual void End() = 0;
		virtual void Present() = 0;

		virtual void OnResize(uint width, uint height) = 0;

        virtual void SetScreenBufferSize(uint width, uint height) { if(width == 0) width = 1; if(height == 0) height = 1; m_ScreenBufferWidth = width; m_ScreenBufferHeight = height; }
	};
}

