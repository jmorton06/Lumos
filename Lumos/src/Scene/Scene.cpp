#include "lmpch.h"
#include "Scene.h"
#include "Core/OS/Input.h"
#include "Core/Application.h"
#include "Scripting/LuaManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Utilities/TimeStep.h"
#include "Audio/AudioManager.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/LumosPhysicsEngine/Octree.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"

#include "Scripting/LuaManager.h"
#include "Graphics/Layers/LayerStack.h"
#include "Maths/Transform.h"
#include "Core/OS/FileSystem.h"
#include "Scene/Component/Components.h"
#include "Scripting/ScriptComponent.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Light.h"
#include "Graphics/Environment.h"
#include "Scene/EntityManager.h"

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <entt/entity/registry.hpp>
//#include <cereal/cereal.hpp>
#include <sol/sol.hpp>

namespace Lumos
{
	Scene::Scene(const std::string& friendly_name)
		: m_SceneName(friendly_name)
		, m_SceneBoundingRadius(0)
		, m_ScreenWidth(0)
		, m_ScreenHeight(0)
	{
		m_LayerStack = new LayerStack();
		m_EntityManager = CreateUniqueRef<EntityManager>(this);
	}

	Scene::~Scene()
	{
		if(m_LuaEnv)
		{
			sol::protected_function onDestroyFunc = (*m_LuaEnv)["OnDestroy"];

			if(onDestroyFunc)
			{
				sol::protected_function_result result = onDestroyFunc.call();
				if(!result.valid())
				{
					sol::error err = result;
					Debug::Log::Error("Failed to Execute Scene Lua OnDestroy function");
					Debug::Log::Error("Error : {0}", err.what());
				}
			}
		}

		delete m_LayerStack;

		m_EntityManager->Clear();
	}

	entt::registry& Scene::GetRegistry()
	{
		return m_EntityManager->GetRegistry();
	}

