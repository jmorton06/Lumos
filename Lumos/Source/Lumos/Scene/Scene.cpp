#include "Precompiled.h"
#include "Scene.h"
#include "Core/OS/Input.h"
#include "Core/Application.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/Renderers/RenderGraph.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Utilities/TimeStep.h"
#include "Audio/AudioManager.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/LumosPhysicsEngine/BruteForceBroadphase.h"
#include "Physics/LumosPhysicsEngine/OctreeBroadphase.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Maths/Transform.h"
#include "Core/OS/FileSystem.h"
#include "Scene/Component/Components.h"
#include "Scripting/Lua/LuaScriptComponent.h"
#include "Scripting/Lua/LuaManager.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Light.h"
#include "Graphics/Model.h"
#include "Graphics/Environment.h"
#include "Scene/EntityManager.h"
#include "Scene/Component/SoundComponent.h"
#include "Scene/Component/ModelComponent.h"
#include "SceneGraph.h"

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <entt/entity/registry.hpp>
#include <sol/sol.hpp>

namespace Lumos
{
    Scene::Scene(const std::string& name)
        : m_SceneName(name)
        , m_ScreenWidth(0)
        , m_ScreenHeight(0)
    {
        m_EntityManager = CreateUniquePtr<EntityManager>(this);
        m_EntityManager->AddDependency<Physics3DComponent, Maths::Transform>();
        m_EntityManager->AddDependency<Physics2DComponent, Maths::Transform>();
        m_EntityManager->AddDependency<Camera, Maths::Transform>();
        m_EntityManager->AddDependency<Graphics::ModelComponent, Maths::Transform>();
        m_EntityManager->AddDependency<Graphics::Light, Maths::Transform>();
        m_EntityManager->AddDependency<Graphics::Sprite, Maths::Transform>();
        m_EntityManager->AddDependency<Graphics::AnimatedSprite, Maths::Transform>();

        m_SceneGraph = CreateUniquePtr<SceneGraph>();
        m_SceneGraph->Init(m_EntityManager->GetRegistry());
    }

    Scene::~Scene()
    {
        m_EntityManager->Clear();
    }

    entt::registry& Scene::GetRegistry()
    {
        return m_EntityManager->GetRegistry();
    }

    void Scene::OnInit()
    {
        LUMOS_PROFILE_FUNCTION();
        LuaManager::Get().GetState().set("registry", &m_EntityManager->GetRegistry());
        LuaManager::Get().GetState().set("scene", this);

        //Default physics setup
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.999f);
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateSharedPtr<OctreeBroadphase>(5, 5, Lumos::CreateSharedPtr<SortAndSweepBroadphase>()));

        LuaManager::Get().OnInit(this);
    }

    void Scene::OnCleanupScene()
    {
        LUMOS_PROFILE_FUNCTION();
        DeleteAllGameObjects();

        LuaManager::Get().GetState().collect_garbage();

        Application::Get().GetRenderGraph()->Reset();

        auto audioManager = Application::Get().GetSystem<AudioManager>();
        if(audioManager)
        {
            audioManager->ClearNodes();
        }
    };

    void Scene::DeleteAllGameObjects()
    {
        LUMOS_PROFILE_FUNCTION();
        m_EntityManager->Clear();
    }

    void Scene::OnUpdate(const TimeStep& timeStep)
    {
        LUMOS_PROFILE_FUNCTION();
        const Maths::Vector2 mousePos = Input::Get().GetMousePosition();

        auto defaultCameraControllerView = m_EntityManager->GetEntitiesWithType<DefaultCameraController>();

        if(!defaultCameraControllerView.Empty())
        {
            auto& cameraController = defaultCameraControllerView.Front().GetComponent<DefaultCameraController>();
            auto trans = defaultCameraControllerView.Front().TryGetComponent<Maths::Transform>();
            if(Application::Get().GetSceneActive() && trans && cameraController.GetController())
            {
                cameraController.GetController()->HandleMouse(*trans, timeStep.GetSeconds(), mousePos.x, mousePos.y);
                cameraController.GetController()->HandleKeyboard(*trans, timeStep.GetSeconds());
            }
        }

        m_SceneGraph->Update(m_EntityManager->GetRegistry());

        auto animatedSpriteView = m_EntityManager->GetEntitiesWithType<Graphics::AnimatedSprite>();

        for(auto entity : animatedSpriteView)
        {
            auto& animSprite = entity.GetComponent<Graphics::AnimatedSprite>();
            animSprite.OnUpdate(timeStep.GetSeconds());
        }
    }

    void Scene::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Scene::OnWindowResize));
    }

    bool Scene::OnWindowResize(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!Application::Get().GetSceneActive())
            return false;

        auto cameraView = m_EntityManager->GetRegistry().view<Camera>();
        if(!cameraView.empty())
        {
            m_EntityManager->GetRegistry().get<Camera>(cameraView.front()).SetAspectRatio(static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight()));
        }

        return false;
    }

    void Scene::SetScreenSize(uint32_t width, uint32_t height)
    {
        m_ScreenWidth = width;
        m_ScreenHeight = height;

        auto cameraView = m_EntityManager->GetRegistry().view<Camera>();
        if(!cameraView.empty())
        {
            m_EntityManager->GetRegistry().get<Camera>(cameraView.front()).SetAspectRatio(static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight));
        }
    }

