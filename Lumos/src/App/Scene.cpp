#include "lmpch.h"
#include "Scene.h"
#include "Core/OS/Input.h"
#include "Application.h"

#include "Graphics/ParticleManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/Light.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Camera/Camera.h"
#include "Utilities/TimeStep.h"
#include "ECS/EntityManager.h"
#include "ECS/Component/Components.h"
#include "Audio/AudioManager.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/LumosPhysicsEngine/Octree.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
	Scene::Scene(const String& friendly_name) :
		m_SceneName(friendly_name),
		m_pCamera(nullptr), 
		m_EnvironmentMap(nullptr), 
		m_SceneBoundingRadius(0),
		m_ScreenWidth(0),
		m_ScreenHeight(0),
		m_RootEntity(nullptr)
	{
	}

    Scene::~Scene()
    {
        DeleteAllGameObjects();
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

		switch (Graphics::GraphicsContext::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case Graphics::RenderAPI::OPENGL: RenderAPI = "OpenGL"; break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
		case Graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan ( MoltenVK )"; break;
#else
		case Graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan"; break;
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
		Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
		Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
		Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

		m_SceneBoundingRadius = 400.0f; //Default scene radius of 400m

		m_pFrameRenderList = CreateScope<RenderList>();

		if (!RenderList::AllocateNewRenderList(m_pFrameRenderList.get(), true))
		{
			LUMOS_LOG_CRITICAL("Unable to allocate scene render list! - Try using less shadow maps");
		}

		m_RootEntity = EntityManager::Instance()->CreateEntity("Root");
	}

	void Scene::OnCleanupScene()
	{
        m_pFrameRenderList.reset();

		DeleteAllGameObjects();

		Application::Instance()->GetRenderManager()->Reset();
		Application::Instance()->GetSystem<AudioManager>()->ClearNodes();

		m_CurrentScene = false;
	};

	void Scene::AddEntity(Entity* game_object)
	{
		m_RootEntity->AddChild(game_object);
	}


	void Scene::DeleteAllGameObjects()
	{
		EntityManager::Instance()->Clear();
	}

	void Scene::OnUpdate(TimeStep* timeStep)
	{
		const Maths::Vector2 mousePos = Input::GetInput()->GetMousePosition();

		if(m_pCamera && Application::Instance()->GetSceneActive())
		{
			m_pCamera->HandleMouse(timeStep->GetMillis(), mousePos.GetX(), mousePos.GetY());
			m_pCamera->HandleKeyboard(timeStep->GetMillis());
			m_pCamera->BuildViewMatrix();
		}

		BuildFrameRenderList();

		std::function<void(Entity*)> per_object_func = [&](Entity* obj)
		{
			obj->OnUpdateObject(timeStep->GetSeconds());

			for(auto child : obj->GetChildren())
				per_object_func(child);
		};

		per_object_func(m_RootEntity);

		ComponentManager::Instance()->OnUpdate();
	}

	void Scene::InsertToRenderList(RenderList* list, const Maths::Frustum& frustum) const
	{
		std::function<void(Entity*)> per_object_func = [&](Entity* obj)
		{
			if (obj->ActiveInHierarchy())
			{
				auto meshComponent = obj->GetComponent<MeshComponent>();
				if (meshComponent &&/* meshComponent->GetActive() &&*/ meshComponent->GetMesh())
				{
					auto transform = obj->GetComponent<Maths::Transform>();

					float maxScaling = 0.0f;
					maxScaling = Maths::Max(transform->GetWorldMatrix().GetScaling().GetX(), maxScaling);
					maxScaling = Maths::Max(transform->GetWorldMatrix().GetScaling().GetY(), maxScaling);
					maxScaling = Maths::Max(transform->GetWorldMatrix().GetScaling().GetZ(), maxScaling);

					bool inside = frustum.InsideFrustum(transform->GetWorldMatrix().GetPositionVector(), maxScaling * meshComponent->GetMesh()->GetBoundingSphere()->SphereRadius());// maxScaling * obj->GetBoundingRadius());

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
			}
		};

		per_object_func(m_RootEntity);
	}

	void Scene::BuildFrameRenderList()
	{
        if(!m_pCamera)
            return;
		m_pCamera->BuildViewMatrix();
		m_FrameFrustum.FromMatrix(m_pCamera->GetProjectionMatrix() * m_pCamera->GetViewMatrix());

		//BuildWorldMatrices();

		m_pFrameRenderList->UpdateCameraWorldPos(m_pCamera->GetPosition());
		m_pFrameRenderList->RemoveExcessObjects(m_FrameFrustum);
		m_pFrameRenderList->SortLists();
		InsertToRenderList(m_pFrameRenderList.get(), m_FrameFrustum);
	}

	void Scene::IterateEntities(const std::function<void(Entity*)>& per_object_func)
	{
		std::function<void(Entity*)> per_object_func2 = [&](Entity* obj)
		{
			if (obj->ActiveInHierarchy())
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
	nlohmann::json Scene::Serialise()
	{
		nlohmann::json output;
		output["typeID"] = LUMOS_TYPENAME(Scene);
		output["name"] = m_SceneName;

		nlohmann::json serialisedEntities = nlohmann::json::array_t();
		auto& entities = EntityManager::Instance()->GetEntities();

		for (int i = 0; i < entities.size(); ++i)
			serialisedEntities.push_back(entities[i]->Serialise());

		output["entities"] = serialisedEntities;

		return output;
	}

	void Scene::Deserialise(nlohmann::json & data)
	{
		m_SceneName = data["name"];

		nlohmann::json::array_t entities = data["entities"];

		for (int i = 0; i < entities.size(); i++)
		{
			auto entity = EntityManager::Instance()->CreateEntity();
			entity->Deserialise(entities[i]);
		}
	}
}
