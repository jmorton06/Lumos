#include "Precompiled.h"
#include "ShadowRenderer.h"

#include "Graphics/API/Texture.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Shader.h"

#include "Graphics/Model.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Light.h"
#include "Maths/Transform.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Maths/Maths.h"
#include "RenderCommand.h"
 

#include <imgui/imgui.h>

//#define THREAD_CASCADE_GEN
#ifdef THREAD_CASCADE_GEN
#	include "Core/JobSystem.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
		enum VSSystemUniformIndices : i32
		{
			VSSystemUniformIndex_ProjectionViewMatrix = 0,
			VSSystemUniformIndex_Size
		};

		ShadowRenderer::ShadowRenderer(TextureDepthArray* texture, u32 shadowMapSize, u32 numMaps)
			: m_ShadowTex(nullptr)
			, m_ShadowMapNum(numMaps)
			, m_ShadowMapSize(shadowMapSize)
			, m_ShadowMapsInvalidated(true)
			, m_UniformBuffer(nullptr)
		{
			m_Shader = Ref<Graphics::Shader>(Shader::CreateFromFile("Shadow", "/CoreShaders/"));
			if(texture == nullptr)
			{
				m_ShadowTex = TextureDepthArray::Create(m_ShadowMapSize, m_ShadowMapSize, m_ShadowMapNum);
			}
			else
				m_ShadowTex = texture;

			m_DescriptorSet = nullptr;

			ShadowRenderer::Init();
		}

		ShadowRenderer::~ShadowRenderer()
		{
			delete m_ShadowTex;

			for(u32 i = 0; i < m_ShadowMapNum; ++i)
			{
				delete m_ShadowFramebuffer[i];
			}

			delete[] m_VSSystemUniformBuffer;
            
            for(auto& pc: m_PushConstants)
                delete[] pc.data;
            
            m_PushConstants.clear();

			delete m_UniformBuffer;
			delete m_CommandBuffer;
		}

		void ShadowRenderer::Init()
		{
			LUMOS_PROFILE_FUNCTION();
			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4) * SHADOWMAP_MAX;
			m_VSSystemUniformBuffer = new u8[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			auto pushConstant = Graphics::PushConstant();
            pushConstant.size = sizeof(i32) + sizeof(Lumos::Maths::Matrix4);
            pushConstant.data = new u8[sizeof(i32) + sizeof(Lumos::Maths::Matrix4)];
            pushConstant.shaderStage = ShaderType::VERTEX;
            
            m_PushConstants.push_back(pushConstant);

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;

			m_RenderPass = Ref<Graphics::RenderPass>(Graphics::RenderPass::Create());
			AttachmentInfo textureTypes[1] =
				{
					{TextureType::DEPTHARRAY, TextureFormat::DEPTH}};

			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 1;
			renderpassCI.textureType = textureTypes;
			renderpassCI.clear = true;

			m_RenderPass->Init(renderpassCI);

			m_CommandBuffer = Graphics::CommandBuffer::Create();
			m_CommandBuffer->Init(true);

			CreateGraphicsPipeline(m_RenderPass.get());
			CreateUniformBuffer();
			CreateFramebuffers();
            m_CurrentDescriptorSets.resize(1);

			m_CommandQueue.reserve(1000);
		}

		void ShadowRenderer::OnResize(u32 width, u32 height)
		{
		}

		void ShadowRenderer::Begin()
		{
			LUMOS_PROFILE_FUNCTION();
			m_CommandQueue.clear();
			
			m_CommandBuffer->BeginRecording();
		}

		void ShadowRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
		{
			LUMOS_PROFILE_FUNCTION();
			UpdateCascades(scene, overrideCamera, overrideCameraTransform);
		}

		void ShadowRenderer::EndScene()
		{
		}

		void ShadowRenderer::End()
		{
			LUMOS_PROFILE_FUNCTION();
			m_CommandBuffer->EndRecording();
			m_CommandBuffer->Execute(false);
		}

		void ShadowRenderer::Present()
		{
			LUMOS_PROFILE_FUNCTION();
			int index = 0;

			m_RenderPass->BeginRenderpass(m_CommandBuffer, Maths::Vector4(0.0f), m_ShadowFramebuffer[m_Layer], Graphics::INLINE, m_ShadowMapSize, m_ShadowMapSize, false);

			m_Pipeline->Bind(m_CommandBuffer);

			for(auto& command : m_CommandQueue)
			{
				Engine::Get().Statistics().NumShadowObjects++;
				
				Mesh* mesh = command.mesh;

                m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();

				mesh->GetVertexBuffer()->Bind(m_CommandBuffer, m_Pipeline.get());
				mesh->GetIndexBuffer()->Bind(m_CommandBuffer);
                
                u32 layer = static_cast<u32>(m_Layer);
                auto trans = command.transform;
                memcpy(m_PushConstants[0].data, &trans, sizeof(Maths::Matrix4));
                memcpy(m_PushConstants[0].data + sizeof(Maths::Matrix4), &layer, sizeof(u32));

                m_CurrentDescriptorSets[0]->SetPushConstants(m_PushConstants);
                
				Renderer::BindDescriptorSets(m_Pipeline.get(), m_CommandBuffer, 0, m_CurrentDescriptorSets);
				Renderer::DrawIndexed(m_CommandBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

				mesh->GetVertexBuffer()->Unbind();
				mesh->GetIndexBuffer()->Unbind();

				index++;
			}

			m_RenderPass->EndRenderpass(m_CommandBuffer, false);
            //m_CommandBuffer->Execute(true);
		}

		void ShadowRenderer::SetShadowMapNum(u32 num)
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_ShadowMapNum != num && num <= SHADOWMAP_MAX)
			{
				m_ShadowMapNum = num;
				m_ShadowMapsInvalidated = true;
			}
		}

		void ShadowRenderer::SetShadowMapSize(u32 size)
		{
			LUMOS_PROFILE_FUNCTION();
			if(!m_ShadowMapsInvalidated)
				m_ShadowMapsInvalidated = (size != m_ShadowMapSize);

			m_ShadowMapSize = size;
		}

		void ShadowRenderer::RenderScene(Scene* scene)
		{
			LUMOS_PROFILE_FUNCTION();

			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], m_ShadowProjView, sizeof(Maths::Matrix4) * SHADOWMAP_MAX);

			Begin();

			auto& registry = scene->GetRegistry();

			auto group = registry.group<Model>(entt::get<Maths::Transform>);

			for(u32 i = 0; i < m_ShadowMapNum; ++i)
			{
				LUMOS_PROFILE_SCOPE("ShadowRenderer::RenderScene Per Shadow Map");
				m_Layer = i;

				Maths::Frustum f;
				f.Define(m_ShadowProjView[i]);
				
				if(group.empty())
					continue;
				
				for(auto entity : group)
				{
					const auto& [model, trans] = group.get<Model, Maths::Transform>(entity);
                    const auto& meshes = model.GetMeshes();
                                   
                   for(auto mesh : meshes)
                   {
                        if(mesh->GetActive())
                        {
                            auto& worldTransform = trans.GetWorldMatrix();

                            auto bb = mesh->GetBoundingBox();
                            auto bbCopy = bb->Transformed(worldTransform);
                            auto inside = f.IsInsideFast(bbCopy);

                            if(inside == Maths::Intersection::OUTSIDE)
                                continue;

                            SubmitMesh(mesh.get(), nullptr, worldTransform, Maths::Matrix4());
                        }
                   }
				}

				SetSystemUniforms(m_Shader.get());

				Present();
			}
			End();
		}

		float cascadeSplitLambda = 0.95f;

		void ShadowRenderer::UpdateCascades(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
		{
			LUMOS_PROFILE_FUNCTION();

			auto& registry = scene->GetRegistry();
			auto view = registry.view<Graphics::Light>();

			Light* light = nullptr;

			for(auto& lightEntity : view)
			{
				auto& currentLight = view.get<Graphics::Light>(lightEntity);
				if(currentLight.Type == (float)Graphics::LightType::DirectionalLight)
					light = &currentLight;
			}

			if(!light)
				return;

			float cascadeSplits[SHADOWMAP_MAX];

			if(overrideCamera)
			{
				m_Camera = overrideCamera;
				m_CameraTransform = overrideCameraTransform;
			}
			else
			{
				auto cameraView = registry.view<Camera>();
				if(!cameraView.empty())
				{
					m_Camera = &cameraView.get<Camera>(cameraView.front());
					m_CameraTransform = registry.try_get<Maths::Transform>(cameraView.front());
				}
			}

			if(!m_Camera || !m_CameraTransform)
				return;

			float nearClip = m_Camera->GetNear();
			float farClip = m_Camera->GetFar();
			float clipRange = farClip - nearClip;

			float minZ = nearClip;
			float maxZ = nearClip + clipRange;
			float range = maxZ - minZ;
			float ratio = maxZ / minZ;
			// Calculate split depths based on view camera frustum
			// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
			for(uint32_t i = 0; i < m_ShadowMapNum; i++)
			{
				float p = static_cast<float>(i + 1) / static_cast<float>(m_ShadowMapNum);
				float log = minZ * std::pow(ratio, p);
				float uniform = minZ + range * p;
				float d = cascadeSplitLambda * (log - uniform) + uniform;
				cascadeSplits[i] = (d - nearClip) / clipRange;
			}

#ifdef THREAD_CASCADE_GEN
			System::JobSystem::Dispatch(static_cast<u32>(m_ShadowMapNum), 1, [&](JobDispatchArgs args)
#else
			for(uint32_t i = 0; i < m_ShadowMapNum; i++)
#endif
				{
#ifdef THREAD_CASCADE_GEN
					int i = args.jobIndex;
#endif
					float splitDist = cascadeSplits[i];
					float lastSplitDist = i == 0 ? 0.0f : cascadeSplits[i - 1];

					Maths::Vector3 frustumCorners[8] = {
						Maths::Vector3(-1.0f, 1.0f, -1.0f),
						Maths::Vector3(1.0f, 1.0f, -1.0f),
						Maths::Vector3(1.0f, -1.0f, -1.0f),
						Maths::Vector3(-1.0f, -1.0f, -1.0f),
						Maths::Vector3(-1.0f, 1.0f, 1.0f),
						Maths::Vector3(1.0f, 1.0f, 1.0f),
						Maths::Vector3(1.0f, -1.0f, 1.0f),
						Maths::Vector3(-1.0f, -1.0f, 1.0f),
					};

					const Maths::Matrix4 invCam = Maths::Matrix4::Inverse(m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse());

					// Project frustum corners into world space
					for(uint32_t j = 0; j < 8; j++)
					{
						Maths::Vector4 invCorner = invCam * Maths::Vector4(frustumCorners[j], 1.0f);
						frustumCorners[j] = (invCorner / invCorner.w).ToVector3();
					}

					for(uint32_t j = 0; j < 4; j++)
					{
						Maths::Vector3 dist = frustumCorners[j + 4] - frustumCorners[j];
						frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
						frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
					}

					// Get frustum center
					Maths::Vector3 frustumCenter = Maths::Vector3(0.0f);
					for(uint32_t j = 0; j < 8; j++)
					{
						frustumCenter += frustumCorners[j];
					}
					frustumCenter /= 8.0f;

					float radius = 0.0f;
					for(uint32_t j = 0; j < 8; j++)
					{
						float distance = (frustumCorners[j] - frustumCenter).Length();
						radius = Maths::Max(radius, distance);
					}
					radius = std::ceil(radius * 16.0f) / 16.0f;
					float sceneBoundingRadius = m_Camera->GetShadowBoundingRadius() * 1.4f;
					//Extend the Z depths to catch shadow casters outside view frustum
					radius = Maths::Max(radius, sceneBoundingRadius);

					Maths::Vector3 maxExtents = Maths::Vector3(radius);
					Maths::Vector3 minExtents = -maxExtents;

					Maths::Vector3 lightDir = -light->Direction.ToVector3();
					lightDir.Normalize();
					Maths::Matrix4 lightViewMatrix = Maths::Quaternion::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter).RotationMatrix4();
					lightViewMatrix.SetTranslation(frustumCenter);

					Maths::Matrix4 lightOrthoMatrix = Maths::Matrix4::Orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -(maxExtents.z - minExtents.z), maxExtents.z - minExtents.z);

                    auto shadowProj = lightOrthoMatrix * lightViewMatrix.Inverse();
                    const bool StabilizeCascades = true;
					if(StabilizeCascades)
					{
						// Create the rounding matrix, by projecting the world-space origin and determining
						// the fractional offset in texel space
						Maths::Matrix4 shadowMatrix = shadowProj;
						Maths::Vector3 shadowOrigin = Maths::Vector3(0.0f);
						shadowOrigin = (shadowMatrix * Maths::Vector4(shadowOrigin, 1.0f)).ToVector3();
						shadowOrigin *= (m_ShadowMapSize / 2.0f);

                        Maths::Vector3 roundedOrigin = Maths::VectorRound(shadowOrigin);
                        Maths::Vector3 roundOffset = roundedOrigin - shadowOrigin;
						roundOffset = roundOffset * (2.0f / m_ShadowMapSize);
						roundOffset.z = 0.0f;

                        shadowProj.ElementRef(0, 3) += roundOffset.x;
                        shadowProj.ElementRef(1, 3) += roundOffset.y;
					}
					// Store split distance and matrix in cascade
					m_SplitDepth[i] = Maths::Vector4((m_Camera->GetNear() + splitDist * clipRange) * -1.0f);
					m_ShadowProjView[i] = shadowProj;
				}
#ifdef THREAD_CASCADE_GEN
			);
			System::JobSystem::Wait();
