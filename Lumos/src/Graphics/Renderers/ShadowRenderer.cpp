#include "LM.h"
#include "ShadowRenderer.h"

#include "Graphics/API/Textures/TextureDepthArray.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Shader.h"

#include "Graphics/ModelLoader/ModelLoader.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Light.h"
#include "Graphics/RenderList.h"

#include "Maths/MathsUtilities.h"
#include "App/Scene.h"
#include "Maths/Maths.h"
#include "RenderCommand.h"
#include "System/JobSystem.h"
#include "System/Memory.h"

#define THREAD_CASCADE_GEN

namespace Lumos
{
	namespace Graphics
	{
		enum VSSystemUniformIndices : int32
		{
			VSSystemUniformIndex_ProjectionViewMatrix = 0,
			VSSystemUniformIndex_Size
		};

		ShadowRenderer::ShadowRenderer(TextureDepthArray* texture, uint shadowMapSize, uint numMaps)
			: m_ShadowTex(nullptr)
			, m_ShadowMapNum(numMaps)
			, m_ShadowMapSize(shadowMapSize)
			, m_ShadowMapsInvalidated(true)
			, m_UniformBuffer(nullptr)
			, m_ModelUniformBuffer(nullptr)
		{
			m_apShadowRenderLists = new RenderList*[SHADOWMAP_MAX];

			//Initialize the shadow render lists
			for (uint i = 0; i < m_ShadowMapNum; ++i)
			{
				m_apShadowRenderLists[i] = new RenderList();
				if (!RenderList::AllocateNewRenderList(m_apShadowRenderLists[i], true))
				{
					LUMOS_CORE_ERROR("Unable to allocate shadow render list {0} - Try using less shadow maps", i);
				}
			}

			m_Shader = Shader::CreateFromFile("Shadow", "/CoreShaders/");
			if (texture == nullptr)
			{
				m_ShadowTex = TextureDepthArray::Create(m_ShadowMapSize, m_ShadowMapSize, m_ShadowMapNum);
				m_DeleteTexture = true;
			}
			else
				m_ShadowTex = texture;

			m_DescriptorSet = nullptr;

			Init();
		}

		ShadowRenderer::~ShadowRenderer()
		{
			if (m_DeleteTexture)
				delete m_ShadowTex;

			for (uint i = 0; i < m_ShadowMapNum; ++i)
			{
				if (m_apShadowRenderLists)
					delete m_apShadowRenderLists[i];

				delete m_ShadowFramebuffer[i];
			}

			delete[] m_apShadowRenderLists;
			delete[] m_VSSystemUniformBuffer;
			delete[] m_PushConstant->data;
            
			delete m_PushConstant;
			delete m_DescriptorSet;
			delete m_Pipeline;
			delete m_UniformBuffer;
			delete m_ModelUniformBuffer;
			delete m_CommandBuffer;
			delete m_RenderPass;
			delete m_Shader;
            
            Memory::AlignedFree(uboDataDynamic.model);
		}

		void ShadowRenderer::Init()
		{
			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4) * 16;
			m_VSSystemUniformBuffer = new byte[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			m_PushConstant = new Graphics::PushConstant();
			m_PushConstant->type = Graphics::PushConstantDataType::UINT;
			m_PushConstant->size = sizeof(int32);
			m_PushConstant->data = new byte[sizeof(int32)];

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;

			m_RenderPass = Graphics::RenderPass::Create();
			AttachmentInfo textureTypes[2] =
			{
				{ TextureType::DEPTHARRAY, TextureFormat::DEPTH }
			};

			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 1;
			renderpassCI.textureType = textureTypes;

			m_RenderPass->Init(renderpassCI);

			m_CommandBuffer = Graphics::CommandBuffer::Create();
			m_CommandBuffer->Init(true);

			CreateGraphicsPipeline(m_RenderPass);
			CreateUniformBuffer();
			CreateFramebuffers();
		}

		void ShadowRenderer::OnResize(uint width, uint height)
		{
		}

		void ShadowRenderer::Begin()
		{
			m_CommandQueue.clear();
			m_CommandBuffer->BeginRecording();
			m_CommandBuffer->UpdateViewport(m_ShadowMapSize, m_ShadowMapSize);
		}

		void ShadowRenderer::BeginScene(Scene* scene)
		{
			UpdateCascades(scene);
		}

		void ShadowRenderer::EndScene()
		{
		}

		void ShadowRenderer::End()
		{
			m_CommandBuffer->EndRecording();
			m_CommandBuffer->Execute(true);
		}

