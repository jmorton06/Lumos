#pragma once
#include "lmpch.h"
#include "RenderCommand.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

namespace Lumos
{
	class RenderList;
	class Scene;
	class Camera;
	class Material;

	namespace Graphics
	{
		class Pipeline;
		class DescriptorSet;
		class RenderPass;
		class Framebuffer;
		class TextureCube;
		class Texture;
		class Shader;

		typedef std::vector<RenderCommand> CommandQueue;
		typedef std::vector<RendererUniform> SystemUniformList;

		class LUMOS_EXPORT Renderer3D
		{
		public:
			virtual ~Renderer3D()
			{
			}

			virtual void RenderScene(Scene* scene) = 0;
			Framebuffer* GetFBO() const
			{
				return m_FBO;
			}

			virtual void Init() = 0;
			virtual void Begin() = 0;
			virtual void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) = 0;
			virtual void Submit(const RenderCommand& command) = 0;
			virtual void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) = 0;
			virtual void EndScene() = 0;
			virtual void End() = 0;
			virtual void Present() = 0;

			virtual void OnResize(u32 width, u32 height) = 0;

			virtual void SetScreenBufferSize(u32 width, u32 height)
			{
				if(width == 0)
					width = 1;
				if(height == 0)
					height = 1;
				m_ScreenBufferWidth = width;
				m_ScreenBufferHeight = height;
			}

			virtual void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer = true)
			{
				m_RenderTexture = texture;
			}
			virtual void OnImGui(){};

			void SetCamera(Camera* camera)
			{
				m_Camera = camera;
			}
			Texture* GetRenderTarget() const
			{
				return m_RenderTexture;
			}

		protected:
			Framebuffer* m_FBO = nullptr;
			Shader* m_Shader = nullptr;
			Camera* m_Camera = nullptr;
			Maths::Transform* m_CameraTransform = nullptr;

			Lumos::Graphics::RenderPass* m_RenderPass = nullptr;
			Lumos::Graphics::Pipeline* m_Pipeline = nullptr;
			Graphics::DescriptorSet* m_DescriptorSet = nullptr;

			u32 m_ScreenBufferWidth = 0, m_ScreenBufferHeight = 0;
			CommandQueue m_CommandQueue;
			SystemUniformList m_SystemUniforms;
			Texture* m_RenderTexture = nullptr;
			Texture* m_DepthTexture = nullptr;
		};
	}
}
