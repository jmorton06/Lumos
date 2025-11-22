#include "Precompiled.h"
#include "LuaManager.h"
#include "Maths/Transform.h"
#include "Core/OS/Window.h"
#include "Core/OS/FileSystem.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Core/OS/Input.h"
#include "Scene/SceneManager.h"
#include "LuaScriptComponent.h"
#include "Scene/SceneGraph.h"
#include "Graphics/Camera/ThirdPersonCamera.h"
#include "Graphics/UI.h"

#include "Scene/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/Camera2D.h"

#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/Light.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Maths/Random.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/EntityFactory.h"
#include "Scene/Component/SoundComponent.h"
#include "Scene/Component/TextureMatrixComponent.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Component/AIComponent.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"

#include "ImGuiLua.h"
#include "PhysicsLua.h"
#include "MathsLua.h"

#include <imgui/imgui.h>
#include <sol/sol.hpp>
#if LUMOS_PROFILE
#include <Tracy/public/tracy/TracyLua.hpp>
#endif

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

#ifdef CUSTOM_SMART_PTR
namespace sol
{
    template <typename T>
    struct unique_usertype_traits<Lumos::SharedPtr<T>>
    {
        typedef T type;
        typedef Lumos::SharedPtr<T> actual_type;
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
    struct unique_usertype_traits<Lumos::UniquePtr<T>>
    {
        typedef T type;
        typedef Lumos::UniquePtr<T> actual_type;
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
    struct unique_usertype_traits<Lumos::WeakPtr<T>>
    {
        typedef T type;
        typedef Lumos::WeakPtr<T> actual_type;
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
    template <typename, typename>
    struct _ECS_export_view;