		void ShadowRenderer::Present()
		{
			int index = 0;

			m_RenderPass->BeginRenderpass(m_CommandBuffer, Maths::Vector4(0.0f), m_ShadowFramebuffer[m_Layer], Graphics::INLINE, m_ShadowMapSize, m_ShadowMapSize);

			m_Pipeline->SetActive(m_CommandBuffer);

			for (auto& command : m_CommandQueue)
			{
				Mesh* mesh = command.mesh;

				uint32_t dynamicOffset = index * static_cast<uint32_t>(dynamicAlignment);

				Renderer::RenderMesh(mesh, m_Pipeline, m_CommandBuffer, dynamicOffset, nullptr, false);

				index++;
			}

			m_RenderPass->EndRenderpass(m_CommandBuffer);
		}

		void ShadowRenderer::SetShadowMapNum(uint num)
		{
			if (m_ShadowMapNum != num && num <= SHADOWMAP_MAX)
			{
				if (m_ShadowMapNum > 0)
				{
					for (int i = m_ShadowMapNum - 1; i >= static_cast<int>(num); --i)
					{
						delete m_apShadowRenderLists[i];
						m_apShadowRenderLists[i] = nullptr;
					}
				}
				for (uint i = m_ShadowMapNum; i < num; i++)
				{
					m_apShadowRenderLists[i] = new RenderList();
					RenderList::AllocateNewRenderList(m_apShadowRenderLists[i], true);
				}
				m_ShadowMapNum = num;
				m_ShadowMapsInvalidated = true;
			}
		}

		void ShadowRenderer::SetShadowMapSize(uint size)
		{
			if (!m_ShadowMapsInvalidated)
				m_ShadowMapsInvalidated = (size != m_ShadowMapSize);

			m_ShadowMapSize = size;
		}

		void ShadowRenderer::ClearRenderLists()
		{
			for (uint i = 0; i < m_ShadowMapNum; ++i)
			{
				if (m_apShadowRenderLists[i])
					m_apShadowRenderLists[i]->Clear();
			}
		}

		void ShadowRenderer::RenderScene(RenderList* renderList, Scene* scene)
		{
			//SortRenderLists(scene);
			UpdateCascades(scene);

			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], m_ShadowProjView, sizeof(Maths::Matrix4) * 16);

