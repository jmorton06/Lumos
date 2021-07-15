#include "Precompiled.h"
#include "LuaManager.h"
#include "Maths/Transform.h"
#include "Core/OS/Window.h"
#include "Core/VFS.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Core/OS/Input.h"
#include "Scene/SceneManager.h"
#include "LuaScriptComponent.h"
#include "Scene/SceneGraph.h"
#include "Graphics/Camera/ThirdPersonCamera.h"

#include "Scene/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/Camera2D.h"

#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/Model.h"
#include "Maths/Random.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/EntityFactory.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"

#include "ImGuiLua.h"
#include "PhysicsLua.h"
#include "MathsLua.h"

#include <imgui/imgui.h>
#include <Tracy/TracyLua.hpp>

#ifdef CUSTOM_SMART_PTR
namespace sol
{
    template <typename T>
    struct unique_usertype_traits<Lumos::SharedRef<T>>
    {
        typedef T type;
        typedef Lumos::SharedRef<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };

    template <typename T>
    struct unique_usertype_traits<Lumos::UniqueRef<T>>
    {
        typedef T type;
        typedef Lumos::UniqueRef<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };

    template <typename T>
    struct unique_usertype_traits<Lumos::WeakRef<T>>
    {
        typedef T type;
        typedef Lumos::WeakRef<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };
}

#endif

namespace Lumos
{
    LuaManager::LuaManager()
        : m_State(nullptr)
    {
    }

    void LuaManager::OnInit()
    {
        LUMOS_PROFILE_FUNCTION();
        m_State.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);
        tracy::LuaRegister(m_State.lua_state());

