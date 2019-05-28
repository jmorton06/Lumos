#include "LM.h"
#include "Scene.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "System/String.h"
#include "Audio/AudioManager.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/LumosPhysicsEngine/Octree.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/Light.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Utilities/TimeStep.h"
#include "App/Input.h"
#include "App/Application.h"
#include "Graphics/RenderManager.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/Camera/Camera.h"
#include "Entity/Component/TransformComponent.h"

namespace lumos
{
	Scene::Scene(const String& friendly_name)
		: m_SceneName(friendly_name), m_pCamera(nullptr), m_EnvironmentMap(nullptr), m_SceneBoundingRadius(0),
		  m_DebugDrawFlags(0), m_ScreenWidth(0),
		  m_ScreenHeight(0),
		  m_DrawDebugData(false),
          m_RootEntity(std::make_shared<Entity>("Root Node"))
    
	{
	}

    Scene::~Scene()
    {
        DeleteAllGameObjects();

        if (m_ParticleManager)
        {
            delete m_ParticleManager;
            m_ParticleManager = nullptr;
        }
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

		switch (graphics::GraphicsContext::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case graphics::RenderAPI::OPENGL: RenderAPI = "OpenGL"; break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
		case graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan ( MoltenVK )"; break;
#else
		case graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan"; break;
#endif
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
		case DIRECT3D: RenderAPI = "Direct3D"; break;
#endif
		}

		std::stringstream Title;
		Title << Platform << dash << RenderAPI << dash << Configuration << dash << m_SceneName << dash << Application::Instance()->GetWindow()->GetTitle();

		Application::Instance()->GetWindow()->SetWindowTitle(Title.str());

		//Default physics setup
		LumosPhysicsEngine::Instance()->SetDampingFactor(0.998f);
		LumosPhysicsEngine::Instance()->SetIntegrationType(INTEGRATION_RUNGE_KUTTA_4);
		LumosPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 5, std::make_shared<SortAndSweepBroadphase>()));
		SetDebugDrawFlags(DEBUGDRAW_FLAGS_COLLISIONVOLUMES
			| DEBUGDRAW_FLAGS_AABB
			| DEBUGDRAW_FLAGS_COLLISIONNORMALS
			| DEBUGDRAW_FLAGS_BROADPHASE
			| DEBUGDRAW_FLAGS_CONSTRAINT
		);
		m_SceneBoundingRadius = 400.0f; //Default scene radius of 400m

		m_pFrameRenderList = std::make_unique<RenderList>();

		if (!RenderList::AllocateNewRenderList(m_pFrameRenderList.get(), true))
		{
			LUMOS_CORE_ERROR("Unable to allocate scene render list! - Try using less shadow maps");
		}
	}

	void Scene::OnCleanupScene()
	{
        m_pFrameRenderList.reset();

		DeleteAllGameObjects();

		auto layerStack = Application::Instance()->GetLayerStack();
		for(auto& layer : m_SceneLayers)
		{
			layerStack->PopLayer(layer);
		}

		m_SceneLayers.clear();

		Input::GetInput().Reset();

		Application::Instance()->GetRenderManager()->Reset();
		Application::Instance()->GetAudioManager()->ClearNodes();

		m_CurrentScene = false;
	};

	void Scene::AddEntity(std::shared_ptr<Entity>& game_object)
	{
		if (game_object->GetComponent<Physics3DComponent>())
			game_object->GetComponent<Physics3DComponent>()->m_PhysicsObject->AutoResizeBoundingBox();

		m_RootEntity->AddChildObject(game_object);
	}


	void Scene::DeleteAllGameObjects()
	{
		m_RootEntity->GetChildren().clear();
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
		BuildLightList();

		std::function<void(std::shared_ptr<Entity>)> per_object_func = [&](std::shared_ptr<Entity> obj)
		{
			obj->OnUpdateObject(timeStep->GetSeconds());

			for(auto child : obj->GetChildren())
				per_object_func(child);
		};

		per_object_func(m_RootEntity);
	}

	void Scene::BuildWorldMatrices()
	{
		std::function<void(std::shared_ptr<Entity>)> per_object_func = [&](std::shared_ptr<Entity> obj)
		{
			Physics3DComponent* physicsComponent = obj->GetComponent<Physics3DComponent>();
			TransformComponent* transformComponent = obj->GetComponent<TransformComponent>();
			if (physicsComponent)
			{
				if (transformComponent)
					transformComponent->m_Transform.GetWorldMatrix() = physicsComponent->m_PhysicsObject->GetWorldSpaceTransform() * transformComponent->m_Transform.GetLocalMatrix();
			}
			else
			{
				if (transformComponent)
					transformComponent->m_Transform.GetWorldMatrix() = transformComponent->m_Transform.GetLocalMatrix();
			}

			for (auto child : obj->GetChildren())
				per_object_func(child);
		};

		per_object_func(m_RootEntity);
	}

	void Scene::DebugRender()
	{
		if (m_DebugDrawFlags & DEBUGDRAW_FLAGS_ENTITY_COMPONENTS)
		{
			std::function<void(std::shared_ptr<Entity>)> per_object_func = [&](std::shared_ptr<Entity> obj)
			{
				obj->DebugDraw(m_DebugDrawFlags);

				for (auto child : obj->GetChildren())
					per_object_func(child);
			};

			per_object_func(m_RootEntity);
		}
	}

	void Scene::InsertToRenderList(RenderList* list, const maths::Frustum& frustum) const
	{
		std::function<void(std::shared_ptr<Entity>)> per_object_func = [&](std::shared_ptr<Entity> obj)
		{
			auto meshComponent = obj->GetComponent<MeshComponent>();
			if (meshComponent)
			{
				bool inside = frustum.InsideFrustum(obj->GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix().GetPositionVector(), obj->GetBoundingRadius());

				if (inside)
				{
					//Check to see if the object is already listed or not
					if (!(list->BitMask() & obj->GetFrustumCullFlags()))
					{
						list->InsertObject(obj);
					}
				}
			}

			for (auto child : obj->GetChildren())
				per_object_func(child);
		};

		per_object_func(m_RootEntity);
	}

	void Scene::BuildFrameRenderList()
	{
        if(!m_pCamera)
            return;
		m_pCamera->BuildViewMatrix();
		m_FrameFrustum.FromMatrix(m_pCamera->GetProjectionMatrix() * m_pCamera->GetViewMatrix());

		BuildWorldMatrices();

		m_pFrameRenderList->UpdateCameraWorldPos(m_pCamera->GetPosition());
		m_pFrameRenderList->RemoveExcessObjects(m_FrameFrustum);
		m_pFrameRenderList->SortLists();
		InsertToRenderList(m_pFrameRenderList.get(), m_FrameFrustum);
	}

	void Scene::BuildLightList()
	{
		m_LightList.clear();

		std::function<void(std::shared_ptr<Entity>)> per_object_func = [&](std::shared_ptr<Entity> obj)
		{
			auto lightComponent = obj->GetComponent<LightComponent>();
			if (lightComponent)
			{
				m_LightList.emplace_back(lightComponent->GetLight());
			}

			for (auto child : obj->GetChildren())
				per_object_func(child);
		};

		per_object_func(m_RootEntity);
	}

	void Scene::IterateEntities(const std::function<void(std::shared_ptr<Entity>)>& per_object_func)
	{
		std::function<void(std::shared_ptr<Entity>)> per_object_func2 = [&](std::shared_ptr<Entity> obj)
		{
			if (obj->IsActive())
			{
				per_object_func(obj);

				for (auto child : obj->GetChildren())
					per_object_func2(child);
			}
		};

		per_object_func2(m_RootEntity);
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
