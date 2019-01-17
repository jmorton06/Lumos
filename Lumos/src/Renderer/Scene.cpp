#include "LM.h"
#include "Scene.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/API/Context.h"
#include "System/String.h"
#include "Audio/SoundSystem.h"
#include "Graphics/Mesh.h"
#include "Physics/JMPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/JMPhysicsEngine/Octree.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Light.h"
#include "Graphics/Model/Model.h"
#include "Utilities/TimeStep.h"
#include "App/Input.h"
#include "App/Application.h"

namespace Lumos
{
	Scene::Scene(const String& friendly_name)
		: m_SceneName(friendly_name), m_pCamera(nullptr), m_EnvironmentMap(nullptr), m_SceneBoundingRadius(0), m_DebugDrawFlags(0), m_DrawDebugData(false)
	{
		m_ParticleManager = nullptr;//  new ParticleManager();

		m_LightSetup = new LightSetup();

		m_MaterialManager = new AssetManager<Material>();
	}

    Scene::~Scene()
    {
        DeleteAllGameObjects();

        if (m_ParticleManager)
        {
            delete m_ParticleManager;
            m_ParticleManager = nullptr;
        }

        delete m_LightSetup;

        SAFE_DELETE(m_MaterialManager);
    }

	void Scene::OnInit()
	{
		m_CurrentScene = true;

		String Configuration;
		String Platform;
		String RenderAPI;
		String dash = " - ";

#ifdef LUMOS_DEBUG
		Configuration = "Debug";
#else
		Configuration = "Release";
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
		Platform = "Windows";
#elif LUMOS_PLATFORM_LINUX
		Platform = "Linux";
#elif LUMOS_PLATFORM_MACOS
		Platform = "MacOS";
#elif LUMOS_PLATFORM_IOS
		Platform = "iOS";
#endif

		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case OPENGL: RenderAPI = "OpenGL"; break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
		case VULKAN: RenderAPI = "Vulkan ( MoltenVK )"; break;
#else
		case VULKAN: RenderAPI = "Vulkan"; break;
#endif
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
		case DIRECT3D: RenderAPI = "Direct3D"; break;
#endif
		}

		std::stringstream Title;
		Title << Platform << dash << RenderAPI << dash << Configuration << dash << m_SceneName;

		Application::Instance()->GetWindow()->SetWindowTitle(Title.str());

		//Default physics setup
		JMPhysicsEngine::Instance()->SetDampingFactor(0.998f);
		JMPhysicsEngine::Instance()->SetIntegrationType(INTEGRATION_RUNGE_KUTTA_4);
		JMPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 5, std::make_shared<SortAndSweepBroadphase>()));
		SetDebugDrawFlags(DEBUGDRAW_FLAGS_COLLISIONVOLUMES
			| DEBUGDRAW_FLAGS_AABB
			| DEBUGDRAW_FLAGS_COLLISIONNORMALS
			| DEBUGDRAW_FLAGS_BROADPHASE
			| DEBUGDRAW_FLAGS_CONSTRAINT
		);
		m_SceneBoundingRadius = 400.0f; //Default scene radius of 400m

		m_BackgroundColour = maths::Vector3(0.8f, 0.8f, 0.8f);
		m_UseShadow    = true;
		m_DrawObjects  = true;
		m_ReflectScene = false;

		m_pFrameRenderList = std::make_unique<RenderList>();

		if (!RenderList::AllocateNewRenderList(m_pFrameRenderList.get(), true))
		{
			LUMOS_CORE_ERROR("Unable to allocate scene render list! - Try using less shadow maps");
		}
	}

	void Scene::OnCleanupScene()
	{
		m_LightSetup->Clear();
		m_MaterialManager->Clear();

		m_pFrameRenderList.reset();

		DeleteAllGameObjects();

		SoundSystem::Instance()->RemoveAllSoundNodes();

		Input::GetInput().Reset();

		m_CurrentScene = false;
	};

	void Scene::AddEntity(std::shared_ptr<Entity> game_object)
	{
		if (game_object->GetComponent<Physics3DComponent>())
			game_object->GetComponent<Physics3DComponent>()->m_PhysicsObject->AutoResizeBoundingBox();

		m_Entities.emplace_back(game_object);
	}


	void Scene::DeleteAllGameObjects()
	{
		m_Entities.clear();
	}

	void Scene::OnUpdate(TimeStep* timeStep)
	{
		const maths::Vector2 mousePos = Input::GetInput().GetMousePosition();

		if(m_pCamera)
		{
			m_pCamera->HandleMouse(timeStep->GetSeconds(), mousePos.GetX(), mousePos.GetY());
			m_pCamera->HandleKeyboard(timeStep->GetSeconds());
			m_pCamera->BuildViewMatrix();
		}

		BuildFrameRenderList();

		for (const auto& entity : m_Entities)
		{
			entity->OnUpdateObject(timeStep->GetSeconds());
		}

		if(m_ParticleManager)
			m_ParticleManager->Update(timeStep->GetSeconds());
	}

	void Scene::BuildWorldMatrices()
	{
		for (const auto& entity : m_Entities)
		{
			Physics3DComponent* physicsComponent = entity->GetComponent<Physics3DComponent>();
			TransformComponent* transformComponent = entity->GetComponent<TransformComponent>();
			if (physicsComponent)
			{
				if(transformComponent)
					transformComponent->m_WorldSpaceTransform = physicsComponent->m_PhysicsObject->GetWorldSpaceTransform() * transformComponent->m_LocalTransform;
			}
			else
            {
                if (transformComponent)
                    transformComponent->m_WorldSpaceTransform = transformComponent->m_LocalTransform;
            }
		}
	}

	void Scene::DebugRender()
	{
		if (m_DebugDrawFlags & DEBUGDRAW_FLAGS_ENTITY_COMPONENTS)
		{
			for(auto& entity : m_Entities)
			{
				entity->DebugDraw(m_DebugDrawFlags);
			}
		}
	}

	void Scene::InsertToRenderList(RenderList* list, const maths::Frustum& frustum) const
	{
		for (const auto& entity : m_Entities)
		{
			auto modelComponent = entity->GetComponent<ModelComponent>();
			if(modelComponent)
			{
				bool inside = modelComponent->m_Model->GetNeedFrustumCheck() ? frustum.InsideFrustum(entity->GetComponent<TransformComponent>()->m_WorldSpaceTransform.GetPositionVector(), entity->GetBoundingRadius()) : true;

				if (inside)
				{
					//Check to see if the object is already listed or not
					if (!(list->BitMask() & entity->GetFrustumCullFlags()))
					{
						list->InsertObject(entity.get());
					}
				}
			}
		}
	}

	void Scene::AddPointLight(std::shared_ptr<Light> light) const
	{
		m_LightSetup->Add(light);
	}

	void Scene::BuildFrameRenderList()
	{
		m_pCamera->BuildViewMatrix();
		m_FrameFrustum.FromMatrix(m_pCamera->GetProjectionMatrix() * m_pCamera->GetViewMatrix());

		BuildWorldMatrices();

		m_pFrameRenderList->UpdateCameraWorldPos(m_pCamera->GetPosition());
		m_pFrameRenderList->RemoveExcessObjects(m_FrameFrustum);
		m_pFrameRenderList->SortLists();
		InsertToRenderList(m_pFrameRenderList.get(), m_FrameFrustum);
	}

	void Scene::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
 		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Scene::OnWindowResize));
	}

	bool Scene::OnWindowResize(WindowResizeEvent& e)
	{
		if (m_pCamera)
			m_pCamera->UpdateProjectionMatrix(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));

		return false;
	}
}