#define ALL_COMPONENTSV1 Maths::Transform, NameComponent, ActiveComponent, Hierarchy, Camera, LuaScriptComponent, Graphics::Model, Graphics::Light, Physics3DComponent, Graphics::Environment, Graphics::Sprite, Physics2DComponent, DefaultCameraController

#define ALL_COMPONENTSV2 ALL_COMPONENTSV1, Graphics::AnimatedSprite
#define ALL_COMPONENTSV3 ALL_COMPONENTSV2, SoundComponent
#define ALL_COMPONENTSV4 ALL_COMPONENTSV3, Listener
#define ALL_COMPONENTSV5 ALL_COMPONENTSV4, IDComponent
#define ALL_COMPONENTSV6 ALL_COMPONENTSV5, Graphics::ModelComponent
#define ALL_COMPONENTSV7 ALL_COMPONENTSV6, AxisConstraintComponent

    void Scene::Serialise(const std::string& filePath, bool binary)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_LOG_INFO("Scene saved - {0}", filePath);
        std::string path = filePath;
        path += StringUtilities::RemoveSpaces(m_SceneName);

        m_SceneSerialisationVersion = SceneVersion;

        if(binary)
        {
            path += std::string(".bin");

            std::ofstream file(path, std::ios::binary);

            {
                // output finishes flushing its contents when it goes out of scope
                cereal::BinaryOutputArchive output { file };
                output(*this);
                entt::snapshot { m_EntityManager->GetRegistry() }.entities(output).component<ALL_COMPONENTSV7>(output);
            }
            file.close();
        }
        else
        {
            std::stringstream storage;
            path += std::string(".lsn");

            {
                // output finishes flushing its contents when it goes out of scope
                cereal::JSONOutputArchive output { storage };
                output(*this);
                entt::snapshot { m_EntityManager->GetRegistry() }.entities(output).component<ALL_COMPONENTSV7>(output);
            }
            FileSystem::WriteTextFile(path, storage.str());
        }
    }

    void Scene::Deserialise(const std::string& filePath, bool binary)
    {
        LUMOS_PROFILE_FUNCTION();
        m_EntityManager->Clear();
        m_SceneGraph->DisableOnConstruct(true, m_EntityManager->GetRegistry());
        std::string path = filePath;
        path += StringUtilities::RemoveSpaces(m_SceneName);

        if(binary)
        {
            path += std::string(".bin");

            if(!FileSystem::FileExists(path))
            {
                LUMOS_LOG_ERROR("No saved scene file found {0}", path);
                return;
            }

            try
            {
                std::ifstream file(path, std::ios::binary);
                cereal::BinaryInputArchive input(file);
                input(*this);
                if(m_SceneSerialisationVersion < 2)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV1>(input);
                else if(m_SceneSerialisationVersion == 3)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV2>(input);
                else if(m_SceneSerialisationVersion == 4)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV3>(input);
                else if(m_SceneSerialisationVersion == 5)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV4>(input);
                else if(m_SceneSerialisationVersion == 6)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV5>(input);
                else if(m_SceneSerialisationVersion == 7)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV6>(input);
                else if(m_SceneSerialisationVersion >= 8)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV7>(input);
                
                if(m_SceneSerialisationVersion < 6)
                {
                    m_EntityManager->GetRegistry().each([&](auto entity)
                        { m_EntityManager->GetRegistry().emplace<IDComponent>(entity, Random64::Rand(0, std::numeric_limits<uint64_t>::max())); });
                }

                if(m_SceneSerialisationVersion < 7)
                {
                    m_EntityManager->GetRegistry().each([&](auto entity)
                        {
                            Graphics::Model* model;
                            if(model = m_EntityManager->GetRegistry().try_get<Graphics::Model>(entity))
                            {
                                Graphics::Model* modelCopy = new Graphics::Model(*model);
                                m_EntityManager->GetRegistry().emplace<Graphics::ModelComponent>(entity, SharedPtr<Graphics::Model>(modelCopy));
                                m_EntityManager->GetRegistry().remove<Graphics::Model>(entity);
                            }
                        });
                }
            }
            catch(...)
            {
                LUMOS_LOG_ERROR("Failed to load scene - {0}", path);
            }
        }
        else
        {
            path += std::string(".lsn");

            if(!FileSystem::FileExists(path))
            {
                LUMOS_LOG_ERROR("No saved scene file found {0}", path);
                return;
            }
            try
            {
                std::string data = FileSystem::ReadTextFile(path);
                std::istringstream istr;
                istr.str(data);
                cereal::JSONInputArchive input(istr);
                input(*this);

                if(m_SceneSerialisationVersion < 2)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV1>(input);
                else if(m_SceneSerialisationVersion == 3)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV2>(input);
                else if(m_SceneSerialisationVersion == 4)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV3>(input);
                else if(m_SceneSerialisationVersion == 5)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV4>(input);
                else if(m_SceneSerialisationVersion == 6)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV5>(input);
                else if(m_SceneSerialisationVersion == 7)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV6>(input);
                else if(m_SceneSerialisationVersion >= 8)
                    entt::snapshot_loader { m_EntityManager->GetRegistry() }.entities(input).component<ALL_COMPONENTSV7>(input);

                if(m_SceneSerialisationVersion < 6)
                {
                    m_EntityManager->GetRegistry().each([&](auto entity)
                        { m_EntityManager->GetRegistry().emplace<IDComponent>(entity, Random64::Rand(0, std::numeric_limits<uint64_t>::max())); });
                }

                if(m_SceneSerialisationVersion < 7)
                {
                    m_EntityManager->GetRegistry().each([&](auto entity)
                        {
                            Graphics::Model* model;
                            if(model = m_EntityManager->GetRegistry().try_get<Graphics::Model>(entity))
                            {
                                Graphics::Model* modelCopy = new Graphics::Model(*model);
                                m_EntityManager->GetRegistry().emplace<Graphics::ModelComponent>(entity, SharedPtr<Graphics::Model>(modelCopy));
                                m_EntityManager->GetRegistry().remove<Graphics::Model>(entity);
                            }
                        });
                }
            }
            catch(...)
            {
                LUMOS_LOG_ERROR("Failed to load scene - {0}", path);
            }
        }

        m_SceneGraph->DisableOnConstruct(false, m_EntityManager->GetRegistry());
    }

    void Scene::UpdateSceneGraph()
    {
        LUMOS_PROFILE_FUNCTION();
        m_SceneGraph->Update(m_EntityManager->GetRegistry());
    }

    template <typename T>
    static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
    {
        if(registry.has<T>(src))
        {
            auto& srcComponent = registry.get<T>(src);
            registry.emplace_or_replace<T>(dst, srcComponent);
        }
    }

    template <typename ...Component>
    static void CopyEntity(entt::entity dst, entt::entity src, entt::registry& registry)
    {
        (CopyComponentIfExists<Component>(dst, src, registry), ...);
    }

    Entity Scene::CreateEntity()
    {
        return m_EntityManager->Create();
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        LUMOS_PROFILE_FUNCTION();
        return m_EntityManager->Create(name);
    }

    Entity Scene::GetEntityByUUID(uint64_t id)
    {
        LUMOS_PROFILE_FUNCTION();
        return m_EntityManager->GetEntityByUUID(id);
    }

    void Scene::DuplicateEntity(Entity entity)
    {
        LUMOS_PROFILE_FUNCTION();
        DuplicateEntity(entity, Entity(entt::null, nullptr));
    }

    void Scene::DuplicateEntity(Entity entity, Entity parent)
    {
        LUMOS_PROFILE_FUNCTION();
        m_SceneGraph->DisableOnConstruct(true, m_EntityManager->GetRegistry());

        Entity newEntity = m_EntityManager->Create();

        CopyEntity<ALL_COMPONENTSV7>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
        
        auto hierarchyComponent = newEntity.TryGetComponent<Hierarchy>();
        if(hierarchyComponent)
        {
            hierarchyComponent->m_First = entt::null;
            hierarchyComponent->m_Parent = entt::null;
            hierarchyComponent->m_Next = entt::null;
            hierarchyComponent->m_Prev = entt::null;
        }
        
        auto children = entity.GetChildren();
        std::vector<Entity> copiedChildren;
        
        for(auto child : children)
        {
            DuplicateEntity(child, newEntity);
        }

        if(parent)
            newEntity.SetParent(parent);
        
        m_SceneGraph->DisableOnConstruct(false, m_EntityManager->GetRegistry());
    }
}