        BindAppLua(m_State);
        BindInputLua(m_State);
        BindMathsLua(m_State);
        BindImGuiLua(m_State);
        BindECSLua(m_State);
        BindLogLua(m_State);
        BindSceneLua(m_State);
        BindPhysicsLua(m_State);
    }

    LuaManager::~LuaManager()
    {
    }

    void LuaManager::OnInit(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry = scene->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.SetThisComponent();
            luaScript.OnInit();
        }
    }

    void LuaManager::OnUpdate(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry = scene->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        float dt = Engine::Get().GetTimeStep().GetElapsedMillis();

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.OnUpdate(dt);
        }
    }

    entt::entity GetEntityByName(entt::registry& registry, const std::string& name)
    {
        LUMOS_PROFILE_FUNCTION();
        entt::entity e = entt::null;
        registry.view<NameComponent>().each([&](const entt::entity& entity, const NameComponent& component)
            {
                if(name == component.name)
                {
                    e = entity;
                }
            });

        return e;
    }

    void LuaManager::BindLogLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        auto log = state.create_table("Log");

        log.set_function("Trace", [&](sol::this_state s, std::string_view message)
            { LUMOS_LOG_TRACE(message); });

        log.set_function("Info", [&](sol::this_state s, std::string_view message)
            { LUMOS_LOG_TRACE(message); });

        log.set_function("Warn", [&](sol::this_state s, std::string_view message)
            { LUMOS_LOG_WARN(message); });

        log.set_function("Error", [&](sol::this_state s, std::string_view message)
            { LUMOS_LOG_ERROR(message); });

        log.set_function("Critical", [&](sol::this_state s, std::string_view message)
            { LUMOS_LOG_CRITICAL(message); });
    }

    void LuaManager::BindInputLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        auto input = state["Input"].get_or_create<sol::table>();

        input.set_function("GetKeyPressed", [](Lumos::InputCode::Key key) -> bool
            { return Input::Get().GetKeyPressed(key); });

        input.set_function("GetKeyHeld", [](Lumos::InputCode::Key key) -> bool
            { return Input::Get().GetKeyHeld(key); });

        input.set_function("GetMouseClicked", [](Lumos::InputCode::MouseKey key) -> bool
            { return Input::Get().GetMouseClicked(key); });

        input.set_function("GetMouseHeld", [](Lumos::InputCode::MouseKey key) -> bool
            { return Input::Get().GetMouseHeld(key); });

        input.set_function("GetMousePosition", []() -> Maths::Vector2
            { return Input::Get().GetMousePosition(); });

        input.set_function("GetScrollOffset", []() -> float
            { return Input::Get().GetScrollOffset(); });

        std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::Key>> keyItems = {
            { "A", Lumos::InputCode::Key::A },
            { "B", Lumos::InputCode::Key::B },
            { "C", Lumos::InputCode::Key::C },
            { "D", Lumos::InputCode::Key::D },
            { "E", Lumos::InputCode::Key::E },
            { "F", Lumos::InputCode::Key::F },
            { "H", Lumos::InputCode::Key::G },
            { "G", Lumos::InputCode::Key::H },
            { "I", Lumos::InputCode::Key::I },
            { "J", Lumos::InputCode::Key::J },
            { "K", Lumos::InputCode::Key::K },
            { "L", Lumos::InputCode::Key::L },
            { "M", Lumos::InputCode::Key::M },
            { "N", Lumos::InputCode::Key::N },
            { "O", Lumos::InputCode::Key::O },
            { "P", Lumos::InputCode::Key::P },
            { "Q", Lumos::InputCode::Key::Q },
            { "R", Lumos::InputCode::Key::R },
            { "S", Lumos::InputCode::Key::S },
            { "T", Lumos::InputCode::Key::T },
            { "U", Lumos::InputCode::Key::U },
            { "V", Lumos::InputCode::Key::V },
            { "W", Lumos::InputCode::Key::W },
            { "X", Lumos::InputCode::Key::X },
            { "Y", Lumos::InputCode::Key::Y },
            { "Z", Lumos::InputCode::Key::Z },
            //{ "UNKOWN", Lumos::InputCode::Key::Unknown },
            { "Space", Lumos::InputCode::Key::Space },
            { "Escape", Lumos::InputCode::Key::Escape },
            { "APOSTROPHE", Lumos::InputCode::Key::Apostrophe },
            { "Comma", Lumos::InputCode::Key::Comma },
            { "MINUS", Lumos::InputCode::Key::Minus },
            { "PERIOD", Lumos::InputCode::Key::Period },
            { "SLASH", Lumos::InputCode::Key::Slash },
            { "SEMICOLON", Lumos::InputCode::Key::Semicolon },
            { "EQUAL", Lumos::InputCode::Key::Equal },
            { "LEFT_BRACKET", Lumos::InputCode::Key::LeftBracket },
            { "BACKSLASH", Lumos::InputCode::Key::Backslash },
            { "RIGHT_BRACKET", Lumos::InputCode::Key::RightBracket },
            //{ "BACK_TICK", Lumos::InputCode::Key::BackTick },
            { "Enter", Lumos::InputCode::Key::Enter },
            { "Tab", Lumos::InputCode::Key::Tab },
            { "Backspace", Lumos::InputCode::Key::Backspace },
            { "Insert", Lumos::InputCode::Key::Insert },
            { "Delete", Lumos::InputCode::Key::Delete },
            { "Right", Lumos::InputCode::Key::Right },
            { "Left", Lumos::InputCode::Key::Left },
            { "Down", Lumos::InputCode::Key::Down },
            { "Up", Lumos::InputCode::Key::Up },
            { "PageUp", Lumos::InputCode::Key::PageUp },
            { "PageDown", Lumos::InputCode::Key::PageDown },
            { "Home", Lumos::InputCode::Key::Home },
            { "End", Lumos::InputCode::Key::End },
            { "CAPS_LOCK", Lumos::InputCode::Key::CapsLock },
            { "SCROLL_LOCK", Lumos::InputCode::Key::ScrollLock },
            { "NumLock", Lumos::InputCode::Key::NumLock },
            { "PrintScreen", Lumos::InputCode::Key::PrintScreen },
            { "Pasue", Lumos::InputCode::Key::Pause },
            { "LeftShift", Lumos::InputCode::Key::LeftShift },
            { "LeftControl", Lumos::InputCode::Key::LeftControl },
            { "LEFT_ALT", Lumos::InputCode::Key::LeftAlt },
            { "LEFT_SUPER", Lumos::InputCode::Key::LeftSuper },
            { "RightShift", Lumos::InputCode::Key::RightShift },
            { "RightControl", Lumos::InputCode::Key::RightControl },
            { "RIGHT_ALT", Lumos::InputCode::Key::RightAlt },
            { "RIGHT_SUPER", Lumos::InputCode::Key::RightSuper },
            { "Menu", Lumos::InputCode::Key::Menu },
            { "F1", Lumos::InputCode::Key::F1 },
            { "F2", Lumos::InputCode::Key::F2 },
            { "F3", Lumos::InputCode::Key::F3 },
            { "F4", Lumos::InputCode::Key::F4 },
            { "F5", Lumos::InputCode::Key::F5 },
            { "F6", Lumos::InputCode::Key::F6 },
            { "F7", Lumos::InputCode::Key::F7 },
            { "F8", Lumos::InputCode::Key::F8 },
            { "F9", Lumos::InputCode::Key::F9 },
            { "F10", Lumos::InputCode::Key::F10 },
            { "F11", Lumos::InputCode::Key::F11 },
            { "F12", Lumos::InputCode::Key::F12 },
            { "Keypad0", Lumos::InputCode::Key::D0 },
            { "Keypad1", Lumos::InputCode::Key::D1 },
            { "Keypad2", Lumos::InputCode::Key::D2 },
            { "Keypad3", Lumos::InputCode::Key::D3 },
            { "Keypad4", Lumos::InputCode::Key::D4 },
            { "Keypad5", Lumos::InputCode::Key::D5 },
            { "Keypad6", Lumos::InputCode::Key::D6 },
            { "Keypad7", Lumos::InputCode::Key::D7 },
            { "Keypad8", Lumos::InputCode::Key::D8 },
            { "Keypad9", Lumos::InputCode::Key::D9 },
            { "Decimal", Lumos::InputCode::Key::Period },
            { "Divide", Lumos::InputCode::Key::Slash },
            { "Multiply", Lumos::InputCode::Key::KPMultiply },
            { "Subtract", Lumos::InputCode::Key::Minus },
            { "Add", Lumos::InputCode::Key::KPAdd },
            { "KP_EQUAL", Lumos::InputCode::Key::KPEqual }
        };
        state.new_enum<Lumos::InputCode::Key, false>("Key", keyItems); // false makes it read/write in Lua, but its faster

        std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::MouseKey>> mouseItems = {
            { "Left", Lumos::InputCode::MouseKey::ButtonLeft },
            { "Right", Lumos::InputCode::MouseKey::ButtonRight },
            { "Middle", Lumos::InputCode::MouseKey::ButtonMiddle },
        };
        state.new_enum<Lumos::InputCode::MouseKey, false>("MouseButton", mouseItems);
    }

    SharedRef<Graphics::Texture2D> LoadTexture(const std::string& name, const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        return SharedRef<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
    }

    SharedRef<Graphics::Texture2D> LoadTextureWithParams(const std::string& name, const std::string& path, Lumos::Graphics::TextureFilter filter, Lumos::Graphics::TextureWrap wrapMode)
    {
        LUMOS_PROFILE_FUNCTION();
        return SharedRef<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path, Graphics::TextureParameters(filter, filter, wrapMode)));
    }

    void LuaManager::BindECSLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();

        sol::usertype<entt::registry> enttRegistry = state.new_usertype<entt::registry>("enttRegistry");

        sol::usertype<Entity> entityType = state.new_usertype<Entity>("Entity", sol::constructors<sol::types<entt::entity, Scene*>>());
        sol::usertype<EntityManager> entityManagerType = state.new_usertype<EntityManager>("EntityManager");
        entityManagerType.set_function("Create", static_cast<Entity (EntityManager::*)()>(&EntityManager::Create));
        entityManagerType.set_function("GetRegistry", &EntityManager::GetRegistry);

        entityType.set_function("Valid", &Entity::Valid);
        entityType.set_function("Destroy", &Entity::Destroy);
        entityType.set_function("SetParent", &Entity::SetParent);
        entityType.set_function("GetParent", &Entity::GetParent);
        entityType.set_function("IsParent", &Entity::IsParent);
        entityType.set_function("GetChildren", &Entity::GetChildren);
        entityType.set_function("SetActive", &Entity::SetActive);
        entityType.set_function("Active", &Entity::Active);

        state.set_function("GetEntityByName", &GetEntityByName);

        state.set_function("AddPyramidEntity", &EntityFactory::AddPyramid);
        state.set_function("AddSphereEntity", &EntityFactory::AddSphere);
        state.set_function("AddLightCubeEntity", &EntityFactory::AddLightCube);

        sol::usertype<NameComponent> nameComponent_type = state.new_usertype<NameComponent>("NameComponent");
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS(state, NameComponent, static_cast<NameComponent& (Entity::*)()>(&Entity::AddComponent<NameComponent>));

        sol::usertype<LuaScriptComponent> script_type = state.new_usertype<LuaScriptComponent>("LuaScriptComponent", sol::constructors<sol::types<std::string, Scene*>>());
        REGISTER_COMPONENT_WITH_ECS(state, LuaScriptComponent, static_cast<LuaScriptComponent& (Entity::*)(std::string&&, Scene * &&)>(&Entity::AddComponent<LuaScriptComponent, std::string, Scene*>));
        script_type.set_function("GetCurrentEntity", &LuaScriptComponent::GetCurrentEntity);
        script_type.set_function("SetThisComponent", &LuaScriptComponent::SetThisComponent);

        using namespace Maths;
        REGISTER_COMPONENT_WITH_ECS(state, Transform, static_cast<Transform& (Entity::*)()>(&Entity::AddComponent<Transform>));

        using namespace Graphics;
        sol::usertype<Sprite> sprite_type = state.new_usertype<Sprite>("Sprite", sol::constructors<sol::types<Maths::Vector2, Maths::Vector2, Maths::Vector4>, Sprite(const SharedRef<Graphics::Texture2D>&, const Maths::Vector2&, const Maths::Vector2&, const Maths::Vector4&)>());
        sprite_type.set_function("SetTexture", &Sprite::SetTexture);

        REGISTER_COMPONENT_WITH_ECS(state, Sprite, static_cast<Sprite& (Entity::*)(const Vector2&, const Vector2&, const Vector4&)>(&Entity::AddComponent<Sprite, const Vector2&, const Vector2&, const Vector4&>));

        state.new_usertype<Light>(
            "Light",
            "Intensity", &Light::Intensity,
            "Radius", &Light::Radius,
            "Colour", &Light::Colour,
            "Direction", &Light::Direction,
            "Position", &Light::Position,
            "Type", &Light::Type,
            "Angle", &Light::Angle);

        REGISTER_COMPONENT_WITH_ECS(state, Light, static_cast<Light& (Entity::*)()>(&Entity::AddComponent<Light>));

        std::initializer_list<std::pair<sol::string_view, Lumos::Graphics::PrimitiveType>> primitives = {
            { "Cube", Lumos::Graphics::PrimitiveType::Cube },
            { "Plane", Lumos::Graphics::PrimitiveType::Plane },
            { "Quad", Lumos::Graphics::PrimitiveType::Quad },
            { "Pyramid", Lumos::Graphics::PrimitiveType::Pyramid },
            { "Sphere", Lumos::Graphics::PrimitiveType::Sphere },
            { "Capsule", Lumos::Graphics::PrimitiveType::Capsule },
            { "Cylinder", Lumos::Graphics::PrimitiveType::Cylinder },
            { "Terrain", Lumos::Graphics::PrimitiveType::Terrain },
        };

        state.new_enum<Lumos::Graphics::PrimitiveType, false>("PrimitiveType", primitives);

        state.new_usertype<Model>("Model", sol::constructors<Model(const std::string&), Model(Lumos::Graphics::PrimitiveType)>());
        REGISTER_COMPONENT_WITH_ECS(state, Model, static_cast<Model& (Entity::*)(const std::string&)>(&Entity::AddComponent<Model, const std::string&>));

        sol::usertype<Camera> camera_type = state.new_usertype<Camera>("Camera", sol::constructors<Camera(float, float, float, float), Camera(float, float)>());
        camera_type["fov"] = &Camera::GetFOV;
        camera_type["aspectRatio"] = &Camera::GetAspectRatio;
        camera_type["nearPlane"] = &Camera::GetNear;
        camera_type["farPlane"] = &Camera::GetFar;
        camera_type["SetIsOrthographic"] = &Camera::SetIsOrthographic;
        camera_type["SetNearPlane"] = &Camera::SetNear;
        camera_type["SetFarPlane"] = &Camera::SetFar;

        REGISTER_COMPONENT_WITH_ECS(state, Camera, static_cast<Camera& (Entity::*)(const float&, const float&)>(&Entity::AddComponent<Camera, const float&, const float&>));

        sol::usertype<Physics3DComponent> physics3DComponent_type = state.new_usertype<Physics3DComponent>("Physics3DComponent", sol::constructors<sol::types<const SharedRef<RigidBody3D>&>>());
        physics3DComponent_type.set_function("GetRigidBody", &Physics3DComponent::GetRigidBody);
        REGISTER_COMPONENT_WITH_ECS(state, Physics3DComponent, static_cast<Physics3DComponent& (Entity::*)(SharedRef<RigidBody3D>&)>(&Entity::AddComponent<Physics3DComponent, SharedRef<RigidBody3D>&>));

        sol::usertype<Physics2DComponent> physics2DComponent_type = state.new_usertype<Physics2DComponent>("Physics2DComponent", sol::constructors<sol::types<const RigidBodyParameters&>>());
        physics2DComponent_type.set_function("GetRigidBody", &Physics2DComponent::GetRigidBody);

        REGISTER_COMPONENT_WITH_ECS(state, Physics2DComponent, static_cast<Physics2DComponent& (Entity::*)(const RigidBodyParameters&)>(&Entity::AddComponent<Physics2DComponent, const RigidBodyParameters&>));

        REGISTER_COMPONENT_WITH_ECS(state, SoundComponent, static_cast<SoundComponent& (Entity::*)()>(&Entity::AddComponent<SoundComponent>));

        //state.set_function("LoadMesh", &ModelLoader::LoadModel);
        //TODO MODEL
        sol::usertype<Graphics::Mesh> mesh_type = state.new_usertype<Graphics::Mesh>("Mesh");

        std::initializer_list<std::pair<sol::string_view, Lumos::Graphics::TextureFilter>> textureFilter = {
            { "None", Lumos::Graphics::TextureFilter::NONE },
            { "Linear", Lumos::Graphics::TextureFilter::LINEAR },
            { "Nearest", Lumos::Graphics::TextureFilter::NEAREST }
        };

        std::initializer_list<std::pair<sol::string_view, Lumos::Graphics::TextureWrap>> textureWrap = {
            { "None", Lumos::Graphics::TextureWrap::NONE },
            { "Repeat", Lumos::Graphics::TextureWrap::REPEAT },
            { "Clamp", Lumos::Graphics::TextureWrap::CLAMP },
            { "MirroredRepeat", Lumos::Graphics::TextureWrap::MIRRORED_REPEAT },
            { "ClampToEdge", Lumos::Graphics::TextureWrap::CLAMP_TO_EDGE },
            { "ClampToBorder", Lumos::Graphics::TextureWrap::CLAMP_TO_BORDER }
        };

        state.set_function("LoadMesh", &CreatePrimative);

        state.new_enum<Lumos::Graphics::TextureWrap, false>("TextureWrap", textureWrap);
        state.new_enum<Lumos::Graphics::TextureFilter, false>("TextureFilter", textureFilter);

        state.set_function("LoadTexture", &LoadTexture);
        state.set_function("LoadTextureWithParams", &LoadTextureWithParams);
    }

    static float LuaRand(float a, float b)
    {
        return Random32::Rand(a, b);
    }

    void LuaManager::BindSceneLua(sol::state& state)
    {
        sol::usertype<Scene> scene_type = state.new_usertype<Scene>("Scene");
        scene_type.set_function("GetRegistry", &Scene::GetRegistry);
        scene_type.set_function("GetEntityManager", &Scene::GetEntityManager);

        sol::usertype<Graphics::Texture2D> texture2D_type = state.new_usertype<Graphics::Texture2D>("Texture2D");
        texture2D_type.set_function("CreateFromFile", &Graphics::Texture2D::CreateFromFile);

        state.set_function("Rand", &LuaRand);
    }

    static void SwitchSceneByIndex(int index)
    {
        Application::Get().GetSceneManager()->SwitchScene(index);
    }

    static void SwitchScene()
    {
        Application::Get().GetSceneManager()->SwitchScene();
    }

    static void SwitchSceneByName(const std::string& name)
    {
        Application::Get().GetSceneManager()->SwitchScene(name);
    }

    static void SetPhysicsDebugFlags(int flags)
    {
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetDebugDrawFlags(flags);
    }

    void LuaManager::BindAppLua(sol::state& state)
    {
        sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
        state.set_function("SwitchSceneByIndex", &SwitchSceneByIndex);
        state.set_function("SwitchSceneByName", &SwitchSceneByName);
        state.set_function("SwitchScene", &SwitchScene);
        state.set_function("SetPhysicsDebugFlags", &SetPhysicsDebugFlags);

        std::initializer_list<std::pair<sol::string_view, Lumos::PhysicsDebugFlags>> physicsDebugFlags = {
            { "CONSTRAINT", Lumos::PhysicsDebugFlags::CONSTRAINT },
            { "MANIFOLD", Lumos::PhysicsDebugFlags::MANIFOLD },
            { "COLLISIONVOLUMES", Lumos::PhysicsDebugFlags::COLLISIONVOLUMES },
            { "COLLISIONNORMALS", Lumos::PhysicsDebugFlags::COLLISIONNORMALS },
            { "AABB", Lumos::PhysicsDebugFlags::AABB },
            { "LINEARVELOCITY", Lumos::PhysicsDebugFlags::LINEARVELOCITY },
            { "LINEARFORCE", Lumos::PhysicsDebugFlags::LINEARFORCE },
            { "BROADPHASE", Lumos::PhysicsDebugFlags::BROADPHASE },
            { "BROADPHASE_PAIRS", Lumos::PhysicsDebugFlags::BROADPHASE_PAIRS },
            { "BOUNDING_RADIUS", Lumos::PhysicsDebugFlags::BOUNDING_RADIUS },
        };

        state.new_enum<PhysicsDebugFlags, false>("PhysicsDebugFlags", physicsDebugFlags);

        app_type.set_function("GetWindowSize", &Application::GetWindowSize);
        state.set_function("GetAppInstance", &Application::Get);
    }
}
