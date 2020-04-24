#include "lmpch.h"
#include "LuaManager.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"
#include "Core/OS/Window.h"
#include "Core/VFS.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "App/Engine.h"
#include "Core/OS/Input.h"
#include "ScriptComponent.h"
#include "App/SceneGraph.h"
#include "Graphics/Camera/ThirdPersonCamera.h"

#include "ECS/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"

#include <imgui/imgui.h>

namespace Lumos
{
	LuaManager::LuaManager() : m_State(nullptr)
	{
	}

	void LuaManager::OnInit()
	{
		m_State = lmnew sol::state();
		m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);

        BindInputLua(m_State);
		BindMathsLua(m_State);
		BindImGuiLua(m_State);
		BindECSLua(m_State);
        BindLogLua(m_State);
        BindSceneLua(m_State);
	}

	LuaManager::~LuaManager()
	{
		lmdel m_State;
	}

    void LuaManager::OnUpdate(Scene* scene)
    {
        auto& registry = scene->GetRegistry();
                                         
        auto view = registry.view<ScriptComponent>();
    
        if (view.empty())
            return;

        float dt = Engine::Instance()->GetTimeStep()->GetElapsedMillis();
        
        for (auto entity : view)
        {
            auto& luaScript = registry.get<ScriptComponent>(entity);
            luaScript.Update(dt);
        }
    }

	WindowProperties LuaManager::LoadConfigFile(const String& file)
	{
		WindowProperties windowProperties;

		std::string physicalPath;
		if (!VFS::Get()->ResolvePhysicalPath(file, physicalPath))
			return windowProperties;

		m_State->script_file(physicalPath);
		windowProperties.Title = m_State->get<std::string>("title");
		windowProperties.Width = m_State->get<int>("width");
		windowProperties.Height = m_State->get<int>("height");
		windowProperties.RenderAPI = m_State->get<int>("renderAPI");
		windowProperties.Fullscreen = m_State->get<bool>("fullscreen");
		windowProperties.Borderless = m_State->get<bool>("borderless");

		return windowProperties;
	}

    entt::entity GetEntityByName( entt::registry& registry, const std::string& name )
    {
        entt::entity e = entt::null;
        registry.view<NameComponent>().each([&]( const entt::entity& entity, const NameComponent& component )
        {
            if ( name == component.name )
            {
                e = entity;
            }
        });

        return e;
    }

    void LuaManager::BindLogLua(sol::state* state)
    {
        auto log = state->create_table("Log");

        log.set_function("Trace", [&](sol::this_state s, std::string_view message)
        {
            Lumos::Debug::Log::Trace(message);
        });

        log.set_function("Info", [&](sol::this_state s, std::string_view message)
        {
            Lumos::Debug::Log::Trace(message);
        });

        log.set_function("Warn", [&](sol::this_state s, std::string_view message)
        {
            Lumos::Debug::Log::Warning(message);
        });
        
        log.set_function("Error", [&](sol::this_state s, std::string_view message)
        {
            Lumos::Debug::Log::Error(message);
        });
        
        log.set_function("Critical", [&](sol::this_state s, std::string_view message)
        {
            Lumos::Debug::Log::Critical(message);
        });
    }

    void LuaManager::BindInputLua(sol::state* state)
    {
        auto input = (*state)["Input"].get_or_create< sol::table >();

        input.set_function( "GetKeyPressed", [](Lumos::InputCode::Key key) -> bool {
            return Input::GetInput()->GetKeyPressed(key);
        });
        
        input.set_function( "GetKeyHeld", [](Lumos::InputCode::Key key) -> bool {
            return Input::GetInput()->GetKeyHeld(key);
        });
        
        input.set_function( "GetMouseClicked", [](Lumos::InputCode::MouseKey key) -> bool {
            return Input::GetInput()->GetMouseClicked(key);
        });
        
        input.set_function( "GetMouseHeld", [](Lumos::InputCode::MouseKey key) -> bool {
            return Input::GetInput()->GetMouseHeld(key);
        });
        
        input.set_function( "GetMousePosition", []() -> Maths::Vector2 {
            return Input::GetInput()->GetMousePosition();
        });
        
        input.set_function( "GetScrollOffset", []() -> float {
            return Input::GetInput()->GetScrollOffset();
        });
        
        std::initializer_list< std::pair< sol::string_view, Lumos::InputCode::Key > > keyItems =
        {
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
            //{ "APOSTROPHE", Lumos::InputCode::Key::APOSTROPHE },
            { "Comma", Lumos::InputCode::Key::Comma },
            //{ "MINUS", Lumos::InputCode::Key::Minus },
            //{ "PERIOD", Lumos::InputCode::Key::Period },
            //{ "SLASH", Lumos::InputCode::Key::Slash },
            //{ "SEMICOLON", Lumos::InputCode::Key::SemiColon },
            //{ "EQUAL", Lumos::InputCode::Key::Equal },
            //{ "LEFT_BRACKET", Lumos::InputCode::Key::LeftBracket },
            //{ "BACKSLASH", Lumos::InputCode::Key::BackSlash },
            //{ "RIGHT_BRACKET", Lumos::InputCode::Key::RightBracket },
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
            //{ "CAPS_LOCK", Lumos::InputCode::Key::CapsLock },
            //{ "SCROLL_LOCK", Lumos::InputCode::Key::ScrollLock },
            { "NumLock", Lumos::InputCode::Key::NumLock },
            { "PrintScreen", Lumos::InputCode::Key::Print },
            { "Pasue", Lumos::InputCode::Key::Pause },
            { "LeftShift", Lumos::InputCode::Key::LeftShift },
            { "LeftControl", Lumos::InputCode::Key::LeftControl },
            //{ "LEFT_ALT", Lumos::InputCode::Key::LeftAlt },
            //{ "LEFT_SUPER", Lumos::InputCode::Key::LeftSuper },
            { "RightShift", Lumos::InputCode::Key::RightShift },
            { "RightControl", Lumos::InputCode::Key::RightControl },
            //{ "RIGHT_ALT", Lumos::InputCode::Key::RightAlt },
            //{ "RIGHT_SUPER", Lumos::InputCode::Key::RightSuper },
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
            { "Keypad0", Lumos::InputCode::Key::Keypad0 },
            { "Keypad1", Lumos::InputCode::Key::Keypad1 },
            { "Keypad2", Lumos::InputCode::Key::Keypad2 },
            { "Keypad3", Lumos::InputCode::Key::Keypad3 },
            { "Keypad4", Lumos::InputCode::Key::Keypad4 },
            { "Keypad5", Lumos::InputCode::Key::Keypad5 },
            { "Keypad6", Lumos::InputCode::Key::Keypad6 },
            { "Keypad7", Lumos::InputCode::Key::Keypad7 },
            { "Keypad8", Lumos::InputCode::Key::Keypad8 },
            { "Keypad9", Lumos::InputCode::Key::Keypad9 },
            { "Decimal",  Lumos::InputCode::Key::Decimal },
            { "Divide",   Lumos::InputCode::Key::Divide },
            { "Multiply", Lumos::InputCode::Key::Multiply },
            { "Subtract", Lumos::InputCode::Key::Subtract },
            { "Add",      Lumos::InputCode::Key::Add },
            //{ "KP_EQUAL",    Lumos::InputCode::Key::Equal }
        };
        state->new_enum< Lumos::InputCode::Key, false >( "Key", keyItems ); // false makes it read/write in Lua, but its faster

        std::initializer_list< std::pair< sol::string_view, Lumos::InputCode::MouseKey > > mouseItems =
        {
                { "Left", Lumos::InputCode::MouseKey::ButtonLeft },
                { "Right", Lumos::InputCode::MouseKey::ButtonRight },
                { "Middle", Lumos::InputCode::MouseKey::ButtonMiddle },
        };
        state->new_enum< Lumos::InputCode::MouseKey, false >( "MouseButton", mouseItems );
    }

    void LuaManager::BindECSLua(sol::state * state)
    {
        sol::usertype<entt::registry> reg_type = state->new_usertype< entt::registry >( "Registry" );
        reg_type.set_function( "Create", static_cast< entt::entity( entt::registry::* )() >( &entt::registry::create ) );
        reg_type.set_function( "Destroy", static_cast< void( entt::registry::* )( entt::entity ) >( &entt::registry::destroy ) );
        state->set_function( "GetEntityByName", &GetEntityByName );
        
        sol::usertype< NameComponent > nameComponent_type = state->new_usertype< NameComponent >( "NameComponent" );
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS( (*state), NameComponent, static_cast< NameComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace< NameComponent > ) );
        
        sol::usertype< ScriptComponent > script_type = state->new_usertype< ScriptComponent >( "ScriptComponent", sol::constructors<sol::types<String, Scene*>>() );
        REGISTER_COMPONENT_WITH_ECS( (*state), ScriptComponent, static_cast< ScriptComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace< ScriptComponent > ) );
        
        using namespace Maths;
        REGISTER_COMPONENT_WITH_ECS( (*state), Transform, static_cast<Transform&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Transform > ) );
        
        using namespace Graphics;
        REGISTER_COMPONENT_WITH_ECS( (*state), Sprite, static_cast<Sprite&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Sprite > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), Light, static_cast<Light&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Light > ) );

        REGISTER_COMPONENT_WITH_ECS( (*state), MeshComponent, static_cast<MeshComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<MeshComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), CameraComponent, static_cast<CameraComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<CameraComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), Physics3DComponent, static_cast<Physics3DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics3DComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), Physics3DComponent, static_cast<Physics3DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics3DComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), Physics2DComponent, static_cast<Physics2DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics2DComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), SoundComponent, static_cast<SoundComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<SoundComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( (*state), MaterialComponent, static_cast<MaterialComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<MaterialComponent > ) );
    }

    void LuaManager::BindSceneLua(sol::state* state)
    {
        sol::usertype<Scene> scene_type = state->new_usertype< Scene >( "Scene" );
        scene_type.set_function( "GetRegistry", &Scene::GetRegistry );
        scene_type.set_function("GetCamera", &Scene::GetCamera);
        
        sol::usertype< ThirdPersonCamera > camera_type = state->new_usertype< ThirdPersonCamera >( "Camera", sol::constructors<sol::types<float, float, float, float>>() );
        camera_type["position"]                 = &ThirdPersonCamera::GetPosition;
        camera_type["yaw"]                      = &ThirdPersonCamera::GetYaw;
        camera_type["fov"]                      = &ThirdPersonCamera::GetFOV;
        camera_type["aspectRatio"]              = &ThirdPersonCamera::GetAspectRatio;
        camera_type["nearPlane"]                = &ThirdPersonCamera::GetNear;
        camera_type["farPlane"]                 = &ThirdPersonCamera::GetFar;
        camera_type["GetForwardDir"]            = &ThirdPersonCamera::GetForwardDirection;
        camera_type["GetUpDir"]                 = &ThirdPersonCamera::GetUpDirection;
        camera_type["GetRightDir"]              = &ThirdPersonCamera::GetRightDirection;
    }

    void LuaManager::BindMathsLua(sol::state * state)
    {
        state->new_usertype<Maths::Vector2>("Vector2",
            sol::constructors<Maths::Vector2(float, float)>(),
            "x", &Maths::Vector2::x,
            "y", &Maths::Vector2::y,
            sol::meta_function::addition, sol::overload(
                static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator+)
            ),
            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector2(Maths::Vector2::*)(float) const>(&Maths::Vector2::operator*),
                static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator*)
            ),
            sol::meta_function::subtraction, sol::overload(
                static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator-)
            ),
            sol::meta_function::division, sol::overload(
                static_cast<Maths::Vector2(Maths::Vector2::*)(float) const>(&Maths::Vector2::operator/),
                static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator/)
            ),
            sol::meta_function::equal_to, &Maths::Vector2::operator==
            );

        state->new_usertype<Maths::Vector3>("Vector3",
            sol::constructors<sol::types<>, sol::types<float, float, float>>(),
            "x", &Maths::Vector3::x,
            "y", &Maths::Vector3::y,
            "z", &Maths::Vector3::z,
            "Dot", static_cast<float(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::DotProduct),
            "Cross", static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::CrossProduct),

            sol::meta_function::addition, sol::overload(
                static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator+)
            ),
            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector3(Maths::Vector3::*)(float) const>(&Maths::Vector3::operator*),
                static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator*)
            ),
            sol::meta_function::subtraction, sol::overload(
                static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator-)
            ),
            sol::meta_function::division, sol::overload(
                static_cast<Maths::Vector3(Maths::Vector3::*)(float) const>(&Maths::Vector3::operator/),
                static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator/)
            ),
            sol::meta_function::equal_to, &Maths::Vector3::operator==
            );

        state->new_usertype<Maths::Vector4>("Vector4",
            sol::constructors<Maths::Vector4(), Maths::Vector4(float, float, float, float)>(),
            "x", &Maths::Vector4::x,
            "y", &Maths::Vector4::y,
            "z", &Maths::Vector4::z,
            "w", &Maths::Vector4::w,

            sol::meta_function::addition, sol::overload(
                static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator+),
                static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator+)
            ),
            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator*),
                static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator*)
            ),
            sol::meta_function::subtraction, sol::overload(
                static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator-),
                static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator-)
            ),
            sol::meta_function::division, sol::overload(
                static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator/),
                static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator/)
            ),
            sol::meta_function::equal_to, &Maths::Vector4::operator==
            );

        state->new_usertype<Maths::Quaternion>("Quaternion",
            sol::constructors<Maths::Quaternion(float, float, float) , Maths::Quaternion(float, float, float, float) , Maths::Quaternion(Maths::Vector3)>(),
            "x", &Maths::Quaternion::x,
            "y", &Maths::Quaternion::y,
            "z", &Maths::Quaternion::z,
            "w", &Maths::Quaternion::w,
            "EulerAngles", &Maths::Quaternion::EulerAngles,
            "Conjugate", &Maths::Quaternion::Conjugate,
            "Normalize", &Maths::Quaternion::Normalize,
            "Normal", &Maths::Quaternion::Normalized,
            sol::meta_function::addition, sol::overload(
                static_cast<Maths::Quaternion(Maths::Quaternion::*)(const Maths::Quaternion&) const>(&Maths::Quaternion::operator+)
            ),
            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Quaternion(Maths::Quaternion::*)(float) const>(&Maths::Quaternion::operator*),
                static_cast<Maths::Quaternion(Maths::Quaternion::*)(const Maths::Quaternion&) const>(&Maths::Quaternion::operator*)
            ),
            sol::meta_function::subtraction, sol::overload(
                static_cast<Maths::Quaternion(Maths::Quaternion::*)(const Maths::Quaternion&) const>(&Maths::Quaternion::operator-)
            ),
            sol::meta_function::equal_to, &Maths::Quaternion::operator==
            );

        state->new_usertype<Maths::Matrix3>("Matrix3",
            sol::constructors<Maths::Matrix3(float, float, float, float, float,float,float,float,float), Maths::Matrix3()>(),
            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector3(Maths::Matrix3::*)(const Maths::Vector3&) const>(&Maths::Matrix3::operator*),
                static_cast<Maths::Matrix3(Maths::Matrix3::*)(const Maths::Matrix3&) const>(&Maths::Matrix3::operator*)
            )
            );

        state->new_usertype<Maths::Matrix4>("Matrix4",
            sol::constructors<Maths::Matrix4(Maths::Matrix3), Maths::Matrix4()>(),
		
  /*          "Scale", &Maths::Matrix4::Scale,
            "Rotation", &Maths::Matrix4::Rotation,
            "Translation", &Maths::Matrix4::Translation,*/

            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector4(Maths::Matrix4::*)(const Maths::Vector4&) const>(&Maths::Matrix4::operator*),
                static_cast<Maths::Matrix4(Maths::Matrix4::*)(const Maths::Matrix4&) const>(&Maths::Matrix4::operator*)
            )
            );

        state->new_usertype<Maths::Transform>("Transform",
            sol::constructors<Maths::Transform(Maths::Matrix4), Maths::Transform(), Maths::Transform(Maths::Vector3)>(),

            "LocalScale", &Maths::Transform::GetLocalScale,
            "LocalOrientation", &Maths::Transform::GetLocalOrientation,
            "LocalPosition", &Maths::Transform::GetLocalPosition,
            "ApplyTransform", &Maths::Transform::ApplyTransform,
            "UpdateMatrices", &Maths::Transform::UpdateMatrices,
            "SetLocalTransform", &Maths::Transform::SetLocalTransform,
            "SetLocalPosition", &Maths::Transform::SetLocalPosition
            );
    }


    void LuaManager::BindImGuiLua(sol::state* solState)
    {
        // imgui bindings=
        sol::table globals = solState->globals();
        sol::table imgui = solState->create_table();
        globals["gui"] = imgui;

        imgui.new_enum("DrawCornerFlags",
            "TopLeft",
            1 << 0,
            "TopRight",
            1 << 1,
            "BotLeft",
            1 << 2,
            "BotRight",
            1 << 3,
            "Top",
            ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_TopRight,
            "Bot",
            ImDrawCornerFlags_BotLeft | ImDrawCornerFlags_BotRight,
            "Left",
            ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft,
            "Right",
            ImDrawCornerFlags_TopRight | ImDrawCornerFlags_BotRight,
            "All",
            0xF);

        imgui.new_enum(
            "DrawListFlags", "None", 0, "AntiAliasedLines", 1 << 0, "AntiAliasedFill", 1 << 1);

        imgui.new_enum(
            "FontAtlasFlags", "None", 0, "NoPowerOfTwoHeight", 1 << 0, "NoMouseCursors", 1 << 1);

        imgui.new_enum("BackendFlags",
            "None",
            0,
            "HasGamepad",
            1 << 0,
            "HasMouseCursors",
            1 << 1,
            "HasSetMousePos",
            1 << 2);

        imgui.new_enum("Col",
            "Text",
            0,
            "TextDisabled",
            1,
            "WindowBg",
            2,
            "ChildBg",
            3,
            "PopupBg",
            4,
            "Border",
            5,
            "BorderShadow",
            6,
            "FrameBg",
            7,
            "FrameBgHovered",
            8,
            "FrameBgActive",
            9,
            "TitleBg",
            10,
            "TitleBgActive",
            11,
            "TitleBgCollapsed",
            12,
            "MenuBarBg",
            13,
            "ScrollbarBg",
            14,
            "ScrollbarGrab",
            15,
            "ScrollbarGrabHovered",
            16,
            "ScrollbarGrabActive",
            17,
            "CheckMark",
            18,
            "SliderGrab",
            19,
            "SliderGrabActive",
            20,
            "Button",
            21,
            "ButtonHovered",
            22,
            "ButtonActive",
            23,
            "Header",
            24,
            "HeaderHovered",
            25,
            "HeaderActive",
            26,
            "Separator",
            27,
            "SeparatorHovered",
            28,
            "SeparatorActive",
            29,
            "ResizeGrip",
            30,
            "ResizeGripHovered",
            31,
            "ResizeGripActive",
            32,
            "Tab",
            33,
            "TabHovered",
            34,
            "TabActive",
            35,
            "TabUnfocused",
            36,
            "TabUnfocusedActive",
            37,
            "PlotLines",
            38,
            "PlotLinesHovered",
            39,
            "PlotHistogram",
            40,
            "PlotHistogramHovered",
            41,
            "TextSelectedBg",
            42,
            "DragDropTarget",
            43,
            "NavHighlight",
            44,
            "NavWindowingHighlight",
            45,
            "NavWindowingDimBg",
            46,
            "ModalWindowDimBg",
            47,
            "COUNT",
            48);

        imgui.new_enum("ColorEditFlags",
            "None",
            0,
            "NoAlpha",
            1 << 1,
            "NoPicker",
            1 << 2,
            "NoOptions",
            1 << 3,
            "NoSmallPreview",
            1 << 4,
            "NoInputs",
            1 << 5,
            "NoTooltip",
            1 << 6,
            "NoLabel",
            1 << 7,
            "NoSidePreview",
            1 << 8,
            "NoDragDrop",
            1 << 9,
            "AlphaBar",
            1 << 16,
            "AlphaPreview",
            1 << 17,
            "AlphaPreviewHalf",
            1 << 18,
            "HDR",
            1 << 19,
            "RGB",
            1 << 20,
            "HSV",
            1 << 21,
            "HEX",
            1 << 22,
            "Uint8",
            1 << 23,
            "Float",
            1 << 24,
            "PickerHueBar",
            1 << 25,
            "PickerHueWheel",
            1 << 26,
            "_InputsMask",
            ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV | ImGuiColorEditFlags_HEX,
            "_DataTypeMask",
            ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_Float,
            "_PickerMask",
            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_PickerHueBar,
            "_OptionsDefault",
            ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_RGB |
            ImGuiColorEditFlags_PickerHueBar);

        imgui.new_enum("ComboFlags",
            "None",
            0,
            "PopupAlignLeft",
            1 << 0,
            "HeightSmall",
            1 << 1,
            "HeightRegular",
            1 << 2,
            "HeightLarge",
            1 << 3,
            "HeightLargest",
            1 << 4,
            "NoArrowButton",
            1 << 5,
            "NoPreview",
            1 << 6,
            "HeightMask_",
            ImGuiComboFlags_HeightSmall | ImGuiComboFlags_HeightRegular |
            ImGuiComboFlags_HeightLarge | ImGuiComboFlags_HeightLargest);

        imgui.new_enum(
            "Cond", "Always", 1 << 0, "Once", 1 << 1, "FirstUseEver", 1 << 2, "Appearing", 1 << 3);

        imgui.new_enum("ConfigFlags",
            "None",
            0,
            "NavEnableKeyboard",
            1 << 0,
            "NavEnableGamepad",
            1 << 1,
            "NavEnableSetMousePos",
            1 << 2,
            "NavNoCaptureKeyboard",
            1 << 3,
            "NoMouse",
            1 << 4,
            "NoMouseCursorChange",
            1 << 5,
            "IsSRGB",
            1 << 20,
            "IsTouchScreen",
            1 << 21);

        imgui.new_enum(
            "DataType", "S32", 0, "U32", 1, "S64", 2, "U64", 3, "Float", 4, "Double", 5, "COUNT", 6);

        imgui.new_enum("Dir", "None", -1, "Left", 0, "Right", 1, "Up", 2, "Down", 3, "COUNT", 4);

        imgui.new_enum("DragDropFlags",
            "None",
            0,
            "SourceNoPreviewTooltip",
            1 << 0,
            "SourceNoDisableHover",
            1 << 1,
            "SourceNoHoldToOpenOthers",
            1 << 2,
            "SourceAllowNullID",
            1 << 3,
            "SourceExtern",
            1 << 4,
            "SourceAutoExpirePayload",
            1 << 5,
            "AcceptBeforeDelivery",
            1 << 10,
            "AcceptNoDrawDefaultRect",
            1 << 11,
            "AcceptNoPreviewTooltip",
            1 << 12,
            "AcceptPeekOnly",
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

        imgui.new_enum("FocusedFlags",
            "None",
            0,
            "ChildWindows",
            1 << 0,
            "RootWindow",
            1 << 1,
            "AnyWindow",
            1 << 2,
            "RootAndChildWindows",
            ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows);

        imgui.new_enum("HoveredFlags",
            "None",
            0,
            "ChildWindows",
            1 << 0,
            "RootWindow",
            1 << 1,
            "AnyWindow",
            1 << 2,
            "AllowWhenBlockedByPopup",
            1 << 3,
            "AllowWhenBlockedByActiveItem",
            1 << 5,
            "AllowWhenOverlapped",
            1 << 6,
            "AllowWhenDisabled",
            1 << 7,
            "RectOnly",
            ImGuiHoveredFlags_AllowWhenBlockedByPopup |
            ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
            ImGuiHoveredFlags_AllowWhenOverlapped,
            "RootAndChildWindows",
            ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows);

        imgui.new_enum("InputTextFlags",
            "None",
            0,
            "CharsDecimal",
            1 << 0,
            "CharsHexadecimal",
            1 << 1,
            "CharsUppercase",
            1 << 2,
            "CharsNoBlank",
            1 << 3,
            "AutoSelectAll",
            1 << 4,
            "EnterReturnsTrue",
            1 << 5,
            "CallbackCompletion",
            1 << 6,
            "CallbackHistory",
            1 << 7,
            "CallbackAlways",
            1 << 8,
            "CallbackCharFilter",
            1 << 9,
            "AllowTabInput",
            1 << 10,
            "CtrlEnterForNewLine",
            1 << 11,
            "NoHorizontalScroll",
            1 << 12,
            "AlwaysInsertMode",
            1 << 13,
            "ReadOnly",
            1 << 14,
            "Password",
            1 << 15,
            "NoUndoRedo",
            1 << 16,
            "CharsScientific",
            1 << 17,
            "CallbackResize",
            1 << 18,
            "Multiline",
            1 << 20);

        imgui.new_enum("Key",
            "Tab",
            0,
            "LeftArrow",
            1,
            "RightArrow",
            2,
            "UpArrow",
            3,
            "DownArrow",
            4,
            "PageUp",
            5,
            "PageDown",
            6,
            "Home",
            7,
            "End",
            8,
            "Insert",
            9,
            "Delete",
            10,
            "Backspace",
            11,
            "Space",
            12,
            "Enter",
            13,
            "Escape",
            14,
            "A",
            15,
            "C",
            16,
            "V",
            17,
            "X",
            18,
            "Y",
            19,
            "Z",
            20,
            "COUNT",
            21);

        imgui.new_enum("MouseCursor",
            "None",
            -1,
            "Arrow",
            0,
            "TextInput",
            1,
            "ResizeAll",
            2,
            "ResizeNS",
            3,
            "ResizeEW",
            4,
            "ResizeNESW",
            5,
            "ResizeNWSE",
            6,
            "Hand",
            7,
            "COUNT",
            8);

        imgui.new_enum("NavInput",
            "Activate",
            0,
            "Cancel",
            1,
            "Input",
            2,
            "Menu",
            3,
            "DpadLeft",
            4,
            "DpadRight",
            5,
            "DpadUp",
            6,
            "DpadDown",
            7,
            "LStickLeft",
            8,
            "LStickRight",
            9,
            "LStickUp",
            10,
            "LStickDown",
            11,
            "FocusPrev",
            12,
            "FocusNext",
            13,
            "TweakSlow",
            14,
            "TweakFast",
            15,
            "KeyMenu_",
            16,
            "KeyLeft_",
            17,
            "KeyRight_",
            18,
            "KeyUp_",
            19,
            "KeyDown_",
            20,
            "COUNT",
            21,
            "InternalStart_",
            ImGuiNavInput_KeyMenu_);

        imgui.new_enum("SelectableFlags",
            "None",
            0,
            "DontClosePopups",
            1 << 0,
            "SpanAllColumns",
            1 << 1,
            "AllowDoubleClick",
            1 << 2,
            "Disabled",
            1 << 3);

        imgui.new_enum("StyleVar",
            "Alpha",
            0,
            "WindowPadding",
            1,
            "WindowRounding",
            2,
            "WindowBorderSize",
            3,
            "WindowMinSize",
            4,
            "WindowTitleAlign",
            5,
            "ChildRounding",
            6,
            "ChildBorderSize",
            7,
            "PopupRounding",
            8,
            "PopupBorderSize",
            9,
            "FramePadding",
            10,
            "FrameRounding",
            11,
            "FrameBorderSize",
            12,
            "ItemSpacing",
            13,
            "ItemInnerSpacing",
            14,
            "IndentSpacing",
            15,
            "ScrollbarSize",
            16,
            "ScrollbarRounding",
            17,
            "GrabMinSize",
            18,
            "GrabRounding",
            19,
            "TabRounding",
            20,
            "ButtonTextAlign",
            21,
            "SelectableTextAlign",
            22,
            "COUNT",
            23);

        imgui.new_enum("TabBarFlags",
            "None",
            0,
            "Reorderable",
            1 << 0,
            "AutoSelectNewTabs",
            1 << 1,
            "TabListPopupButton",
            1 << 2,
            "NoCloseWithMiddleMouseButton",
            1 << 3,
            "NoTabListScrollingButtons",
            1 << 4,
            "NoTooltip",
            1 << 5,
            "FittingPolicyResizeDown",
            1 << 6,
            "FittingPolicyScroll",
            1 << 7,
            "FittingPolicyMask_",
            ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll,
            "FittingPolicyDefault_",
            ImGuiTabBarFlags_FittingPolicyResizeDown);

        imgui.new_enum("TabItemFlags",
            "None",
            0,
            "UnsavedDocument",
            1 << 0,
            "SetSelected",
            1 << 1,
            "NoCloseWithMiddleMouseButton",
            1 << 2,
            "NoPushId",
            1 << 3);

        imgui.new_enum("TreeNodeFlags",
            "None",
            0,
            "Selected",
            1 << 0,
            "Framed",
            1 << 1,
            "AllowItemOverlap",
            1 << 2,
            "NoTreePushOnOpen",
            1 << 3,
            "NoAutoOpenOnLog",
            1 << 4,
            "DefaultOpen",
            1 << 5,
            "OpenOnDoubleClick",
            1 << 6,
            "OpenOnArrow",
            1 << 7,
            "Leaf",
            1 << 8,
            "Bullet",
            1 << 9,
            "FramePadding",
            1 << 10,
            "NavLeftJumpsBackHere",
            1 << 13,
            "CollapsingHeader",
            ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_NoAutoOpenOnLog);

        imgui.new_enum("WindowFlags",
            "None",
            0,
            "NoTitleBar",
            1 << 0,
            "NoResize",
            1 << 1,
            "NoMove",
            1 << 2,
            "NoScrollbar",
            1 << 3,
            "NoScrollWithMouse",
            1 << 4,
            "NoCollapse",
            1 << 5,
            "AlwaysAutoResize",
            1 << 6,
            "NoBackground",
            1 << 7,
            "NoSavedSettings",
            1 << 8,
            "NoMouseInputs",
            1 << 9,
            "MenuBar",
            1 << 10,
            "HorizontalScrollbar",
            1 << 11,
            "NoFocusOnAppearing",
            1 << 12,
            "NoBringToFrontOnFocus",
            1 << 13,
            "AlwaysVerticalScrollbar",
            1 << 14,
            "AlwaysHorizontalScrollbar",
            1 << 15,
            "AlwaysUseWindowPadding",
            1 << 16,
            "NoNavInputs",
            1 << 18,
            "NoNavFocus",
            1 << 19,
            "UnsavedDocument",
            1 << 20,
            "NoNav",
            ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
            "NoDecoration",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
            "NoInputs",
            ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs |
            ImGuiWindowFlags_NoNavFocus,
            "NavFlattened",
            1 << 23,
            "ChildWindow",
            1 << 24,
            "Tooltip",
            1 << 25,
            "Popup",
            1 << 26,
            "Modal",
            1 << 27,
            "ChildMenu",
            1 << 28);

        // some flags that are useful but live in the internal imgui for now as their api will most
        // likely change.
        imgui.new_enum("ItemFlags",
            "NoTabStop",
            1 << 0,
            "ButtonRepeat",
            1 << 1,
            "Disabled",
            1 << 2,
            "NoNav",
            1 << 3,
            "NoNavDefaultFocus",
            1 << 4,
            "SelectableDontClosePopup",
            1 << 5,
            "Default",
            0);

        //@TODO: Put a lot more stuff in here
        imgui.new_usertype<ImGuiIO>("IO",
            "new",
            sol::no_constructor,
            "configFlags",
            &ImGuiIO::ConfigFlags,
            "backendFlags",
            &ImGuiIO::BackendFlags,
            "displaySize",
            &ImGuiIO::DisplaySize,
            "deltaTime",
            &ImGuiIO::DeltaTime,
            "iniSavingRate",
            &ImGuiIO::IniSavingRate);

        imgui["getIO"] = ImGui::GetIO;
        imgui["getStyle"] = ImGui::GetStyle;
        imgui["newFrame"] = ImGui::NewFrame;
        imgui["endFrame"] = ImGui::EndFrame;
        imgui["render"] = ImGui::Render;
        imgui["getDrawData"] = ImGui::GetDrawData;

        imgui["showDemoWindow"] = []() {
            bool bShow;
            ImGui::ShowDemoWindow(&bShow);
            return bShow;
        };
        imgui["showAboutWindow"] = []() {
            bool bShow;
            ImGui::ShowAboutWindow(&bShow);
            return bShow;
        };
        imgui["showMetricsWindow"] = []() {
            bool bShow;
            ImGui::ShowMetricsWindow(&bShow);
            return bShow;
        };
        imgui["showStyleEditor"] =
            sol::overload(ImGui::ShowStyleEditor, []() { ImGui::ShowStyleEditor(); });
        imgui["showStyleEditor"] = ImGui::ShowStyleSelector;
        imgui["showFontSelector"] = ImGui::ShowFontSelector;
        imgui["showUserGuid"] = ImGui::ShowUserGuide;
        imgui["getVersion"] = ImGui::GetVersion;

        imgui["styleColorsDark"] =
            sol::overload(ImGui::StyleColorsDark, []() { ImGui::StyleColorsDark(); });
        imgui["styleColorsClassic"] =
            sol::overload(ImGui::StyleColorsClassic, []() { ImGui::StyleColorsClassic(); });
        imgui["styleColorsLight"] =
            sol::overload(ImGui::StyleColorsLight, []() { ImGui::StyleColorsLight(); });

        imgui["beginWindow"] = sol::overload([](const char * _str) { return ImGui::Begin(_str); },
            [](const char * _str, ImGuiWindowFlags _flags) {
            return ImGui::Begin(_str, NULL, _flags);
        });
        imgui["beginWindowClosable"] = sol::overload(
            [](const char * _str, sol::function _cb) {
            bool bOpen = true;
            if (ImGui::Begin(_str, &bOpen, 0))
                _cb(bOpen);
        },
            [](const char * _str, ImGuiWindowFlags _flags, sol::function _cb) {
            bool bOpen = true;
            if (ImGui::Begin(_str, &bOpen, _flags))
                _cb(bOpen);
        });

        imgui["endWindow"] = ImGui::End;
        imgui["beginChild"] = sol::overload(
            (bool(*)(const char *, const ImVec2 &, bool, ImGuiWindowFlags))ImGui::BeginChild,
            [](const char * _str) { return ImGui::BeginChild(_str); },
            [](const char * _str, const ImVec2 & _size) { return ImGui::BeginChild(_str, _size); },
            [](const char * _str, const ImVec2 & _size, bool _border) {
            return ImGui::BeginChild(_str, _size, _border);
        });
        imgui["endChild"] = ImGui::EndChild;

        imgui["isWindowAppearing"] = ImGui::IsWindowAppearing;
        imgui["isWindowCollapsed"] = ImGui::IsWindowCollapsed;
        imgui["isWindowFocused"] =
            sol::overload(ImGui::IsWindowFocused, []() { return ImGui::IsWindowFocused(); });
        imgui["isWindowHovered"] =
            sol::overload(ImGui::IsWindowHovered, []() { return ImGui::IsWindowHovered(); });

        imgui["getWindowPos"] = ImGui::GetWindowPos;
        imgui["getWindowSize"] = ImGui::GetWindowSize;
        imgui["getWindowWidth"] = ImGui::GetWindowWidth;
        imgui["getWindowHeight"] = ImGui::GetWindowHeight;
        imgui["getContentRegionMax"] = ImGui::GetContentRegionMax;
        imgui["getContentRegionAvail"] = ImGui::GetContentRegionAvail;
        imgui["getContentRegionAvailWidth"] = ImGui::GetContentRegionAvailWidth;
        imgui["getWindowContentRegionMin"] = ImGui::GetWindowContentRegionMin;
        imgui["getWindowContentRegionMax"] = ImGui::GetWindowContentRegionMax;
        imgui["getWindowContentRegionWidth"] = ImGui::GetWindowContentRegionWidth;

        imgui["setNextWindowPos"] = sol::overload(
            ImGui::SetNextWindowPos,
            [](const ImVec2 & _pos) { ImGui::SetNextWindowPos(_pos); },
            [](const ImVec2 & _pos, ImGuiCond _cond) { ImGui::SetNextWindowPos(_pos, _cond); });
        imgui["setNextWindowSize"] = sol::overload(
            ImGui::SetNextWindowSize, [](const ImVec2 & _size) { ImGui::SetNextWindowSize(_size); });
        // imgui["setNextWindowSizeConstraints"] =
        //     sol::overload(ImGui::SetNextWindowSizeConstraints,
        //                   [](const ImVec2 & _size_min, const ImVec2 & _size_max) {
        //                       ImGui::SetNextWindowSizeConstraints(_size_min, _size_max);
        //                   });
        imgui["setNextWindowContentSize"] = ImGui::SetNextWindowContentSize;
        imgui["setNextWindowCollapsed"] =
            sol::overload(ImGui::SetNextWindowCollapsed,
                [](bool _collapsed) { ImGui::SetNextWindowCollapsed(_collapsed); });
        imgui["setNextWindowFocus"] = ImGui::SetNextWindowFocus;
        imgui["setNextWindowBgAlpha"] = ImGui::SetNextWindowBgAlpha;

        imgui["setWindowCollapsed"] = sol::overload(
            (void(*)(bool, ImGuiCond))ImGui::SetWindowCollapsed,
            [](bool _collapsed) { ImGui::SetWindowCollapsed(_collapsed); },
            (void(*)(const char *, bool, ImGuiCond))ImGui::SetWindowCollapsed,
            [](const char * _name, bool _collapsed) { ImGui::SetWindowCollapsed(_name, _collapsed); });
        imgui["setWindowFocus"] = sol::overload((void(*)(const char *))ImGui::SetWindowFocus,
            (void(*)())ImGui::SetWindowFocus);
        imgui["setWindowFontScale"] = ImGui::SetWindowFontScale;
        imgui["setWindowPos"] = sol::overload(
            (void(*)(const ImVec2 &, ImGuiCond))ImGui::SetWindowPos,
            [](const ImVec2 & _pos) { ImGui::SetWindowPos(_pos); },
            (void(*)(const char *, const ImVec2 &, ImGuiCond))ImGui::SetWindowPos,
            [](const char * _name, const ImVec2 & _pos) { ImGui::SetWindowPos(_name, _pos); });
        imgui["setWindowSize"] = sol::overload(
            (void(*)(const ImVec2 &, ImGuiCond))ImGui::SetWindowSize,
            [](const ImVec2 & _size) { ImGui::SetWindowSize(_size); },
            (void(*)(const char *, const ImVec2 &, ImGuiCond))ImGui::SetWindowSize,
            [](const char * _name, const ImVec2 & _size) { ImGui::SetWindowSize(_name, _size); });
        imgui["setWindowCollapsed"] = sol::overload(
            (void(*)(bool, ImGuiCond))ImGui::SetWindowCollapsed,
            [](bool _collapsed) { ImGui::SetWindowCollapsed(_collapsed); },
            (void(*)(const char *, bool, ImGuiCond))ImGui::SetWindowCollapsed,
            [](const char * _name, bool _collapsed) { ImGui::SetWindowCollapsed(_name, _collapsed); });
        imgui["setWindowFocus"] = sol::overload((void(*)())ImGui::SetWindowFocus,
            (void(*)(const char *))ImGui::SetWindowFocus);

        imgui["getScrollX"] = ImGui::GetScrollX;
        imgui["getScrollY"] = ImGui::GetScrollY;
        imgui["getScrollMaxX"] = ImGui::GetScrollMaxX;
        imgui["getScrollMaxY"] = ImGui::GetScrollMaxY;
        imgui["getScrollX"] = ImGui::SetScrollX;
        imgui["getScrollY"] = ImGui::SetScrollY;
        imgui["setScrollHereY"] = ImGui::SetScrollHereY;
        imgui["setScrollFromPosY"] = ImGui::SetScrollFromPosY;

        imgui["pushFont"] = ImGui::PushFont;
        imgui["popFont"] = ImGui::PopFont;
        imgui["pushStyleColor"] =
            sol::overload((void(*)(ImGuiCol, ImU32))ImGui::PushStyleColor,
            (void(*)(ImGuiCol, const ImVec4 &))ImGui::PushStyleColor);
        imgui["popStyleColor"] = sol::overload(ImGui::PopStyleColor, []() { ImGui::PopStyleColor(); });
        imgui["pushStyleVar"] = sol::overload((void(*)(ImGuiStyleVar, float))ImGui::PushStyleVar,
                                              (void(*)(ImGuiStyleVar, const ImVec2 &))ImGui::PushStyleVar);
        imgui["popStyleVar"] = sol::overload(ImGui::PopStyleVar, []() { ImGui::PopStyleVar(); });
        imgui["getStyleColorVec4"] = ImGui::GetStyleColorVec4;
        imgui["getFont"] = ImGui::GetFont;
        imgui["getFontSize"] = ImGui::GetFontSize;
        imgui["getFontTexUvWhitePixel"] = ImGui::GetFontTexUvWhitePixel;

        imgui["pushItemWidth"] = ImGui::PushItemWidth;
        imgui["popItemWidth"] = ImGui::PopItemWidth;
        imgui["calcItemWidth"] = ImGui::CalcItemWidth;
        imgui["pushTextWrapPos"] =
            sol::overload(ImGui::PushTextWrapPos, []() { ImGui::PushTextWrapPos(); });
        imgui["popTextWrapPos"] = ImGui::PopTextWrapPos;
        imgui["pushAllowKeyboardFocus"] = ImGui::PushAllowKeyboardFocus;
        imgui["popAllowKeyboardFocus"] = ImGui::PopAllowKeyboardFocus;
        imgui["pushButtonRepeat"] = ImGui::PushButtonRepeat;
        imgui["popButtonRepeat"] = ImGui::PopButtonRepeat;

        imgui["separator"] = ImGui::Separator;
        imgui["sameLine"] = sol::overload(ImGui::SameLine,
            []() { ImGui::SameLine(); },
            [](float _localPos) { ImGui::SameLine(_localPos); });
        imgui["newLine"] = ImGui::NewLine;
        imgui["spacing"] = ImGui::Spacing;
        imgui["dummy"] = ImGui::Dummy;
        imgui["indent"] = sol::overload(ImGui::Indent, []() { ImGui::Indent(); });
        imgui["unindent"] = sol::overload(ImGui::Unindent, []() { ImGui::Unindent(); });
        imgui["beginGroup"] = ImGui::BeginGroup;
        imgui["endGroup"] = ImGui::EndGroup;
        imgui["getCursorPos"] = ImGui::GetCursorPos;
        imgui["getCursorPosX"] = ImGui::GetCursorPosX;
        imgui["getCursorPosY"] = ImGui::GetCursorPosY;
        imgui["setCursorPos"] = ImGui::SetCursorPos;
        imgui["setCursorPosX"] = ImGui::SetCursorPosX;
        imgui["setCursorPosY"] = ImGui::SetCursorPosY;
        imgui["getCursorStartPos"] = ImGui::GetCursorStartPos;
        imgui["getCursorScreenPos"] = ImGui::GetCursorScreenPos;
        imgui["setCursorScreenPos"] = ImGui::SetCursorScreenPos;
        imgui["alignTextToFramePadding"] = ImGui::AlignTextToFramePadding;
        imgui["getTextLineHeight"] = ImGui::GetTextLineHeight;
        imgui["getTextLineHeightWithSpacing"] = ImGui::GetTextLineHeightWithSpacing;
        imgui["getFrameHeight"] = ImGui::GetFrameHeight;
        imgui["getFrameHeightWithSpacing"] = ImGui::GetFrameHeightWithSpacing;

        imgui["pushID"] =
            sol::overload((void(*)(const char *))ImGui::PushID, (void(*)(int))ImGui::PushID);
        imgui["popID"] = ImGui::PopID;
        imgui["getID"] = (ImGuiID(*)(const char *))ImGui::GetID;

        imgui["text"] = [](const char * _text) { ImGui::TextUnformatted(_text); };
        imgui["textColored"] = [](const ImVec4 & col, const char * _text) {
            ImGui::TextColored(col, "%s", _text);
        };
        imgui["textDisabled"] = [](const char * _text) { ImGui::TextDisabled("%s", _text); };
        imgui["textWrapped"] = [](const char * _text) { ImGui::TextWrapped("%s", _text); };
        imgui["labelText"] = [](const char * _label, const char * _text) {
            ImGui::LabelText(_label, "%s", _text);
        };
        imgui["bulletText"] = [](const char * _text) { ImGui::BulletText("%s", _text); };

        imgui["button"] =
            sol::overload(ImGui::Button, [](const char * _label) { return ImGui::Button(_label); });
        imgui["smallButton"] = ImGui::SmallButton;
        imgui["invisibleButton"] = ImGui::InvisibleButton;
        imgui["arrowButton"] = ImGui::ArrowButton;
        // Image
        // ImageButton
        imgui["checkbox"] = [](const char * _name, bool _bSelected, sol::function _cb) {
            if (ImGui::Checkbox(_name, &_bSelected))
            {
                _cb(_bSelected);
            }
        };
        imgui["radioButton"] = (bool(*)(const char *, bool))ImGui::RadioButton;
        imgui["progressBar"] = sol::overload(ImGui::ProgressBar,
            [](float _fraction) { ImGui::ProgressBar(_fraction); },
            [](float _fraction, const ImVec2 & _size_arg) {
            ImGui::ProgressBar(_fraction, _size_arg);
        });
        imgui["bullet"] = ImGui::Bullet;

        imgui["beginCombo"] =
            sol::overload(ImGui::BeginCombo, [](const char * _label, const char * _preview_value) {
            return ImGui::BeginCombo(_label, _preview_value);
        });
        imgui["endCombo"] = ImGui::EndCombo;

        imgui["dragFloat"] = sol::overload(
            [](const char * _label, float _currentValue, sol::function _cb) {
            if (ImGui::DragFloat(_label, &_currentValue))
                _cb(_currentValue);
        },
            [](const char * _label, float _currentValue, float _v_speed, sol::function _cb) {
            if (ImGui::DragFloat(_label, &_currentValue, _v_speed))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_speed,
                float _v_min,
                sol::function _cb) {
            if (ImGui::DragFloat(_label, &_currentValue, _v_speed, _v_min))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_speed,
                float _v_min,
                float _v_max,
                sol::function _cb) {
            if (ImGui::DragFloat(_label, &_currentValue, _v_speed, _v_min, _v_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_speed,
                float _v_min,
                float _v_max,
                const char * _fmt,
                sol::function _cb) {
            if (ImGui::DragFloat(_label, &_currentValue, _v_speed, _v_min, _v_max, _fmt))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_speed,
                float _v_min,
                float _v_max,
                const char * _fmt,
                float _power,
                sol::function _cb) {
            if (ImGui::DragFloat(_label, &_currentValue, _v_speed, _v_min, _v_max, _fmt, _power))
                _cb(_currentValue);
        });

        imgui["dragInt"] = sol::overload(
            [](const char * _label, int _currentValue, sol::function _cb) {
            if (ImGui::DragInt(_label, &_currentValue))
                _cb(_currentValue);
        },
            [](const char * _label, int _currentValue, float _v_speed, sol::function _cb) {
            if (ImGui::DragInt(_label, &_currentValue, _v_speed))
                _cb(_currentValue);
        },
            [](const char * _label, int _currentValue, float _v_speed, int _v_min, sol::function _cb) {
            if (ImGui::DragInt(_label, &_currentValue, _v_speed, _v_min))
                _cb(_currentValue);
        },
            [](const char * _label,
                int _currentValue,
                float _v_speed,
                int _v_min,
                int _v_max,
                sol::function _cb) {
            if (ImGui::DragInt(_label, &_currentValue, _v_speed, _v_min, _v_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                int _currentValue,
                float _v_speed,
                int _v_min,
                int _v_max,
                const char * _fmt,
                sol::function _cb) {
            if (ImGui::DragInt(_label, &_currentValue, _v_speed, _v_min, _v_max, _fmt))
                _cb(_currentValue);
        });

        imgui["sliderFloat"] = sol::overload(
            [](const char * _label,
                float _currentValue,
                float _v_min,
                float _v_max,
                sol::function _cb) {
            if (ImGui::SliderFloat(_label, &_currentValue, _v_min, _v_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_min,
                float _v_max,
                const char * _format,
                sol::function _cb) {
            if (ImGui::SliderFloat(_label, &_currentValue, _v_min, _v_max, _format))
                _cb(_currentValue);
        });

        imgui["sliderAngle"] = sol::overload(
            [](const char * _label, float _currentValue, sol::function _cb) {
            if (ImGui::SliderAngle(_label, &_currentValue))
                _cb(_currentValue);
        },
            [](const char * _label, float _currentValue, float _v_degrees_min, sol::function _cb) {
            if (ImGui::SliderAngle(_label, &_currentValue, _v_degrees_min))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_degrees_min,
                float _v_degrees_max,
                sol::function _cb) {
            if (ImGui::SliderAngle(_label, &_currentValue, _v_degrees_min, _v_degrees_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                float _currentValue,
                float _v_degrees_min,
                float _v_degrees_max,
                const char * _format,
                sol::function _cb) {
            if (ImGui::SliderAngle(_label, &_currentValue, _v_degrees_min, _v_degrees_max, _format))
                _cb(_currentValue);
        });

        imgui["sliderInt"] = sol::overload(
            [](const char * _label, int _currentValue, int _v_min, int _v_max, sol::function _cb) {
            if (ImGui::SliderInt(_label, &_currentValue, _v_min, _v_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                int _currentValue,
                int _v_min,
                int _v_max,
                const char * _format,
                sol::function _cb) {
            if (ImGui::SliderInt(_label, &_currentValue, _v_min, _v_max, _format))
                _cb(_currentValue);
        });

        imgui["vSliderFloat"] = sol::overload(
            [](const char * _label,
                const ImVec2 & _size,
                float _currentValue,
                float _v_min,
                float _v_max,
                sol::function _cb) {
            if (ImGui::VSliderFloat(_label, _size, &_currentValue, _v_min, _v_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                const ImVec2 & _size,
                float _currentValue,
                float _v_min,
                float _v_max,
                const char * _format,
                sol::function _cb) {
            if (ImGui::VSliderFloat(_label, _size, &_currentValue, _v_min, _v_max, _format))
                _cb(_currentValue);
        });

        imgui["vSliderInt"] = sol::overload(
            [](const char * _label,
                const ImVec2 & _size,
                int _currentValue,
                int _v_min,
                int _v_max,
                sol::function _cb) {
            if (ImGui::VSliderInt(_label, _size, &_currentValue, _v_min, _v_max))
                _cb(_currentValue);
        },
            [](const char * _label,
                const ImVec2 & _size,
                int _currentValue,
                int _v_min,
                int _v_max,
                const char * _format,
                sol::function _cb) {
            if (ImGui::VSliderInt(_label, _size, &_currentValue, _v_min, _v_max, _format))
                _cb(_currentValue);
        });

        //@TODO: add overloads
        //imgui["inputText"] = [](const char * _label, int _capacity, sol::function _cb) {
        //    //@TODO: This is kinda gnarly
        //    static stick::HashMap<const char *, stick::String> s_stringStorage;
        //    auto it = s_stringStorage.find(_label);
        //    stick::String * str;
        //    if (it != s_stringStorage.end())
        //        str = &it->value;
        //    else
        //    {
        //        auto res = s_stringStorage.insert(_label, String());
        //        str = &res.iterator->value;
        //    }

        //    str->reserve(_capacity);
        //    if (ImGui::InputText(_label, &(*str)[0], str->capacity()))
        //        _cb(str->cString());
        //};
        //imgui["inputTextMultiline"] = [](const char * _label, int _capacity, sol::function _cb) {
        //    //@TODO: This is kinda gnarly
        //    static stick::HashMap<const char *, stick::String> s_stringStorage;
        //    auto it = s_stringStorage.find(_label);
        //    stick::String * str;
        //    if (it != s_stringStorage.end())
        //        str = &it->value;
        //    else
        //    {
        //        auto res = s_stringStorage.insert(_label, String());
        //        str = &res.iterator->value;
        //    }

        //    str->reserve(_capacity);
        //    if (ImGui::InputTextMultiline(_label, &(*str)[0], str->capacity()))
        //        _cb(str->cString());
        //};

        imgui["inputFloat"] = sol::overload(
            [](const char * _label, float _current, sol::function _cb) {
            if (ImGui::InputFloat(_label, &_current))
                _cb(_current);
        },
            [](const char * _label, float _current, float _step, sol::function _cb) {
            if (ImGui::InputFloat(_label, &_current, _step))
                _cb(_current);
        },
            [](const char * _label, float _current, float _step, float _stepFast, sol::function _cb) {
            if (ImGui::InputFloat(_label, &_current, _step, _stepFast))
                _cb(_current);
        },
            [](const char * _label,
                float _current,
                float _step,
                float _stepFast,
                const char * _fmt,
                sol::function _cb) {
            if (ImGui::InputFloat(_label, &_current, _step, _stepFast, _fmt))
                _cb(_current);
        },
            [](const char * _label,
                float _current,
                float _step,
                float _stepFast,
                const char * _fmt,
                ImGuiInputTextFlags _flags,
                sol::function _cb) {
            if (ImGui::InputFloat(_label, &_current, _step, _stepFast, _fmt, _flags))
                _cb(_current);
        });

        imgui["inputInt"] = sol::overload(
            [](const char * _label, int _current, sol::function _cb) {
            if (ImGui::InputInt(_label, &_current))
                _cb(_current);
        },
            [](const char * _label, int _current, int _step, sol::function _cb) {
            if (ImGui::InputInt(_label, &_current, _step))
                _cb(_current);
        },
            [](const char * _label, int _current, int _step, int _stepFast, sol::function _cb) {
            if (ImGui::InputInt(_label, &_current, _step, _stepFast))
                _cb(_current);
        },
            [](const char * _label,
                int _current,
                int _step,
                int _stepFast,
                ImGuiInputTextFlags _flags,
                sol::function _cb) {
            if (ImGui::InputInt(_label, &_current, _step, _stepFast, _flags))
                _cb(_current);
        });

        //@TODO: Overload
        imgui["colorEdit3"] = [](const char * _label, float _r, float _g, float _b, sol::function _cb) {
            float tmp[3] = { _r, _g, _b };
            if (ImGui::ColorEdit3(_label, tmp))
                _cb(tmp[0], tmp[1], tmp[2]);
        };
        imgui["colorEdit4"] =
            sol::overload([](const char * _label, float _r, float _g, float _b, float _a, sol::function _cb) {
            float tmp[4] = { _r, _g, _b, _a };
            if (ImGui::ColorEdit4(_label, tmp))
                _cb(tmp[0], tmp[1], tmp[2], tmp[3]);
        },
                [](const char * _label, float _r, float _g, float _b, float _a, ImGuiColorEditFlags _flags, sol::function _cb) {
            float tmp[4] = { _r, _g, _b, _a };
            if (ImGui::ColorEdit4(_label, tmp, _flags))
                _cb(tmp[0], tmp[1], tmp[2], tmp[3]);
        });
        //@TODO: Overload
        imgui["colorPicker3"] =
            [](const char * _label, float _r, float _g, float _b, sol::function _cb) {
            float tmp[3] = { _r, _g, _b };
            if (ImGui::ColorPicker3(_label, tmp))
                _cb(tmp[0], tmp[1], tmp[2]);
        };
        //@TODO: Overload
        imgui["colorPicker3"] =
            [](const char * _label, float _r, float _g, float _b, float _a, sol::function _cb) {
            float tmp[4] = { _r, _g, _b, _a };
            if (ImGui::ColorPicker4(_label, tmp))
                _cb(tmp[0], tmp[1], tmp[2], tmp[3]);
        };
        //@TODO: Overload
        imgui["colorButton"] =
            [](const char * _label, float _r, float _g, float _b, float _a, sol::function _cb) {
            ImVec4 tmp = { _r, _g, _b, _a };
            if (ImGui::ColorButton(_label, tmp))
                _cb(tmp.x, tmp.y, tmp.z, tmp.y);
        };

        imgui["setColorEditOptions"] = ImGui::SetColorEditOptions;

        imgui["treeNode"] = (bool(*)(const char *))ImGui::TreeNode;
        imgui["treeNodeEx"] =
            sol::overload((bool(*)(const char *, ImGuiTreeNodeFlags))ImGui::TreeNodeEx,
                [](const char * _label) { return ImGui::TreeNodeEx(_label); });
        imgui["treePush"] = (void(*)(const char *))ImGui::TreePush;
        imgui["treePop"] = ImGui::TreePop;
        imgui["treeAdvanceToLabelPos"] = ImGui::TreeAdvanceToLabelPos;
        imgui["getTreeNodeToLabelSpacing"] = ImGui::GetTreeNodeToLabelSpacing;
        imgui["setNextTreeNodeOpen"] = sol::overload(
            ImGui::SetNextTreeNodeOpen, [](bool _is_open) { ImGui::SetNextTreeNodeOpen(_is_open); });
        imgui["collapsingHeader"] =
            sol::overload((bool(*)(const char *, ImGuiTreeNodeFlags))ImGui::CollapsingHeader,
                [](const char * _label) { ImGui::CollapsingHeader(_label); });
        //@TODO: Overloads
        imgui["collapsingHeaderClosable"] = sol::overload([](const char * _label, sol::function _cb) {
            bool bOpen = true;
            ImGui::CollapsingHeader(_label, &bOpen);
            _cb(bOpen);
        });

        imgui["selectable"] = sol::overload(
            [](const char * _label, bool _bSelected, sol::function _cb) {
            if (ImGui::Selectable(_label, &_bSelected))
                _cb(_bSelected);
        },
            [](const char * _label, bool _bSelected, ImGuiSelectableFlags _flags, sol::function _cb) {
            if (ImGui::Selectable(_label, &_bSelected, _flags))
                _cb(_bSelected);
        });

        // PlotLines

        imgui["value"] =
            sol::overload((void(*)(const char *, bool))ImGui::Value,
            (void(*)(const char *, int))ImGui::Value,
                (void(*)(const char *, float, const char *))ImGui::Value,
                [](const char * _pref, float _val) { ImGui::Value(_pref, _val); });

        imgui["beginMainMenuBar"] = ImGui::BeginMainMenuBar;
        imgui["endMainMenuBar"] = ImGui::EndMainMenuBar;
        imgui["beginMenuBar"] = ImGui::BeginMenuBar;
        imgui["endMenuBar"] = ImGui::EndMenuBar;
        imgui["beginMenu"] = sol::overload(
            ImGui::BeginMenu, [](const char * _label) { return ImGui::BeginMenu(_label); });
        imgui["endMenu"] = ImGui::EndMenu;
        imgui["menuItem"] =
            sol::overload((bool(*)(const char *, const char *, bool, bool))ImGui::MenuItem,
                [](const char * _label) { return ImGui::MenuItem(_label); },
                [](const char * _label, const char * _shortcut) {
            return ImGui::MenuItem(_label, _shortcut);
        },
                [](const char * _label, const char * _shortcut, bool _selected) {
            return ImGui::MenuItem(_label, _shortcut, _selected);
        });

        imgui["beginTooltip"] = ImGui::BeginTooltip;
        imgui["endTooltip"] = ImGui::EndTooltip;
        imgui["setTooltip"] = [](const char * _txt) { ImGui::SetTooltip("%s", _txt); };

        imgui["openPopup"] = ImGui::OpenPopup;
        imgui["beginPopup"] =
            sol::overload(ImGui::BeginPopup, [](const char * _str) { return ImGui::BeginPopup(_str); });
        imgui["beginPopupContextItem"] =
            sol::overload(ImGui::BeginPopupContextItem,
                [](const char * _str) { return ImGui::BeginPopupContextItem(_str); },
                          []() { return ImGui::BeginPopupContextItem(); });
        imgui["beginPopupContextWindow"] =
            sol::overload(ImGui::BeginPopupContextWindow,
                [](const char * _str) { return ImGui::BeginPopupContextWindow(_str); },
                [](const char * _str, int _mouse_button) {
            return ImGui::BeginPopupContextWindow(_str, _mouse_button);
        }, []() { return ImGui::BeginPopupContextWindow(); });
        
        imgui["beginPopupContextVoid"] =
            sol::overload(ImGui::BeginPopupContextVoid,
                [](const char * _str) { return ImGui::BeginPopupContextVoid(_str); },
                          []() { return ImGui::BeginPopupContextVoid(); });
        imgui["beginPopupModal"] = sol::overload(
                [](const char * _name, sol::function _cb) {
            if (ImGui::BeginPopupModal(_name, NULL))
                _cb();
        },
                [](const char * _name, ImGuiWindowFlags _flags, sol::function _cb) {
            if (ImGui::BeginPopupModal(_name, NULL, _flags))
                _cb();
            });
        imgui["endPopup"] = ImGui::EndPopup;
        imgui["openPopupOnItemClick"] =
            sol::overload(ImGui::OpenPopupOnItemClick,
                [](const char * _name) { return ImGui::OpenPopupOnItemClick(_name); });
        imgui["isPopupOpen"] = (bool(*)(const char *))ImGui::IsPopupOpen;
        imgui["closeCurrentPopup"] = ImGui::CloseCurrentPopup;

        imgui["columns"] = sol::overload(
            ImGui::Columns,
            [](int _c) { ImGui::Columns(_c); },
            [](int _c, const char * _id) { ImGui::Columns(_c, _id); },
            [](int _c, const char * _id, bool _bBorder) { ImGui::Columns(_c, _id, _bBorder); });
        imgui["nextColumn"] = ImGui::NextColumn;
        imgui["endPopup"] = ImGui::EndPopup;
        imgui["GetColumnIndex"] = ImGui::GetColumnIndex;
        imgui["GetColumnWidth"] =
            sol::overload(ImGui::GetColumnWidth, []() { return ImGui::GetColumnWidth(); });
        imgui["setColumnWidth"] = ImGui::SetColumnWidth;
        imgui["GetColumnOffset"] =
            sol::overload(ImGui::GetColumnOffset, []() { return ImGui::GetColumnOffset(); });
        imgui["setColumnOffset"] = ImGui::SetColumnOffset;
        imgui["GetColumnsCount"] = ImGui::GetColumnsCount;

        imgui["beginTabBar"] =
            sol::overload(ImGui::BeginTabBar, [](const char * _id) { return ImGui::BeginTabBar(_id); });
        imgui["endTabBar"] = ImGui::EndTabBar;
        imgui["beginTabItem"] = sol::overload(
            [](const char * _name, sol::function _cb) {
            bool bOpen = true;
            if (ImGui::BeginTabItem(_name, &bOpen))
                _cb(bOpen);
        },
            [](const char * _name, ImGuiTabItemFlags _flags, sol::function _cb) {
            bool bOpen = true;
            if (ImGui::BeginTabItem(_name, &bOpen, _flags))
                _cb(bOpen);
        });
        imgui["endTabItem"] = ImGui::EndTabItem;
        imgui["setTabItemClosed"] = ImGui::SetTabItemClosed;

        imgui["beginDragDropSource"] =
            sol::overload(ImGui::BeginDragDropSource, []() { return ImGui::BeginDragDropSource(); });
        imgui["setDragDropPayload"] = [](const char * _name, const sol::object & _obj) {
            static sol::object s_obj;
            s_obj = _obj;
            sol::object * ptr = &s_obj;
            return ImGui::SetDragDropPayload(_name, &ptr, sizeof(ptr));
        };

        imgui["test"] = [](const sol::object & _obj, sol::this_state _s) {
            sol::state_view view(_s);
            printf("typename %s\n", sol::type_name(_s, _obj.get_type()).c_str());
            if (_obj.get_type() == sol::type::userdata)
            {
                int c = _obj.push();
                printf("usertype: %s\n", sol::associated_type_name(_s, -1, _obj.get_type()).c_str());
                lua_pop(_s, c);
            }
        };
        imgui["endDragDropSource"] = ImGui::EndDragDropSource;
        imgui["beginDragDropTarget"] = ImGui::BeginDragDropTarget;
        //@TODO: expose ImGuiPayload
        // imgui["acceptDragDropPayload"] = sol::overload(ImGui::AcceptDragDropPayload, [](const char *
        // _str) { return ImGui::AcceptDragDropPayload(_str); });
        imgui["acceptDragDropPayload"] = [](const char * _name, sol::function _cb, sol::this_state _s) {
            if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(_name))
            {
                LUMOS_ASSERT(payload->DataSize == sizeof(sol::object *), "Incorect Payload");
                const sol::object * obj = (*(const sol::object **)payload->Data);
                _cb(*obj);
            }
        };
        imgui["endDragDropTarget"] = ImGui::EndDragDropTarget;
        imgui["getDragDropPayload"] = ImGui::GetDragDropPayload;

        imgui["pushClipRect"] = ImGui::PushClipRect;
        imgui["popClipRect"] = ImGui::PopClipRect;

        imgui["setItemDefaultFocus"] = ImGui::SetItemDefaultFocus;
        imgui["setKeyboardFocusHere"] = ImGui::SetKeyboardFocusHere;

        imgui["isItemHovered"] =
            sol::overload(ImGui::IsItemHovered, []() { return ImGui::IsItemHovered(); });
        imgui["isItemActive"] = ImGui::IsItemActive;
        imgui["isItemFocused"] = ImGui::IsItemFocused;
        imgui["isItemClicked"] =
            sol::overload(ImGui::IsItemClicked, []() { return ImGui::IsItemClicked(); });
        imgui["isItemVisible"] = ImGui::IsItemVisible;
        imgui["isItemEdited"] = ImGui::IsItemEdited;
        imgui["isItemActivated"] = ImGui::IsItemActivated;
        imgui["isItemDeactivated"] = ImGui::IsItemDeactivated;
        imgui["isItemDeactivatedAfterEdit"] = ImGui::IsItemDeactivatedAfterEdit;
        imgui["isAnyItemHovered"] = ImGui::IsAnyItemHovered;
        imgui["isAnyItemActive"] = ImGui::IsAnyItemActive;
        imgui["isAnyItemFocused"] = ImGui::IsAnyItemFocused;
        imgui["getItemRectMin"] = ImGui::GetItemRectMin;
        imgui["getItemRectMax"] = ImGui::GetItemRectMax;
        imgui["getItemRectSize"] = ImGui::GetItemRectSize;
        imgui["setItemAllowOverlap"] = ImGui::SetItemAllowOverlap;

        imgui["getKeyIndex"] = ImGui::GetKeyIndex;
        imgui["isKeyDown"] = ImGui::IsKeyDown;
        imgui["isKeyPressed"] = ImGui::IsKeyPressed;
        imgui["isKeyReleased"] = ImGui::IsKeyReleased;
        imgui["getKeyPressedAmount"] = ImGui::GetKeyPressedAmount;
        imgui["isMouseDown"] = ImGui::IsMouseDown;
        imgui["isAnyMouseDown"] = ImGui::IsAnyMouseDown;
        imgui["isMouseClicked"] = ImGui::IsMouseClicked;
        imgui["isMouseDoubleClicked"] = ImGui::IsMouseDoubleClicked;
        imgui["isMouseReleased"] = ImGui::IsMouseReleased;
        imgui["isMouseDragging"] = ImGui::IsMouseDragging;
        imgui["isMouseHoveringRect"] = ImGui::IsMouseHoveringRect;
        imgui["getMousePos"] = ImGui::GetMousePos;
        imgui["getMousePosOnOpeningCurrentPopup"] = ImGui::GetMousePosOnOpeningCurrentPopup;
        imgui["getMouseDragDelta"] = ImGui::GetMouseDragDelta;
        imgui["resetMouseDragDelta"] = ImGui::ResetMouseDragDelta;
        imgui["getMouseCursor"] = ImGui::GetMouseCursor;
        imgui["setMouseCursor"] = ImGui::SetMouseCursor;
        imgui["captureKeyboardFromApp"] = ImGui::CaptureKeyboardFromApp;
        imgui["captureMouseFromApp"] = ImGui::CaptureMouseFromApp;
    }


}
