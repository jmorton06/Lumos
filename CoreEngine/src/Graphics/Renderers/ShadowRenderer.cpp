#include "JM.h"
#include "ShadowRenderer.h"
#include "Graphics/RenderList.h"
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "Utilities/AssetsManager.h"
#include "Graphics/API/FrameBuffer.h"
#include "Renderer/Scene.h"
#include "Graphics/API/Shader.h"
#include "Maths/Maths.h"
#include "Maths/BoundingBox.h"
#include "Graphics/Model/Model.h"
#include "Maths/MathsUtilities.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/UniformBuffer.h"
#include "RenderCommand.h"

namespace jm
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
				JM_ERROR("Unable to allocate shadow render list %d! - Try using less shadow maps", i);
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
		//m_ShadowFBO = Framebuffer::Create();
		//m_ShadowFBO->AddTextureLayer(0, m_ShadowTex);

		Renderer::GetRenderer()->SetRenderTargets(0);

        m_DescriptorSet = nullptr;

		Init();
	}

	ShadowRenderer::~ShadowRenderer()
	{
		if(m_DeleteTexture)
			delete m_ShadowTex;

		for (uint i = 0; i < m_ShadowMapNum; ++i)
		{
			if (m_apShadowRenderLists)
				delete m_apShadowRenderLists[i];

			delete m_ShadowFramebuffer[i];
		}

		delete[] m_apShadowRenderLists;

		delete m_DescriptorSet;
		delete m_Pipeline;
		delete m_UniformBuffer;
		delete m_ModelUniformBuffer;
		delete m_CommandBuffer;
		delete m_RenderPass;
		delete m_Shader;
	}

	void ShadowRenderer::Init()
	{
		m_VSSystemUniformBufferSize = sizeof(maths::Matrix4);
		m_VSSystemUniformBuffer = new byte[m_VSSystemUniformBufferSize];
		memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
		m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

        // Per Scene System Uniforms
        m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;

		m_RenderPass = graphics::api::RenderPass::Create();
		TextureType textureTypes[1] = { TextureType::DEPTHARRAY };
		graphics::api::RenderpassInfo renderpassCI{};
		renderpassCI.attachmentCount = 1;
		renderpassCI.textureType = textureTypes;
		renderpassCI.depthOnly = true;

		m_RenderPass->Init(renderpassCI);

		m_CommandBuffer = graphics::api::CommandBuffer::Create();
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

	void ShadowRenderer::BeginScene(Camera * camera)
	{
	}

	void ShadowRenderer::EndScene()
	{
	}

	void ShadowRenderer::End()
	{
		m_CommandBuffer->EndRecording();
		//m_CommandBuffer->Execute(true);
	}

	void ShadowRenderer::Present()
	{
		int index = 0;

			m_RenderPass->BeginRenderpass(m_CommandBuffer, maths::Vector4(0.0f), m_ShadowFramebuffer[m_Layer], graphics::api::INLINE, m_ShadowMapSize, m_ShadowMapSize);

			m_Pipeline->SetActive(m_CommandBuffer);


		for (auto& command : m_CommandQueue)
		{
            Mesh* mesh = command.mesh;

			//graphics::api::CommandBuffer* currentCMDBuffer = mesh->GetCommandBuffer(0);

            uint32_t dynamicOffset = index * static_cast<uint32_t>(dynamicAlignment);

			//m_CommandBuffer->BeginRecording(m_RenderPass, m_ShadowFramebuffer[m_Layer]);
			//currentCMDBuffer->UpdateViewport(m_ShadowMapSize, m_ShadowMapSize);

			Renderer::RenderMesh(mesh, m_Pipeline, m_CommandBuffer, dynamicOffset, nullptr, false);


			//currentCMDBuffer->ExecuteSecondary(m_CommandBuffer);

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
			if(m_apShadowRenderLists[i])
				m_apShadowRenderLists[i]->Clear();
		}
	}

	void ShadowRenderer::RenderScene(RenderList* renderList, Scene* scene)
	{
		SortRenderLists(scene);

		Begin();
		for (uint i = 0; i < m_ShadowMapNum; ++i)
		{
			m_Layer = i;

			m_apShadowRenderLists[i]->RenderOpaqueObjects([&](Entity* obj)
			{
				if (obj)
				{
					ModelComponent* model = obj->GetComponent<ModelComponent>();

					if (model && model->m_Model)
					{
						for (auto& mesh : model->m_Model->GetMeshs())
						{
							SubmitMesh(mesh.get(), obj->GetComponent<TransformComponent>()->m_WorldSpaceTransform, maths::Matrix4());
						}
					}
				}
			});

			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], &m_ShadowProjView[m_Layer], sizeof(maths::Matrix4));

			SetSystemUniforms(m_Shader);

			Present();

		}
		End();

		Renderer::SetCulling(true);
	}

	void ShadowRenderer::SortRenderLists(Scene* scene)
	{
		scene->GetCamera()->BuildViewMatrix();
		maths::Matrix4 cameraProj = scene->GetCamera()->GetProjectionMatrix();
#ifdef JM_RENDER_API_VULKAN
		if (graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
		 	cameraProj[5] *= -1;
#endif

		const float proj_range = scene->GetCamera()->GetFar() - scene->GetCamera()->GetNear();
		const maths::Matrix4 lightView = maths::Matrix4::BuildViewMatrix(maths::Vector3(0.0f, 0.0f, 0.0f), scene->GetLightSetup()->GetDirectionalLightDirection());
		const maths::Matrix4 invCamProjView = maths::Matrix4::Inverse(cameraProj * scene->GetCamera()->GetViewMatrix());

		auto compute_depth = [&](float x)
		{
			float proj_start = -(proj_range * x + scene->GetCamera()->GetNear());
			return (proj_start * cameraProj[10] + cameraProj[14]) / (proj_start * cameraProj[11]);
		};

		const float divisor = (m_ShadowMapNum*m_ShadowMapNum) - 1.f;
		for (int i = 0; i < static_cast<int>(m_ShadowMapNum); ++i)
		{
			//Linear scalars going from 0.0f (near) to 1.0f (far)
			float lin_near = (powf(2.0f, static_cast<float>(i)) - 1.f) / divisor;
			float lin_far = (powf(2.0f, static_cast<float>(i + 1)) - 1.f) / divisor;

			//True non-linear depth ranging from -1.0f (near) to 1.0f (far)
			float norm_near = compute_depth(lin_near);
			float norm_far = compute_depth(lin_far);

			//Build Bounding Box around frustum section (Axis Aligned)
			maths::BoundingBox bb;
			bb.ExpandToFit(invCamProjView * maths::Vector3(-1.0f, -1.0f, norm_near));
			bb.ExpandToFit(invCamProjView * maths::Vector3(-1.0f, 1.0f, norm_near));
			bb.ExpandToFit(invCamProjView * maths::Vector3(1.0f, -1.0f, norm_near));
			bb.ExpandToFit(invCamProjView * maths::Vector3(1.0f, 1.0f, norm_near));
			bb.ExpandToFit(invCamProjView * maths::Vector3(-1.0f, -1.0f, norm_far));
			bb.ExpandToFit(invCamProjView * maths::Vector3(-1.0f, 1.0f, norm_far));
			bb.ExpandToFit(invCamProjView * maths::Vector3(1.0f, -1.0f, norm_far));
			bb.ExpandToFit(invCamProjView * maths::Vector3(1.0f, 1.0f, norm_far));

			//Rotate bounding box so it's orientated in the lights direction
            const maths::Vector3 centre = (bb.Upper() + bb.Lower()) * 0.5f;
            const maths::Matrix4 localView = lightView * maths::Matrix4::Translation(-centre);

			bb = bb.Transform(localView);

			float sceneBoundingRadius = 1000.0f;
			//Extend the Z depths to catch shadow casters outside view frustum
			bb.Lower().z = maths::Min(bb.Lower().z, -sceneBoundingRadius);
			bb.Upper().z = maths::Max(bb.Upper().z, sceneBoundingRadius);

			//Build Light Projection
            auto shadowProj = maths::Matrix4::Orthographic(bb.Upper().GetZ(), bb.Lower().GetZ(), bb.Lower().GetX(), bb.Upper().GetX(), bb.Upper().GetY(), bb.Lower().GetY());

			m_ShadowProjView[i] = shadowProj * localView;

			//Construct Shadow RenderList
			const maths::Vector3 top_mid = centre + lightView * maths::Vector3(0.0f, 0.0f, bb.Upper().GetZ());
			maths::Frustum f;
			f.FromMatrix(m_ShadowProjView[i]);
			m_apShadowRenderLists[i]->UpdateCameraWorldPos(top_mid);
			m_apShadowRenderLists[i]->RemoveExcessObjects(f);
			m_apShadowRenderLists[i]->SortLists();
			scene->InsertToRenderList(m_apShadowRenderLists[i], f);
		}
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

				FrameBufferInfo bufferInfo{};
				bufferInfo.width = m_ShadowMapSize;
				bufferInfo.height = m_ShadowMapSize;
				bufferInfo.attachmentCount = attachmentCount;
				bufferInfo.renderPass = m_RenderPass;
				bufferInfo.attachmentTypes = attachmentTypes;
				bufferInfo.layer = i;

				Texture* attachments[attachmentCount];
				attachments[0] = m_ShadowTex;
				bufferInfo.attachments = attachments;

				m_ShadowFramebuffer[i] = Framebuffer::Create(bufferInfo);
			}
		}
	}

	void ShadowRenderer::CreateGraphicsPipeline(graphics::api::RenderPass* renderPass)
	{
		 std::vector<graphics::api::DescriptorPoolInfo> poolInfo =
        {
           	{ graphics::api::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
            { graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS },
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, graphics::api::ShaderStage::VERTEX, 0 },
            { graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC,graphics::api::ShaderStage::VERTEX , 1 },
        };

        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        std::vector<graphics::api::DescriptorLayout> descriptorLayouts;

        graphics::api::DescriptorLayout sceneDescriptorLayout{};
        sceneDescriptorLayout.count = static_cast<uint>(layoutInfo.size());
        sceneDescriptorLayout.layoutInfo = layoutInfo.data();

        descriptorLayouts.push_back(sceneDescriptorLayout);

        graphics:: api::PipelineInfo pipelineCI{};
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
        pipelineCI.cullMode = graphics::api::CullMode::NONE;
        pipelineCI.transparencyEnabled = false;
        pipelineCI.depthBiasEnabled = true;
        pipelineCI.width = m_ShadowMapSize;
        pipelineCI.height = m_ShadowMapSize;
        pipelineCI.maxObjects = MAX_OBJECTS;

        m_Pipeline = graphics::api::Pipeline::Create(pipelineCI);
	}

	void ShadowRenderer::CreateUniformBuffer()
	{
		if(m_UniformBuffer == nullptr)
		{
			m_UniformBuffer = graphics::api::UniformBuffer::Create();

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
			m_UniformBuffer->Init(bufferSize, nullptr);
		}

		if(m_ModelUniformBuffer == nullptr)
		{
			m_ModelUniformBuffer = graphics::api::UniformBuffer::Create();
			const size_t minUboAlignment = graphics::Context::GetContext()->GetMinUniformBufferOffsetAlignment();

			dynamicAlignment = sizeof(jm::maths::Matrix4);
			if (minUboAlignment > 0)
			{
				dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment);

			uboDataDynamic.model = static_cast<maths::Matrix4*>(AlignedAlloc(bufferSize2, dynamicAlignment));

			m_ModelUniformBuffer->Init(bufferSize2, nullptr);
		}

		std::vector<graphics::api::BufferInfo> bufferInfos;

		graphics::api::BufferInfo bufferInfo = {};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.binding = 0;
		bufferInfo.shaderType = ShaderType::VERTEX;
		bufferInfo.systemUniforms = true;

		graphics::api::BufferInfo bufferInfo2 = {};
		bufferInfo2.buffer = m_ModelUniformBuffer;
		bufferInfo2.offset = 0;
		bufferInfo2.size = sizeof(UniformBufferModel);
		bufferInfo2.type = graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
		bufferInfo2.binding = 1;
		bufferInfo2.shaderType = ShaderType::VERTEX;
		bufferInfo2.systemUniforms = false;

		bufferInfos.push_back(bufferInfo);
		bufferInfos.push_back(bufferInfo2);

		m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
	}

	void ShadowRenderer::SetSystemUniforms(Shader* shader) const
	{
        m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);

        int index = 0;

        for (auto& command : m_CommandQueue)
        {
            maths::Matrix4* modelMat = reinterpret_cast<maths::Matrix4*>((reinterpret_cast<uint64_t>(uboDataDynamic.model) + (index * dynamicAlignment)));
            *modelMat = command.transform;
            index++;
        }
        m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment), sizeof(maths::Matrix4), &*uboDataDynamic.model);
	}

	void ShadowRenderer::Submit(const RenderCommand& command)
	{
		m_CommandQueue.push_back(command);
	}

	void ShadowRenderer::SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix)
	{
		RenderCommand command;
		command.mesh = mesh;
		command.transform = transform;
		Submit(command);
	}
}