    template <typename... Component, typename... Exclude>
    struct _ECS_export_view<entt::type_list<Component...>, entt::type_list<Exclude...>>
    {
        static entt::view<entt::get_t<Component...>> view(entt::registry& registry)
        {
            return registry.view<Component...>(entt::exclude<Exclude...>);
        }
    };

#define REGISTER_COMPONENT_WITH_ECS(curLuaState, Comp, assignPtr)                                              \
    {                                                                                                          \
        using namespace entt;                                                                                  \
        auto entity_type = curLuaState["Entity"].get_or_create<sol::usertype<registry>>();                     \
        entity_type.set_function("Add" #Comp, assignPtr);                                                      \
        entity_type.set_function("Remove" #Comp, &Entity::RemoveComponent<Comp>);                              \
        entity_type.set_function("Get" #Comp, &Entity::GetComponent<Comp>);                                    \
        entity_type.set_function("GetOrAdd" #Comp, &Entity::GetOrAddComponent<Comp>);                          \
        entity_type.set_function("TryGet" #Comp, &Entity::TryGetComponent<Comp>);                              \
        entity_type.set_function("AddOrReplace" #Comp, &Entity::AddOrReplaceComponent<Comp>);                  \
        entity_type.set_function("Has" #Comp, &Entity::HasComponent<Comp>);                                    \
        auto entityManager_type = curLuaState["enttRegistry"].get_or_create<sol::usertype<registry>>();        \
        entityManager_type.set_function("view_" #Comp, &_ECS_export_view<type_list<Comp>, type_list<>>::view); \
        auto V = curLuaState.new_usertype<view<entt::get_t<Comp>>>(#Comp "_view");                             \
        V.set_function("each", &view<entt::get_t<Comp>>::each<std::function<void(Comp&)>>);                    \
        V.set_function("front", &view<entt::get_t<Comp>>::front);                                              \
        s_Identifiers.PushBack(#Comp);                                                                         \
        s_Identifiers.PushBack("Add" #Comp);                                                                   \
        s_Identifiers.PushBack("Remove" #Comp);                                                                \
        s_Identifiers.PushBack("Get" #Comp);                                                                   \
        s_Identifiers.PushBack("GetOrAdd" #Comp);                                                              \
        s_Identifiers.PushBack("TryGet" #Comp);                                                                \
        s_Identifiers.PushBack("AddOrReplace" #Comp);                                                          \
        s_Identifiers.PushBack("Has" #Comp);                                                                   \
    }

    TDArray<std::string> LuaManager::s_Identifiers;

    LuaManager::LuaManager()
        : m_State(nullptr)
    {
    }

    struct TracyEmpty
    {
    };

#if LUMOS_PROFILE && defined(TRACY_ENABLE)

#else
    static void Empty()
    {
    }
#endif
    void LuaManager::OnInit()
    {
        LUMOS_PROFILE_FUNCTION();

        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);
#if LUMOS_PROFILE && defined(TRACY_ENABLE)
        tracy::LuaRegister(m_State->lua_state());
#else
        sol::usertype<TracyEmpty> app_type = m_State->new_usertype<TracyEmpty>("tracy");
        app_type.set_function("ZoneBegin", &Empty);
        app_type.set_function("ZoneEnd", &Empty);
        app_type.set_function("ZoneName", &Empty);
        app_type.set_function("ZoneMessage", &Empty);
#endif
        s_Identifiers = {
            "Log",
            "Trace",
            "Info",
            "Warn",
            "Error",
            "FATAL",
            "Input",
            "GetKeyPressed",
            "GetKeyHeld",
            "GetMouseClicked",
            "GetMouseHeld",
            "GetMousePosition",
            "GetScrollOffset",
            "enttRegistry",
            "Entity",
            "EntityManager",
            "Create"
            "GetRegistry",
            "Valid",
            "Destroy",
            "SetParent",
            "GetParent",
            "IsParent",
            "GetChildren",
            "SetActive",
            "Active",
            "GetEntityByName",
            "AddPyramidEntity",
            "AddSphereEntity",
            "AddLightCubeEntity",
            "NameComponent",
            "GetNameComponent",
            "GetCurrentEntity",
            "SetThisComponent",
            "LuaScriptComponent",
            "GetLuaScriptComponent",
            "Transform",
            "GetTransform"
        };

        BindAppLua(*m_State);
        BindInputLua(*m_State);
        BindMathsLua(*m_State);
        BindImGuiLua(*m_State);
        BindECSLua(*m_State);
        BindLogLua(*m_State);
        BindSceneLua(*m_State);
        BindPhysicsLua(*m_State);
        BindUILua(*m_State);

        LINFO("Initialised Lua Manager");
    }

    LuaManager::~LuaManager()
    {
        delete m_State;
    }

    void LuaManager::OnInit(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry = scene->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        // auto& state = *m_State;
        // std::string ScriptsPath;
        // FileSystem::Get().ResolvePhysicalPath("//Assets/Scripts", ScriptsPath);
        //
        //// Setup the lua path to see luarocks packages
        // auto package_path = std::filesystem::path(ScriptsPath) / "lua" / "?.lua;";
        // package_path += std::filesystem::path(ScriptsPath) / "?" / "?.lua;";
        // package_path += std::filesystem::path(ScriptsPath) / "?" / "?" / "?.lua;";

        // std::string test = state["package"]["path"];
        // state["package"]["path"] = std::string(package_path.string()) + test;

        m_State->set("registry", &registry);
        m_State->set("scene", scene);

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

        float dt = (float)Engine::Get().GetTimeStep().GetSeconds();

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.OnUpdate(dt);
        }
    }

    void LuaManager::CollectGarbage()
    {
        m_State->collect_garbage();
    }

    void LuaManager::OnNewProject(const std::string& projectPath)
    {
        auto& state = *m_State;

        ArenaTemp Scratch = ScratchBegin(0, 0);
        String8 ScriptsPath;
        FileSystem::Get().ResolvePhysicalPath(Scratch.arena, Str8Lit("//Assets/Scripts"), &ScriptsPath);

        // Setup the lua path to see luarocks packages
        auto package_path = std::filesystem::path((const char*)ScriptsPath.str) / "lua" / "?.lua;";
        package_path += std::filesystem::path((const char*)ScriptsPath.str) / "?" / "?.lua;";
        package_path += std::filesystem::path((const char*)ScriptsPath.str) / "?" / "?" / "?.lua;";

        std::string currentPaths = state["package"]["path"];
        state["package"]["path"] = std::string(package_path.string()) + currentPaths;

        ScratchEnd(Scratch);
    }

    Entity GetEntityByName(Scene* scene, const std::string& name)
    {
        LUMOS_PROFILE_FUNCTION();
        entt::entity e           = entt::null;
        entt::registry& registry = scene->GetRegistry();
        registry.view<NameComponent>().each([&](const entt::entity& entity, const NameComponent& component)
                                            {
                                                if(name == component.name)
                                                {
                                                    e = entity;
                                                } });

        if(e == entt::null)
            LWARN("Failed to find entity %s", name.c_str());
        return { e, scene };
    }

    void LuaManager::BindLogLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        auto log = state.create_table("Log");

        log.set_function("Trace", [&](sol::this_state s, std::string_view message)
                         { LTRACE((char*)message.data()); });

        log.set_function("Info", [&](sol::this_state s, std::string_view message)
                         { LTRACE((char*)message.data()); });

        log.set_function("Warn", [&](sol::this_state s, std::string_view message)
                         { LWARN((char*)message.data()); });

        log.set_function("Error", [&](sol::this_state s, std::string_view message)
                         { LERROR((char*)message.data()); });

        log.set_function("FATAL", [&](sol::this_state s, std::string_view message)
                         { LFATAL((char*)message.data()); });
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

        input.set_function("GetMousePosition", []() -> Vec2
                           { return Input::Get().Get().GetMousePosition(); });

        input.set_function("GetScrollOffset", []() -> float
                           { return Input::Get().Get().GetScrollOffset(); });

        input.set_function("GetControllerAxis", [](int id, int axis) -> float
                           { return Input::Get().GetControllerAxis(id, axis); });

        input.set_function("GetControllerName", [](int id) -> std::string
                           { return Input::Get().GetControllerName(id); });

        input.set_function("GetControllerHat", [](int id, int hat) -> int
                           { return Input::Get().GetControllerHat(id, hat); });

        input.set_function("IsControllerButtonPressed", [](int id, int button) -> bool
                           { return Input::Get().IsControllerButtonPressed(id, button); });

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

    SharedPtr<Graphics::Texture2D> LoadTexture(const std::string& name, const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        return SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
    }

    SharedPtr<Graphics::Texture2D> LoadTextureWithParams(const std::string& name, const std::string& path, Lumos::Graphics::TextureFilter filter, Lumos::Graphics::TextureWrap wrapMode)
    {
        LUMOS_PROFILE_FUNCTION();
        return SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path, Graphics::TextureDesc(filter, filter, wrapMode)));
    }

    void LuaManager::BindECSLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();

        sol::usertype<entt::registry> enttRegistry = state.new_usertype<entt::registry>("enttRegistry");

        sol::usertype<Entity> entityType               = state.new_usertype<Entity>("Entity", sol::constructors<sol::types<entt::entity, Scene*>>());
        sol::usertype<EntityManager> entityManagerType = state.new_usertype<EntityManager>("EntityManager");
        entityManagerType.set_function("Create", static_cast<Entity (EntityManager::*)()>(&EntityManager::Create));
        entityManagerType.set_function("GetRegistry", &EntityManager::GetRegistry);

        entityType.set_function("Valid", &Entity::Valid);
        entityType.set_function("Destroy", &Entity::Destroy);
        entityType.set_function("SetParent", &Entity::SetParent);
        entityType.set_function("GetParent", &Entity::GetParent);
        entityType.set_function("IsParent", &Entity::IsParent);
        entityType.set_function("GetChildren", &Entity::GetChildrenTemp);
        entityType.set_function("SetActive", &Entity::SetActive);
        entityType.set_function("Active", &Entity::Active);

        state.set_function("GetEntityByName", &GetEntityByName);

        state.set_function("AddPyramidEntity", &EntityFactory::AddPyramid);
        state.set_function("AddSphereEntity", &EntityFactory::AddSphere);
        state.set_function("AddLightCubeEntity", &EntityFactory::AddLightCube);

        sol::usertype<NameComponent> nameComponent_type = state.new_usertype<NameComponent>("NameComponent");
        nameComponent_type["name"]                      = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS(state, NameComponent, static_cast<NameComponent& (Entity::*)()>(&Entity::AddComponent<NameComponent>));

        sol::usertype<LuaScriptComponent> script_type = state.new_usertype<LuaScriptComponent>("LuaScriptComponent", sol::constructors<sol::types<std::string, Scene*>>());
        REGISTER_COMPONENT_WITH_ECS(state, LuaScriptComponent, static_cast<LuaScriptComponent& (Entity::*)(std::string&&, Scene * &&)>(&Entity::AddComponent<LuaScriptComponent, std::string, Scene*>));
        script_type.set_function("GetCurrentEntity", &LuaScriptComponent::GetCurrentEntity);
        script_type.set_function("SetThisComponent", &LuaScriptComponent::SetThisComponent);

        using namespace Maths;
        REGISTER_COMPONENT_WITH_ECS(state, Transform, static_cast<Transform& (Entity::*)()>(&Entity::AddComponent<Transform>));

        using namespace Graphics;
        sol::usertype<TextComponent> textComponent_type = state.new_usertype<TextComponent>("TextComponent");
        textComponent_type["TextString"]                = &TextComponent::TextString;
        textComponent_type["Colour"]                    = &TextComponent::Colour;
        textComponent_type["MaxWidth"]                  = &TextComponent::MaxWidth;

        REGISTER_COMPONENT_WITH_ECS(state, TextComponent, static_cast<TextComponent& (Entity::*)()>(&Entity::AddComponent<TextComponent>));

        sol::usertype<Sprite> sprite_type = state.new_usertype<Sprite>("Sprite", sol::constructors<sol::types<Vec2, Vec2, Vec4>, Sprite(const SharedPtr<Graphics::Texture2D>&, const Vec2&, const Vec2&, const Vec4&)>());
        sprite_type.set_function("SetTexture", &Sprite::SetTexture);
        sprite_type.set_function("SetSpriteSheet", &Sprite::SetSpriteSheet);
        sprite_type.set_function("SetSpriteSheetIndex", &Sprite::SetSpriteSheetIndex);
        sprite_type["SpriteSheetTileSizeX"] = &Sprite::SpriteSheetTileSizeX;
        sprite_type["SpriteSheetTileSizeY"] = &Sprite::SpriteSheetTileSizeY;

        REGISTER_COMPONENT_WITH_ECS(state, Sprite, static_cast<Sprite& (Entity::*)(const Vec2&, const Vec2&, const Vec4&)>(&Entity::AddComponent<Sprite, const Vec2&, const Vec2&, const Vec4&>));

        sol::usertype<AnimatedSprite> AnimatedSpriteType = state.new_usertype<AnimatedSprite>("AnimatedSprite");
        AnimatedSpriteType.set_function("SetTexture", &AnimatedSprite::SetTexture);
        AnimatedSpriteType.set_function("SetSpriteSheet", &AnimatedSprite::SetSpriteSheet);
        AnimatedSpriteType.set_function("SetSpriteSheetIndex", &AnimatedSprite::SetSpriteSheetIndex);
        AnimatedSpriteType.set_function("SetState", &AnimatedSprite::SetState);
        AnimatedSpriteType["SpriteSheetTileSizeX"] = &AnimatedSprite::SpriteSheetTileSizeX;
        AnimatedSpriteType["SpriteSheetTileSizeY"] = &AnimatedSprite::SpriteSheetTileSizeY;

        REGISTER_COMPONENT_WITH_ECS(state, AnimatedSprite, static_cast<AnimatedSprite& (Entity::*)()>(&Entity::AddComponent<AnimatedSprite>));

        sol::usertype<Light> lightType = state.new_usertype<Light>("Light");
        lightType.set_function("Intensity", &Light::Intensity);
        lightType.set_function("Radius", &Light::Radius);
        lightType.set_function("Colour", &Light::Colour);
        lightType.set_function("Direction", &Light::Direction);
        lightType.set_function("Position", &Light::Position);
        lightType.set_function("Type", &Light::Type);
        lightType.set_function("Angle", &Light::Angle);

        REGISTER_COMPONENT_WITH_ECS(state, Light, static_cast<Light& (Entity::*)()>(&Entity::AddComponent<Light>));

        {
            std::initializer_list<std::pair<sol::string_view, ParticleEmitter::BlendType>> blendItems = {
                { "Additive", ParticleEmitter::BlendType::Additive },
                { "Alpha", ParticleEmitter::BlendType::Alpha },
                { "Off", ParticleEmitter::BlendType::Off }
            };
            state.new_enum<ParticleEmitter::BlendType, false>("ParticleBlendType", blendItems);

            std::initializer_list<std::pair<sol::string_view, ParticleEmitter::AlignedType>> alignedItems = {
                { "Aligned2D", ParticleEmitter::AlignedType::Aligned2D },
                { "Aligned3D", ParticleEmitter::AlignedType::Aligned3D },
                { "None", ParticleEmitter::AlignedType::None }
            };
            state.new_enum<ParticleEmitter::AlignedType, false>("ParticleAlignedType", alignedItems);

            sol::usertype<ParticleEmitter> particleEmitter_type = state.new_usertype<ParticleEmitter>("ParticleEmitter",
                                                                                                      sol::constructors<ParticleEmitter(), ParticleEmitter(uint32_t)>());

            particleEmitter_type.set_function("Update", &ParticleEmitter::Update);
            particleEmitter_type.set_function("SetTextureFromFile", &ParticleEmitter::SetTextureFromFile);

            particleEmitter_type.set_function("GetParticleCount", &ParticleEmitter::GetParticleCount);
            particleEmitter_type.set_function("GetParticleLife", &ParticleEmitter::GetParticleLife);
            particleEmitter_type.set_function("GetParticleSize", &ParticleEmitter::GetParticleSize);
            particleEmitter_type.set_function("GetParticleRate", &ParticleEmitter::GetParticleRate);
            particleEmitter_type.set_function("GetNumLaunchParticles", &ParticleEmitter::GetNumLaunchParticles);
            particleEmitter_type.set_function("GetIsAnimated", &ParticleEmitter::GetIsAnimated);
            particleEmitter_type.set_function("GetAnimatedTextureRows", &ParticleEmitter::GetAnimatedTextureRows);
            particleEmitter_type.set_function("GetSortParticles", &ParticleEmitter::GetSortParticles);
            particleEmitter_type.set_function("GetBlendType", &ParticleEmitter::GetBlendType);
            particleEmitter_type.set_function("GetFadeIn", &ParticleEmitter::GetFadeIn);
            particleEmitter_type.set_function("GetFadeOut", &ParticleEmitter::GetFadeOut);
            particleEmitter_type.set_function("GetLifeSpread", &ParticleEmitter::GetLifeSpread);
            particleEmitter_type.set_function("GetAlignedType", &ParticleEmitter::GetAlignedType);
            particleEmitter_type.set_function("GetDepthWrite", &ParticleEmitter::GetDepthWrite);

            particleEmitter_type.set_function("SetParticleCount", &ParticleEmitter::SetParticleCount);
            particleEmitter_type.set_function("SetParticleLife", &ParticleEmitter::SetParticleLife);
            particleEmitter_type.set_function("SetParticleSize", &ParticleEmitter::SetParticleSize);
            particleEmitter_type.set_function("SetInitialVelocity", &ParticleEmitter::SetInitialVelocity);
            particleEmitter_type.set_function("SetInitialColour", &ParticleEmitter::SetInitialColour);
            particleEmitter_type.set_function("SetSpread", &ParticleEmitter::SetSpread);
            particleEmitter_type.set_function("SetVelocitySpread", &ParticleEmitter::SetVelocitySpread);
            particleEmitter_type.set_function("SetGravity", &ParticleEmitter::SetGravity);
            particleEmitter_type.set_function("SetNextParticleTime", &ParticleEmitter::SetNextParticleTime);
            particleEmitter_type.set_function("SetParticleRate", &ParticleEmitter::SetParticleRate);
            particleEmitter_type.set_function("SetNumLaunchParticles", &ParticleEmitter::SetNumLaunchParticles);
            particleEmitter_type.set_function("SetIsAnimated", &ParticleEmitter::SetIsAnimated);
            particleEmitter_type.set_function("SetAnimatedTextureRows", &ParticleEmitter::SetAnimatedTextureRows);
            particleEmitter_type.set_function("SetSortParticles", &ParticleEmitter::SetSortParticles);
            particleEmitter_type.set_function("SetBlendType", &ParticleEmitter::SetBlendType);
            particleEmitter_type.set_function("SetFadeIn", &ParticleEmitter::SetFadeIn);
            particleEmitter_type.set_function("SetFadeOut", &ParticleEmitter::SetFadeOut);
            particleEmitter_type.set_function("SetLifeSpread", &ParticleEmitter::SetLifeSpread);
            particleEmitter_type.set_function("SetAlignedType", &ParticleEmitter::SetAlignedType);
            particleEmitter_type.set_function("SetDepthWrite", &ParticleEmitter::SetDepthWrite);
        }

        REGISTER_COMPONENT_WITH_ECS(state, ParticleEmitter, static_cast<ParticleEmitter& (Entity::*)()>(&Entity::AddComponent<ParticleEmitter>));

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

        auto Modeltype = state.new_usertype<Model>("Model");

        // Constructors
        Modeltype[sol::call_constructor] = sol::constructors<
            Lumos::Graphics::Model(),
            Lumos::Graphics::Model(const std::string&),
            Lumos::Graphics::Model(const Lumos::SharedPtr<Lumos::Graphics::Mesh>&, Lumos::Graphics::PrimitiveType),
            Lumos::Graphics::Model(Lumos::Graphics::PrimitiveType)>();

        // Properties
        Modeltype["meshes"]         = &Lumos::Graphics::Model::GetMeshes;
        Modeltype["file_path"]      = &Lumos::Graphics::Model::GetFilePath;
        Modeltype["primitive_type"] = sol::property(&Lumos::Graphics::Model::GetPrimitiveType, &Lumos::Graphics::Model::SetPrimitiveType);

        // Methods
        Modeltype["add_mesh"]   = &Lumos::Graphics::Model::AddMesh;
        Modeltype["load_model"] = &Lumos::Graphics::Model::LoadModel;

        REGISTER_COMPONENT_WITH_ECS(state, Model, static_cast<Model& (Entity::*)(const std::string&)>(&Entity::AddComponent<Model, const std::string&>));

        auto material_type = state.new_usertype<Material>("Material");
        // Setters
        material_type["set_albedo_texture"]    = &Material::SetAlbedoTexture;
        material_type["set_normal_texture"]    = &Material::SetNormalTexture;
        material_type["set_roughness_texture"] = &Material::SetRoughnessTexture;
        material_type["set_metallic_texture"]  = &Material::SetMetallicTexture;
        material_type["set_ao_texture"]        = &Material::SetAOTexture;
        material_type["set_emissive_texture"]  = &Material::SetEmissiveTexture;

        // Getters
        material_type["get_name"]       = &Material::GetName;
        material_type["get_properties"] = &Material::GetProperties;
        // material_type["get_textures"] = &Material::GetTextures; // Commented out in original
        material_type["get_shader"] = &Material::GetShader;

        // Other member functions
        material_type["load_pbr_material"]               = &Material::LoadPBRMaterial;
        material_type["load_material"]                   = &Material::LoadMaterial;
        material_type["set_textures"]                    = &Material::SetTextures;
        material_type["set_material_properties"]         = &Material::SetMaterialProperites;
        material_type["update_material_properties_data"] = &Material::UpdateMaterialPropertiesData;
        material_type["set_name"]                        = &Material::SetName;
        material_type["bind"]                            = &Material::Bind;

        // Enum for RenderFlags
        std::initializer_list<std::pair<sol::string_view, Material::RenderFlags>> render_flags = {
            { "NONE", Material::RenderFlags::NONE },
            { "DEPTHTEST", Material::RenderFlags::DEPTHTEST },
            { "WIREFRAME", Material::RenderFlags::WIREFRAME },
            { "FORWARDRENDER", Material::RenderFlags::FORWARDRENDER },
            { "DEFERREDRENDER", Material::RenderFlags::DEFERREDRENDER },
            { "NOSHADOW", Material::RenderFlags::NOSHADOW },
            { "TWOSIDED", Material::RenderFlags::TWOSIDED },
            { "ALPHABLEND", Material::RenderFlags::ALPHABLEND }

        };

        state.new_enum<Material::RenderFlags, false>("RenderFlags", render_flags);

        sol::usertype<Camera> camera_type = state.new_usertype<Camera>("Camera", sol::constructors<Camera(float, float, float, float), Camera(float, float)>());
        camera_type["fov"]                = &Camera::GetFOV;
        camera_type["aspectRatio"]        = &Camera::GetAspectRatio;
        camera_type["nearPlane"]          = &Camera::GetNear;
        camera_type["farPlane"]           = &Camera::GetFar;
        camera_type["SetIsOrthographic"]  = &Camera::SetIsOrthographic;
        camera_type["SetNearPlane"]       = &Camera::SetNear;
        camera_type["SetFarPlane"]        = &Camera::SetFar;

        REGISTER_COMPONENT_WITH_ECS(state, Camera, static_cast<Camera& (Entity::*)(const float&, const float&)>(&Entity::AddComponent<Camera, const float&, const float&>));

        sol::usertype<RigidBody3DComponent> RigidBody3DComponent_type = state.new_usertype<RigidBody3DComponent>("RigidBody3DComponent", sol::constructors<sol::types<RigidBody3D*>>());
        RigidBody3DComponent_type.set_function("GetRigidBody", &RigidBody3DComponent::GetRigidBody);

        REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)(const RigidBody3DProperties&)>(&Entity::AddComponent<RigidBody3DComponent, const RigidBody3DProperties&>));
        // REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)>(&Entity::AddComponent<RigidBody3DComponent));

        sol::usertype<RigidBody2DComponent> RigidBody2DComponent_type = state.new_usertype<RigidBody2DComponent>("RigidBody2DComponent", sol::constructors<sol::types<const RigidBodyParameters&>>());
        RigidBody2DComponent_type.set_function("GetRigidBody", &RigidBody2DComponent::GetRigidBody);

        REGISTER_COMPONENT_WITH_ECS(state, RigidBody2DComponent, static_cast<RigidBody2DComponent& (Entity::*)(const RigidBodyParameters&)>(&Entity::AddComponent<RigidBody2DComponent, const RigidBodyParameters&>));

        REGISTER_COMPONENT_WITH_ECS(state, SoundComponent, static_cast<SoundComponent& (Entity::*)()>(&Entity::AddComponent<SoundComponent>));

        auto mesh_type = state.new_usertype<Lumos::Graphics::Mesh>("Mesh",
                                                                   sol::constructors<Lumos::Graphics::Mesh(), Lumos::Graphics::Mesh(const Lumos::Graphics::Mesh&),
                                                                                     Lumos::Graphics::Mesh(const TDArray<uint32_t>&, const TDArray<Vertex>&)>());

        // Bind the member functions and variables
        mesh_type["GetMaterial"]    = &Lumos::Graphics::Mesh::GetMaterial;
        mesh_type["SetMaterial"]    = &Lumos::Graphics::Mesh::SetMaterial;
        mesh_type["GetBoundingBox"] = &Lumos::Graphics::Mesh::GetBoundingBox;
        mesh_type["SetName"]        = &Lumos::Graphics::Mesh::SetName;

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
        Application::Get().GetSceneManager()->SwitchScene(name.c_str());
    }

    static void SetPhysicsDebugFlags(int flags)
    {
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetDebugDrawFlags(flags);
    }

    static void ExitApp()
    {
        Application::Get().SetAppState(Lumos::AppState::Closing);
    }

    void LuaManager::BindAppLua(sol::state& state)
    {
        sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
        state.set_function("SwitchSceneByIndex", &SwitchSceneByIndex);
        state.set_function("SwitchSceneByName", &SwitchSceneByName);
        state.set_function("SwitchScene", &SwitchScene);
        state.set_function("SetPhysicsDebugFlags", &SetPhysicsDebugFlags);
        state.set_function("ExitApp", &ExitApp);

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

    void LuaManager::BindUILua(sol::state& lua)
    {
        // Enums
        lua.new_enum("WidgetFlags",
                     "Clickable", Lumos::WidgetFlags_Clickable,
                     "DrawText", Lumos::WidgetFlags_DrawText,
                     "DrawBorder", Lumos::WidgetFlags_DrawBorder,
                     "DrawBackground", Lumos::WidgetFlags_DrawBackground,
                     "Draggable", Lumos::WidgetFlags_Draggable,
                     "StackVertically", Lumos::WidgetFlags_StackVertically,
                     "StackHorizontally", Lumos::WidgetFlags_StackHorizontally,
                     "Floating_X", Lumos::WidgetFlags_Floating_X,
                     "Floating_Y", Lumos::WidgetFlags_Floating_Y,
                     "CentreX", Lumos::WidgetFlags_CentreX,
                     "CentreY", Lumos::WidgetFlags_CentreY,
                     "DragParent", Lumos::WidgetFlags_DragParent);

        lua.new_enum("UITextAlignment",
                     "None", Lumos::UI_Text_Alignment_None,
                     "Center_X", Lumos::UI_Text_Alignment_Center_X,
                     "Center_Y", Lumos::UI_Text_Alignment_Center_Y);

        lua.new_enum("SizeKind",
                     "Pixels", Lumos::SizeKind_Pixels,
                     "TextContent", Lumos::SizeKind_TextContent,
                     "PercentOfParent", Lumos::SizeKind_PercentOfParent,
                     "ChildSum", Lumos::SizeKind_ChildSum,
                     "MaxChild", Lumos::SizeKind_MaxChild);

        lua.new_enum("UIAxis",
                     "X", Lumos::UIAxis_X,
                     "Y", Lumos::UIAxis_Y,
                     "Count", Lumos::UIAxis_Count);

        lua.new_enum("StyleVar",
                     "Padding", Lumos::StyleVar_Padding,
                     "Border", Lumos::StyleVar_Border,
                     "BorderColor", Lumos::StyleVar_BorderColor,
                     "BackgroundColor", Lumos::StyleVar_BackgroundColor,
                     "TextColor", Lumos::StyleVar_TextColor,
                     "HotBorderColor", Lumos::StyleVar_HotBorderColor,
                     "HotBackgroundColor", Lumos::StyleVar_HotBackgroundColor,
                     "HotTextColor", Lumos::StyleVar_HotTextColor,
                     "ActiveBorderColor", Lumos::StyleVar_ActiveBorderColor,
                     "ActiveBackgroundColor", Lumos::StyleVar_ActiveBackgroundColor,
                     "ActiveTextColor", Lumos::StyleVar_ActiveTextColor,
                     "FontSize", Lumos::StyleVar_FontSize,
                     "Count", Lumos::StyleVar_Count);

        // Structs
        lua.new_usertype<Lumos::UI_Size>("UI_Size",
                                         sol::constructors<Lumos::UI_Size()>(),
                                         "kind", &Lumos::UI_Size::kind,
                                         "value", &Lumos::UI_Size::value);

        lua.new_usertype<Lumos::UI_Widget>("UI_Widget",
                                           "parent", &Lumos::UI_Widget::parent,
                                           "first", &Lumos::UI_Widget::first,
                                           "last", &Lumos::UI_Widget::last,
                                           "next", &Lumos::UI_Widget::next,
                                           "prev", &Lumos::UI_Widget::prev,
                                           "style_vars", &Lumos::UI_Widget::style_vars,
                                           "hash", &Lumos::UI_Widget::hash,
                                           "flags", &Lumos::UI_Widget::flags,
                                           "text", &Lumos::UI_Widget::text,
                                           "texture", &Lumos::UI_Widget::texture,
                                           "semantic_size", &Lumos::UI_Widget::semantic_size,
                                           "LayoutingAxis", &Lumos::UI_Widget::LayoutingAxis,
                                           "TextAlignment", &Lumos::UI_Widget::TextAlignment,
                                           "cursor", &Lumos::UI_Widget::cursor,
                                           "position", &Lumos::UI_Widget::position,
                                           "relative_position", &Lumos::UI_Widget::relative_position,
                                           "size", &Lumos::UI_Widget::size,
                                           "clicked", &Lumos::UI_Widget::clicked,
                                           "is_initial_dragging_position_set", &Lumos::UI_Widget::is_initial_dragging_position_set,
                                           "dragging", &Lumos::UI_Widget::dragging,
                                           "drag_constraint_x", &Lumos::UI_Widget::drag_constraint_x,
                                           "drag_constraint_y", &Lumos::UI_Widget::drag_constraint_y,
                                           "drag_offset", &Lumos::UI_Widget::drag_offset,
                                           "drag_mouse_p", &Lumos::UI_Widget::drag_mouse_p,
                                           "HotTransition", &Lumos::UI_Widget::HotTransition,
                                           "ActiveTransition", &Lumos::UI_Widget::ActiveTransition,
                                           "LastFrameIndexActive", &Lumos::UI_Widget::LastFrameIndexActive);

        lua.new_usertype<Lumos::UI_Interaction>("UI_Interaction",
                                                "widget", &Lumos::UI_Interaction::widget,
                                                "hovering", &Lumos::UI_Interaction::hovering,
                                                "clicked", &Lumos::UI_Interaction::clicked,
                                                "dragging", &Lumos::UI_Interaction::dragging);

        // Functions
        lua["GetUIState"]    = &Lumos::GetUIState;
        lua["GetStringSize"] = &Lumos::GetStringSize;
        lua["InitialiseUI"]  = &Lumos::InitialiseUI;
        lua["ShutDownUI"]    = &Lumos::ShutDownUI;
        lua["UIBeginFrame"]  = &Lumos::UIBeginFrame;
        lua["UIEndFrame"]    = &Lumos::UIEndFrame;
        lua["UIBeginPanel"]  = sol::overload(
            static_cast<Lumos::UI_Interaction (*)(const char*)>(&Lumos::UIBeginPanel),
            static_cast<Lumos::UI_Interaction (*)(const char*, u32)>(&Lumos::UIBeginPanel),
            static_cast<Lumos::UI_Interaction (*)(const char*, Lumos::SizeKind, float, Lumos::SizeKind, float, u32)>(&Lumos::UIBeginPanel));
        lua["UIEndPanel"]  = &Lumos::UIEndPanel;
        lua["UIPushStyle"] = sol::overload(
            static_cast<void (*)(Lumos::StyleVar, float)>(&Lumos::UIPushStyle),
            static_cast<void (*)(Lumos::StyleVar, const Vec2&)>(&Lumos::UIPushStyle),
            static_cast<void (*)(Lumos::StyleVar, const Vec3&)>(&Lumos::UIPushStyle),
            static_cast<void (*)(Lumos::StyleVar, const Vec4&)>(&Lumos::UIPushStyle));

        lua["UIPopStyle"] = &Lumos::UIPopStyle;
        lua["UILabel"]    = &Lumos::UILabelCStr;
        lua["UIButton"]   = &Lumos::UIButton;
        lua["UIImage"]    = &Lumos::UIImage;
        lua["UISlider"]   = &Lumos::UISlider;
        // lua["UIToggle"]     = &Lumos::UIToggle;
        lua["UILayoutRoot"] = &Lumos::UILayoutRoot;

        lua.set_function("UIToggle", [](const char* label, const bool& value)
                         { 
                    bool tempValue = value;
                    Lumos::UIToggle(label, &tempValue);
                    return tempValue; });
    }

}