			Begin();
			for (uint i = 0; i < m_ShadowMapNum; ++i)
			{
				m_Layer = i;

				m_apShadowRenderLists[i]->RenderOpaqueObjects([&](Entity* obj)
				{
					if (obj)
					{
						const auto model = obj->GetComponent<MeshComponent>();

						if (model && model->m_Model)
						{
							auto mesh = model->m_Model;
							{
								SubmitMesh(mesh.get(), obj->GetComponent<TransformComponent>()->GetTransform().GetWorldMatrix(), Maths::Matrix4());
							}
						}
					}
				});

				SetSystemUniforms(m_Shader);

				int32 layer = static_cast<int32>(m_Layer);
				memcpy(m_PushConstant->data, &layer, sizeof(int32));
				std::vector<Graphics::PushConstant> pcVector;
				pcVector.push_back(*m_PushConstant);
				m_Pipeline->GetDescriptorSet()->SetPushConstants(pcVector);

				Present();

			}
			End();
		}

		float cascadeSplitLambda = 0.95f;

		void ShadowRenderer::UpdateCascades(Scene* scene)
		{
			if (!m_Light)
				return;

			float cascadeSplits[SHADOWMAP_MAX];

			float nearClip = scene->GetCamera()->GetNear();
			float farClip = scene->GetCamera()->GetFar();
			float clipRange = farClip - nearClip;

			float minZ = nearClip;
			float maxZ = nearClip + clipRange;
			float range = maxZ - minZ;
			float ratio = maxZ / minZ;
			// Calculate split depths based on view camera furstum
			// Based on method presentd in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
			for (uint32_t i = 0; i < m_ShadowMapNum; i++)
			{
				float p = (i + 1) / static_cast<float>(m_ShadowMapNum);
				float log = minZ * std::pow(ratio, p);
				float uniform = minZ + range * p;
				float d = cascadeSplitLambda * (log - uniform) + uniform;
				cascadeSplits[i] = (d - nearClip) / clipRange;
			}

#ifdef THREAD_CASCADE_GEN
			System::JobSystem::Dispatch(static_cast<uint32>(m_ShadowMapNum), 1, [&](JobDispatchArgs args)
#else
			for (uint32_t i = 0; i < m_ShadowMapNum; i++)
#endif
			{
#ifdef THREAD_CASCADE_GEN
				int i = args.jobIndex;
#endif
				float splitDist = cascadeSplits[i];
				float lastSplitDist = i == 0 ? 0.0f : cascadeSplits[i - 1];

				Maths::Vector3 frustumCorners[8] = {
					Maths::Vector3(-1.0f,  1.0f, -1.0f),
					Maths::Vector3(1.0f,  1.0f, -1.0f),
					Maths::Vector3(1.0f, -1.0f, -1.0f),
					Maths::Vector3(-1.0f, -1.0f, -1.0f),
					Maths::Vector3(-1.0f,  1.0f,  1.0f),
					Maths::Vector3(1.0f,  1.0f,  1.0f),
					Maths::Vector3(1.0f, -1.0f,  1.0f),
					Maths::Vector3(-1.0f, -1.0f,  1.0f),
				};

				scene->GetCamera()->BuildViewMatrix();
				Maths::Matrix4 cameraProj = scene->GetCamera()->GetProjectionMatrix();

				const Maths::Matrix4 invCam = Maths::Matrix4::Inverse(cameraProj * scene->GetCamera()->GetViewMatrix());

				// Project frustum corners into world space
				for (uint32_t i = 0; i < 8; i++)
				{
					Maths::Vector4 invCorner = invCam * Maths::Vector4(frustumCorners[i], 1.0f);
					frustumCorners[i] = (invCorner / invCorner.GetW()).ToVector3();
				}

				for (uint32_t i = 0; i < 4; i++)
				{
					Maths::Vector3 dist = frustumCorners[i + 4] - frustumCorners[i];
					frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
					frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
				}

				// Get frustum center
				Maths::Vector3 frustumCenter = Maths::Vector3(0.0f);
				for (uint32_t i = 0; i < 8; i++)
				{
					frustumCenter += frustumCorners[i];
				}
				frustumCenter /= 8.0f;

				float radius = 0.0f;
				for (uint32_t i = 0; i < 8; i++)
				{
					float distance = (frustumCorners[i] - frustumCenter).Length();
					radius = Maths::Max(radius, distance);
				}
				radius = std::ceil(radius * 16.0f) / 16.0f;
				float sceneBoundingRadius = scene->GetWorldRadius();
				//Extend the Z depths to catch shadow casters outside view frustum
				radius = Maths::Max(radius, sceneBoundingRadius);

				Maths::Vector3 maxExtents = Maths::Vector3(radius);
				Maths::Vector3 minExtents = -maxExtents;

				Maths::Vector3 lightDir = m_Light->m_Direction.ToVector3();
				lightDir.Normalise();
				Maths::Matrix4 lightViewMatrix = Maths::Matrix4::BuildViewMatrix(frustumCenter, frustumCenter + lightDir * -minExtents.z);

				//Maths::Matrix4 lightOrthoMatrix = Maths::Matrix4::Orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
				Maths::Matrix4 lightOrthoMatrix = Maths::Matrix4::Orthographic(maxExtents.z - minExtents.z, -(maxExtents.z - minExtents.z), maxExtents.x, minExtents.x, maxExtents.y, minExtents.y);

				// Store split distance and matrix in cascade
				m_SplitDepth[i] = Maths::Vector4((scene->GetCamera()->GetNear() + splitDist * clipRange) * -1.0f);
				m_ShadowProjView[i] = lightOrthoMatrix * lightViewMatrix;

				const Maths::Vector3 top_mid = frustumCenter + lightViewMatrix * Maths::Vector3(0.0f, 0.0f, maxExtents.GetZ());
				Maths::Frustum f;
				f.FromMatrix(m_ShadowProjView[i]);
				m_apShadowRenderLists[i]->UpdateCameraWorldPos(top_mid);
				m_apShadowRenderLists[i]->RemoveExcessObjects(f);
				m_apShadowRenderLists[i]->SortLists();
				scene->InsertToRenderList(m_apShadowRenderLists[i], f);
			}
#ifdef THREAD_CASCADE_GEN
			);
			System::JobSystem::Wait();
#endif
		}

		void ShadowRenderer::CreateFramebuffers()
		{
			if (m_ShadowMapsInvalidated && m_ShadowMapNum > 0)
			{
				m_ShadowMapsInvalidated = false;

				for (uint i = 0; i < m_ShadowMapNum; ++i)
				{
					const uint attachmentCount = 1;
					TextureType attachmentTypes[attachmentCount];
					attachmentTypes[0] = TextureType::DEPTHARRAY;

					FramebufferInfo bufferInfo{};
					bufferInfo.width = m_ShadowMapSize;
					bufferInfo.height = m_ShadowMapSize;
					bufferInfo.attachmentCount = attachmentCount;
					bufferInfo.renderPass = m_RenderPass;
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
			std::vector<Graphics::DescriptorPoolInfo> poolInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
				{ Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS },
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderStage::VERTEX, 0 },
				{ Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC,Graphics::ShaderStage::VERTEX , 1 },
			};

			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			std::vector<Graphics::DescriptorLayout> descriptorLayouts;

			Graphics::DescriptorLayout sceneDescriptorLayout{};
			sceneDescriptorLayout.count = static_cast<uint>(layoutInfo.size());
			sceneDescriptorLayout.layoutInfo = layoutInfo.data();

			descriptorLayouts.push_back(sceneDescriptorLayout);

			Graphics::PipelineInfo pipelineCI{};
			pipelineCI.pipelineName = "ShadowRenderer";
			pipelineCI.shader = m_Shader;
			pipelineCI.vulkanRenderpass = renderPass;
			pipelineCI.numVertexLayout = static_cast<uint>(attributeDescriptions.size());
			pipelineCI.descriptorLayouts = descriptorLayouts;
			pipelineCI.vertexLayout = attributeDescriptions.data();
			pipelineCI.numLayoutBindings = static_cast<uint>(poolInfo.size());
			pipelineCI.typeCounts = poolInfo.data();
			pipelineCI.strideSize = sizeof(Vertex);
			pipelineCI.numColorAttachments = 0;
			pipelineCI.wireframeEnabled = false;
			pipelineCI.cullMode = Graphics::CullMode::FRONT;
			pipelineCI.transparencyEnabled = false;
			pipelineCI.depthBiasEnabled = true;
			pipelineCI.width = m_ShadowMapSize;
			pipelineCI.height = m_ShadowMapSize;
			pipelineCI.maxObjects = MAX_OBJECTS;

			m_Pipeline = Graphics::Pipeline::Create(pipelineCI);
		}

		void ShadowRenderer::CreateUniformBuffer()
		{
			if (m_UniformBuffer == nullptr)
			{
				m_UniformBuffer = Graphics::UniformBuffer::Create();

				uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
				m_UniformBuffer->Init(bufferSize, nullptr);
			}

			if (m_ModelUniformBuffer == nullptr)
			{
				m_ModelUniformBuffer = Graphics::UniformBuffer::Create();
				const size_t minUboAlignment = Graphics::GraphicsContext::GetContext()->GetMinUniformBufferOffsetAlignment();

				dynamicAlignment = sizeof(Lumos::Maths::Matrix4);
				if (minUboAlignment > 0)
				{
					dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
				}

				uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment);

				uboDataDynamic.model = static_cast<Maths::Matrix4*>(Memory::AlignedAlloc(bufferSize2, dynamicAlignment));

				m_ModelUniformBuffer->Init(bufferSize2, nullptr);
			}

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.name = "UniformBufferObject";
			bufferInfo.size = sizeof(UniformBufferObject);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.systemUniforms = false;

			Graphics::BufferInfo bufferInfo2 = {};
			bufferInfo2.buffer = m_ModelUniformBuffer;
			bufferInfo2.offset = 0;
			bufferInfo2.size = sizeof(UniformBufferModel);
			bufferInfo2.type = Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
			bufferInfo2.binding = 1;
			bufferInfo2.shaderType = ShaderType::VERTEX;
			bufferInfo2.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);
			bufferInfos.push_back(bufferInfo2);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		}

		void ShadowRenderer::SetSystemUniforms(Shader* shader)
		{
			m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);

			int index = 0;

			for (auto& command : m_CommandQueue)
			{
				Maths::Matrix4* modelMat = reinterpret_cast<Maths::Matrix4*>((reinterpret_cast<uint64_t>(uboDataDynamic.model) + (index * dynamicAlignment)));
				*modelMat = command.transform;
				index++;
			}
			m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment), sizeof(Maths::Matrix4), &*uboDataDynamic.model);
		}

		void ShadowRenderer::Submit(const RenderCommand& command)
		{
			m_CommandQueue.emplace_back(command);
		}

		void ShadowRenderer::SubmitMesh(Mesh* mesh, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
		{
			RenderCommand command;
			command.mesh = mesh;
			command.transform = transform;
			Submit(command);
		}
	}
}