#endif
		}

		void ShadowRenderer::CreateFramebuffers()
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_ShadowMapsInvalidated && m_ShadowMapNum > 0)
			{
				m_ShadowMapsInvalidated = false;

				for(u32 i = 0; i < m_ShadowMapNum; ++i)
				{
					const u32 attachmentCount = 1;
					TextureType attachmentTypes[attachmentCount];
					attachmentTypes[0] = TextureType::DEPTHARRAY;

					FramebufferInfo bufferInfo{};
					bufferInfo.width = m_ShadowMapSize;
					bufferInfo.height = m_ShadowMapSize;
					bufferInfo.attachmentCount = attachmentCount;
					bufferInfo.renderPass = m_RenderPass.get();
					bufferInfo.attachmentTypes = attachmentTypes;
					bufferInfo.layer = i;
					bufferInfo.screenFBO = false;

					Texture* attachments[attachmentCount];
					attachments[0] = m_ShadowTex;
					bufferInfo.attachments = attachments;

					m_ShadowFramebuffer[i] = Framebuffer::Create(bufferInfo);
				}
			}
		}

		void ShadowRenderer::CreateGraphicsPipeline(Graphics::RenderPass* renderPass)
		{
			LUMOS_PROFILE_FUNCTION();
			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
            {
                {Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0},
            };
            
            Graphics::BufferLayout vertexBufferLayout;
            vertexBufferLayout.Push<Maths::Vector3>("position");
            vertexBufferLayout.Push<Maths::Vector4>("colour");
            vertexBufferLayout.Push<Maths::Vector2>("uv");
            vertexBufferLayout.Push<Maths::Vector3>("normal");
            vertexBufferLayout.Push<Maths::Vector3>("tangent");

			Graphics::PipelineInfo pipelineCreateInfo;
			pipelineCreateInfo.pipelineName = "ShadowRenderer";
			pipelineCreateInfo.shader = m_Shader.get();
			pipelineCreateInfo.renderpass = renderPass;
            pipelineCreateInfo.vertexBufferLayout = vertexBufferLayout;
			pipelineCreateInfo.descriptorLayouts = layoutInfo;
			pipelineCreateInfo.polygonMode = Graphics::PolygonMode::Fill;
			pipelineCreateInfo.cullMode = Graphics::CullMode::NONE;
			pipelineCreateInfo.transparencyEnabled = false;
			pipelineCreateInfo.depthBiasEnabled = true;
            pipelineCreateInfo.pushConstSize = sizeof(u32) + sizeof(Maths::Matrix4);
            pipelineCreateInfo.pushConstSize = 1;
			pipelineCreateInfo.maxObjects = MAX_OBJECTS;

			m_Pipeline = Ref<Graphics::Pipeline>(Graphics::Pipeline::Create(pipelineCreateInfo));
		}

		void ShadowRenderer::CreateUniformBuffer()
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_UniformBuffer == nullptr)
			{
				m_UniformBuffer = Graphics::UniformBuffer::Create();

				const uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
				m_UniformBuffer->Init(bufferSize, nullptr);
			}
            
			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo;
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.name = "UniformBufferObject";
			bufferInfo.size = sizeof(UniformBufferObject);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		}

		void ShadowRenderer::SetSystemUniforms(Shader* shader)
		{
			LUMOS_PROFILE_FUNCTION();
			
			{
				LUMOS_PROFILE_SCOPE("Vertex Uniform Buffer Update");
                m_UniformBuffer->SetData(sizeof(Maths::Matrix4) * m_ShadowMapNum, *&m_VSSystemUniformBuffer);
			}
		}

		void ShadowRenderer::Submit(const RenderCommand& command)
		{
			m_CommandQueue.push_back(command);
		}

		void ShadowRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
		{
			LUMOS_PROFILE_FUNCTION();
			RenderCommand command;
			command.mesh = mesh;
			command.transform = transform;
			command.material = material;
			Submit(command);
		}

		void ShadowRenderer::OnImGui()
		{
			LUMOS_PROFILE_FUNCTION();
			ImGui::TextUnformatted("Shadow Renderer");
			if(ImGui::TreeNode("Texture"))
			{
				static int index = 0;

				ImGui::InputInt("Texture Array Index", &index);

				index = Maths::Max(0, index);
				index = Maths::Min(index, 3);
				bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

				ImGui::Image(m_ShadowTex->GetHandleArray(u32(index)), ImVec2(128, 128), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if(ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(m_ShadowTex->GetHandleArray(u32(index)), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}

				ImGui::TreePop();
			}
		}
	}
}