	void Scene::OnInit()
	{
		//m_EntityManager->AddDependency<Physics3DComponent, Maths::Transform>();
		//m_EntityManager->AddDependency<Physics2DComponent, Maths::Transform>();
		//m_EntityManager->AddDependency<MeshComponent, Maths::Transform>();

		LuaManager::Get().GetState().set("registry", &m_EntityManager->GetRegistry());
		LuaManager::Get().GetState().set("scene", this);

		m_CurrentScene = true;

		std::string Configuration;
		std::string Platform;
		std::string RenderAPI;
		std::string dash = " - ";

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

		switch(Graphics::GraphicsContext::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case Graphics::RenderAPI::OPENGL:
			RenderAPI = "OpenGL";
			break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#	if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
		case Graphics::RenderAPI::VULKAN:
			RenderAPI = "Vulkan ( MoltenVK )";
			break;
#	else
		case Graphics::RenderAPI::VULKAN:
			RenderAPI = "Vulkan";
			break;
#	endif
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
		case DIRECT3D:
			RenderAPI = "Direct3D";
			break;
#endif
		default:
			break;
		}

		std::stringstream Title;
		Title << Platform << dash << RenderAPI << dash << Configuration << dash << m_SceneName << dash << Application::Get().GetWindow()->GetTitle();

		Application::Get().GetWindow()->SetWindowTitle(Title.str());

		//Default physics setup
		Application::Get().GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
		Application::Get().GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
		Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));
		m_SceneBoundingRadius = 400.0f; //Default scene radius of 400m

		m_SceneGraph.Init(m_EntityManager->GetRegistry());
	}

	void Scene::OnCleanupScene()
	{
		if(m_LuaEnv)
		{
			sol::protected_function onCleanupFunc = (*m_LuaEnv)["OnCleanUp"];

			if(onCleanupFunc)
			{
				sol::protected_function_result result = onCleanupFunc.call();
				if(!result.valid())
				{
					sol::error err = result;
					Debug::Log::Error("Failed to Execute Scene Lua OnCleanUp function");
					Debug::Log::Error("Error : {0}", err.what());
				}
			}
		}

		//m_LuaEnv = NULL;

		LuaManager::Get().GetState().collect_garbage();

		m_LayerStack->Clear();

		DeleteAllGameObjects();

		Application::Get().GetRenderManager()->Reset();

		auto audioManager = Application::Get().GetSystem<AudioManager>();
		if(audioManager)
		{
			audioManager->ClearNodes();
		}

		m_CurrentScene = false;
	};

	void Scene::DeleteAllGameObjects()
	{
		m_EntityManager->Clear();
	}

	void Scene::OnUpdate(const TimeStep& timeStep)
	{
		const Maths::Vector2 mousePos = Input::GetInput()->GetMousePosition();

		auto cameraView = m_EntityManager->GetRegistry().view<Camera>();
		if(!cameraView.empty())
		{
			Camera& camera = m_EntityManager->GetRegistry().get<Camera>(cameraView.front());

			auto cameraController = camera.GetController();

			if(cameraController && Application::Get().GetSceneActive())
			{
				cameraController->HandleMouse(&camera, timeStep.GetMillis(), mousePos.x, mousePos.y);
				cameraController->HandleKeyboard(&camera, timeStep.GetMillis());
			}
		}

		m_SceneGraph.Update(m_EntityManager->GetRegistry());

		if(m_LuaUpdateFunction)
		{
			sol::protected_function_result result = m_LuaUpdateFunction->call(timeStep.GetElapsedMillis());
			if(!result.valid())
			{
				sol::error err = result;
				Debug::Log::Error("Failed to Execute Scene Lua update");
				Debug::Log::Error("Error : {0}", err.what());
			}
		}
	}

	void Scene::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Scene::OnWindowResize));
	}

	bool Scene::OnWindowResize(WindowResizeEvent& e)
	{
		if(!Application::Get().GetSceneActive())
			return false;

		auto cameraView = m_EntityManager->GetRegistry().view<Camera>();
		if(!cameraView.empty())
		{
			m_EntityManager->GetRegistry().get<Camera>(cameraView.front()).SetAspectRatio(static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight()));
		}

		return false;
	}

	void Scene::LoadLuaScene(const std::string& filePath)
	{
		m_LuaEnv = CreateRef<sol::environment>(LuaManager::Get().GetState(), sol::create, LuaManager::Get().GetState().globals());
		(*m_LuaEnv)["scene"] = this;
		(*m_LuaEnv)["reg"] = &m_EntityManager->GetRegistry();

		std::string physicalPath;
		if(!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
		{
			Debug::Log::Error("Failed to Load Lua script {0}", filePath);
			return;
		}

		auto loadFileResult = LuaManager::Get().GetState().script_file(physicalPath, *m_LuaEnv, sol::script_pass_on_error);
		if(!loadFileResult.valid())
		{
			sol::error err = loadFileResult;
			Debug::Log::Error("Failed to Execute Lua script {0}", physicalPath);
			Debug::Log::Error("Error : {0}", err.what());
		}

		sol::protected_function onInitFunc = (*m_LuaEnv)["OnInit"];

		if(onInitFunc)
		{
			sol::protected_function_result result = onInitFunc.call();
			if(!result.valid())
			{
				sol::error err = result;
				Debug::Log::Error("Failed to Execute Scene Lua Init function");
				Debug::Log::Error("Error : {0}", err.what());
			}
		}

		m_LuaUpdateFunction = CreateRef<sol::protected_function>((*m_LuaEnv)["OnUpdate"]);
	}

	Scene* Scene::LoadFromLua(const std::string& filePath)
	{
		Scene* scene = new Scene("");
		scene->LoadLuaScene(filePath);

		return scene;
	}

	void Scene::PushLayer(Layer* layer, bool overlay)
	{
		if(overlay)
			m_LayerStack->PushOverlay(layer);
		else
			m_LayerStack->PushLayer(layer);

		layer->OnAttach();
	}

#define ALL_COMPONENTS Maths::Transform, NameComponent, ActiveComponent, Hierarchy, Camera, ScriptComponent, MaterialComponent, MeshComponent, Graphics::Light, Physics3DComponent, Graphics::Environment, Graphics::Sprite, Physics2DComponent
	void Scene::Serialise(const std::string& filePath, bool binary)
	{
		std::string path = filePath;
		path += RemoveSpaces(m_SceneName);
		if(binary)
		{
			path += std::string(".bin");

			std::ofstream file(path, std::ios::binary);

			{
				// output finishes flushing its contents when it goes out of scope
				cereal::BinaryOutputArchive output{file};
				entt::snapshot{m_EntityManager->GetRegistry()}.entities(output).component<ALL_COMPONENTS>(output);
				output(*this);
			}
			file.close();
		}
		else
		{
			std::stringstream storage;
			path += std::string(".lsn");

			{
				// output finishes flushing its contents when it goes out of scope
				cereal::JSONOutputArchive output{storage};
				entt::snapshot{m_EntityManager->GetRegistry()}.entities(output).component<ALL_COMPONENTS>(output);
				output(*this);
			}
			FileSystem::WriteTextFile(path, storage.str());
		}
	}

	void Scene::Deserialise(const std::string& filePath, bool binary)
	{
		m_EntityManager->Clear();

		std::string path = filePath;
		path += RemoveSpaces(m_SceneName);

		if(binary)
		{
			path += std::string(".bin");

			if(!FileSystem::FileExists(path))
			{
				Lumos::Debug::Log::Error("No saved scene file found");
				return;
			}

			std::ifstream file(path, std::ios::binary);
			cereal::BinaryInputArchive input(file);
			entt::snapshot_loader{m_EntityManager->GetRegistry()}.entities(input).component<ALL_COMPONENTS>(input); //continuous_loader
			input(*this);
		}
		else
		{
			path += std::string(".lsn");

			if(!FileSystem::FileExists(path))
			{
				Lumos::Debug::Log::Error("No saved scene file found");
				return;
			}

			std::string data = FileSystem::ReadTextFile(path);
			std::istringstream istr;
			istr.str(data);
			cereal::JSONInputArchive input(istr);
			entt::snapshot_loader{m_EntityManager->GetRegistry()}.entities(input).component<ALL_COMPONENTS>(input); //continuous_loader
			input(*this);
		}
	}

	void Scene::UpdateSceneGraph()
	{
		m_SceneGraph.Update(m_EntityManager->GetRegistry());
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
	{
		if(registry.has<T>(src))
		{
			auto& srcComponent = registry.get<T>(src);
			registry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		Entity newEntity = m_EntityManager->Create();

		CopyComponentIfExists<Maths::Transform>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<MeshComponent>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<ScriptComponent>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<Camera>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<Graphics::Sprite>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<RigidBody2D>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<RigidBody3D>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<Graphics::Light>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<Material>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<SoundComponent>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
		CopyComponentIfExists<Graphics::Environment>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
	}
}
