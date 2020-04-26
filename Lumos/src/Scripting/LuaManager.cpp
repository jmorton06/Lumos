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
#include "Graphics/API/Texture.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Utilities/RandomNumberGenerator.h"

#include "ImGuiLua.h"

#include <Box2D/Box2D.h>
#include <imgui/imgui.h>

namespace Lumos
{
	LuaManager::LuaManager() : m_State(nullptr)
	{
	}

	void LuaManager::OnInit()
	{
        m_State.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);

        BindInputLua(m_State);
		BindMathsLua(m_State);
		BindImGuiLua(m_State);
		BindECSLua(m_State);
        BindLogLua(m_State);
        BindSceneLua(m_State);
        BindPhysicsObjects(m_State);
	}

	LuaManager::~LuaManager()
	{
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

		m_State.script_file(physicalPath);
		windowProperties.Title = m_State.get<std::string>("title");
		windowProperties.Width = m_State.get<int>("width");
		windowProperties.Height = m_State.get<int>("height");
		windowProperties.RenderAPI = m_State.get<int>("renderAPI");
		windowProperties.Fullscreen = m_State.get<bool>("fullscreen");
		windowProperties.Borderless = m_State.get<bool>("borderless");

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

    void LuaManager::BindLogLua(sol::state& state)
    {
        auto log = state.create_table("Log");

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

    void LuaManager::BindInputLua(sol::state& state)
    {
        auto input = state["Input"].get_or_create< sol::table >();

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
        state.new_enum< Lumos::InputCode::Key, false >( "Key", keyItems ); // false makes it read/write in Lua, but its faster

        std::initializer_list< std::pair< sol::string_view, Lumos::InputCode::MouseKey > > mouseItems =
        {
                { "Left", Lumos::InputCode::MouseKey::ButtonLeft },
                { "Right", Lumos::InputCode::MouseKey::ButtonRight },
                { "Middle", Lumos::InputCode::MouseKey::ButtonMiddle },
        };
        state.new_enum< Lumos::InputCode::MouseKey, false >( "MouseButton", mouseItems );
    }

    void LuaManager::BindPhysicsObjects(sol::state& state)
    {
        sol::usertype< PhysicsObjectParamaters > physicsObjectParamaters_type = state.new_usertype< PhysicsObjectParamaters >( "PhysicsObjectParamaters" );
        physicsObjectParamaters_type["mass"] = &PhysicsObjectParamaters::mass;
        physicsObjectParamaters_type["shape"] = &PhysicsObjectParamaters::shape;
        physicsObjectParamaters_type["position"] = &PhysicsObjectParamaters::position;
        physicsObjectParamaters_type["scale"] = &PhysicsObjectParamaters::scale;
        physicsObjectParamaters_type["isStatic"] = &PhysicsObjectParamaters::isStatic;

        std::initializer_list< std::pair< sol::string_view, Shape > > shapes =
        {
            { "Square", Shape::Square  },
            { "Circle", Shape::Circle  },
            { "Custom", Shape::Custom  }
        };
        state.new_enum< Shape, false >( "Shape", shapes );
        
        sol::usertype< PhysicsObject2D > physics2D_type = state.new_usertype< PhysicsObject2D >( "PhysicsObject2D" );
        state.set_function( "SetForce", &PhysicsObject2D::SetForce );
        state.set_function( "SetPosition", &PhysicsObject2D::SetPosition );
        state.set_function( "SetLinearVelocity", &PhysicsObject2D::SetLinearVelocity );
        state.set_function( "SetAngularVelocity", &PhysicsObject2D::SetAngularVelocity );
        state.set_function( "SetFriction", &PhysicsObject2D::SetFriction );
        state.set_function( "GetPosition", &PhysicsObject2D::GetPosition );
        state.set_function( "GetAngle", &PhysicsObject2D::GetAngle );
        state.set_function( "GetFriction", &PhysicsObject2D::GetFriction );
        state.set_function( "GetIsStatic", &PhysicsObject2D::GetIsStatic );
        state.set_function( "GetB2Body", &PhysicsObject2D::GetB2Body );
        
        state.new_enum("b2BodyType"
            ,"b2_staticBody"
            ,b2BodyType::b2_staticBody
            ,"b2_kinematicBody"
            ,b2BodyType::b2_kinematicBody
            ,"b2_dynamicBody"
            ,b2BodyType::b2_dynamicBody
        );
        
        state.new_usertype<b2BodyDef>("b2BodyDef"
        // fields
            ,"type"
            ,&b2BodyDef::type
            ,"position"
            ,&b2BodyDef::position
            ,"angle"
            ,&b2BodyDef::angle
            ,"linearVelocity"
            ,&b2BodyDef::linearVelocity
            ,"angularVelocity"
            ,&b2BodyDef::angularVelocity
            ,"linearDamping"
            ,&b2BodyDef::linearDamping
            ,"angularDamping"
            ,&b2BodyDef::angularDamping
            ,"allowSleep"
            ,&b2BodyDef::allowSleep
            ,"awake"
            ,&b2BodyDef::awake
            ,"fixedRotation"
            ,&b2BodyDef::fixedRotation
            ,"bullet"
            ,&b2BodyDef::bullet
            ,"active"
            ,&b2BodyDef::active
            ,"userData"
            ,&b2BodyDef::userData
            ,"gravityScale"
            ,&b2BodyDef::gravityScale
        // methods
        // constructors
            ,sol::call_constructor
            ,sol::constructors<
                b2BodyDef()
            >()
        );
        
        state.new_usertype<b2Body>("b2Body","CreateFixture"
                ,sol::overload(
                    sol::resolve<b2Fixture *(const b2FixtureDef *)>(&b2Body::CreateFixture)
                    ,
                    sol::resolve<b2Fixture *(const b2Shape *, float32)>(&b2Body::CreateFixture)
                )
                ,"DestroyFixture"
                ,&b2Body::DestroyFixture
                ,"SetTransform"
                ,&b2Body::SetTransform
                ,"GetTransform"
                ,&b2Body::GetTransform
                ,"GetPosition"
                ,&b2Body::GetPosition
                ,"GetAngle"
                ,&b2Body::GetAngle
                ,"GetWorldCenter"
                ,&b2Body::GetWorldCenter
                ,"GetLocalCenter"
                ,&b2Body::GetLocalCenter
                ,"SetLinearVelocity"
                ,&b2Body::SetLinearVelocity
                ,"GetLinearVelocity"
                ,&b2Body::GetLinearVelocity
                ,"SetAngularVelocity"
                ,&b2Body::SetAngularVelocity
                ,"GetAngularVelocity"
                ,&b2Body::GetAngularVelocity
                ,"ApplyForce"
                ,&b2Body::ApplyForce
                ,"ApplyForceToCenter"
                ,&b2Body::ApplyForceToCenter
                ,"ApplyTorque"
                ,&b2Body::ApplyTorque
                ,"ApplyLinearImpulse"
                ,&b2Body::ApplyLinearImpulse
                ,"ApplyAngularImpulse"
                ,&b2Body::ApplyAngularImpulse
                ,"GetMass"
                ,&b2Body::GetMass
                ,"GetInertia"
                ,&b2Body::GetInertia
                ,"GetMassData"
                ,&b2Body::GetMassData
                ,"SetMassData"
                ,&b2Body::SetMassData
                ,"ResetMassData"
                ,&b2Body::ResetMassData
                ,"GetWorldPoint"
                ,&b2Body::GetWorldPoint
                ,"GetWorldVector"
                ,&b2Body::GetWorldVector
                ,"GetLocalPoint"
                ,&b2Body::GetLocalPoint
                ,"GetLocalVector"
                ,&b2Body::GetLocalVector
                ,"GetLinearVelocityFromWorldPoint"
                ,&b2Body::GetLinearVelocityFromWorldPoint
                ,"GetLinearVelocityFromLocalPoint"
                ,&b2Body::GetLinearVelocityFromLocalPoint
                ,"GetLinearDamping"
                ,&b2Body::GetLinearDamping
                ,"SetLinearDamping"
                ,&b2Body::SetLinearDamping
                ,"GetAngularDamping"
                ,&b2Body::GetAngularDamping
                ,"SetAngularDamping"
                ,&b2Body::SetAngularDamping
                ,"GetGravityScale"
                ,&b2Body::GetGravityScale
                ,"SetGravityScale"
                ,&b2Body::SetGravityScale
                ,"SetType"
                ,&b2Body::SetType
                ,"GetType"
                ,&b2Body::GetType
                ,"SetBullet"
                ,&b2Body::SetBullet
                ,"IsBullet"
                ,&b2Body::IsBullet
                ,"SetSleepingAllowed"
                ,&b2Body::SetSleepingAllowed
                ,"IsSleepingAllowed"
                ,&b2Body::IsSleepingAllowed
                ,"SetAwake"
                ,&b2Body::SetAwake
                ,"IsAwake"
                ,&b2Body::IsAwake
                ,"SetActive"
                ,&b2Body::SetActive
                ,"IsActive"
                ,&b2Body::IsActive
                ,"SetFixedRotation"
                ,&b2Body::SetFixedRotation
                ,"IsFixedRotation"
                ,&b2Body::IsFixedRotation
                ,"GetFixtureList"
                ,sol::overload(
                    sol::resolve<b2Fixture *()>(&b2Body::GetFixtureList)
                    ,
                    sol::resolve<const b2Fixture *()const>(&b2Body::GetFixtureList)
                )
                ,"GetJointList"
                ,sol::overload(
                    sol::resolve<b2JointEdge *()>(&b2Body::GetJointList)
                    ,
                    sol::resolve<const b2JointEdge *()const>(&b2Body::GetJointList)
                )
                ,"GetContactList"
                ,sol::overload(
                    sol::resolve<b2ContactEdge *()>(&b2Body::GetContactList)
                    ,
                    sol::resolve<const b2ContactEdge *()const>(&b2Body::GetContactList)
                )
                ,"GetNext"
                ,sol::overload(
                    sol::resolve<b2Body *()>(&b2Body::GetNext)
                    ,
                    sol::resolve<const b2Body *()const>(&b2Body::GetNext)
                )
                ,"GetUserData"
                ,&b2Body::GetUserData
                ,"SetUserData"
                ,&b2Body::SetUserData
                ,"GetWorld"
                ,sol::overload(
                    sol::resolve<b2World *()>(&b2Body::GetWorld)
                    ,
                    sol::resolve<const b2World *()const>(&b2Body::GetWorld)
                )
                ,"Dump"
                ,&b2Body::Dump
        // constructors
        );

    }

    void LuaManager::BindECSLua(sol::state& state)
    {
        sol::usertype<entt::registry> reg_type = state.new_usertype< entt::registry >( "Registry" );
        reg_type.set_function( "Create", static_cast< entt::entity( entt::registry::* )() >( &entt::registry::create ) );
        reg_type.set_function( "Destroy", static_cast< void( entt::registry::* )( entt::entity ) >( &entt::registry::destroy ) );
        state.set_function( "GetEntityByName", &GetEntityByName );
        
        sol::usertype< NameComponent > nameComponent_type = state.new_usertype< NameComponent >( "NameComponent" );
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS( state, NameComponent, static_cast< NameComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace< NameComponent > ) );
        
        sol::usertype< ScriptComponent > script_type = state.new_usertype< ScriptComponent >( "ScriptComponent", sol::constructors<sol::types<String, Scene*>>() );
        REGISTER_COMPONENT_WITH_ECS( state, ScriptComponent, static_cast< ScriptComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace< ScriptComponent > ) );
        
        using namespace Maths;
        REGISTER_COMPONENT_WITH_ECS( state, Transform, static_cast<Transform&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Transform > ) );
        
        using namespace Graphics;
        REGISTER_COMPONENT_WITH_ECS( state, Sprite, static_cast<Sprite&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Sprite > ) );
        REGISTER_COMPONENT_WITH_ECS( state, Light, static_cast<Light&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Light > ) );
        
        sol::usertype< MeshComponent > meshComponent_type = state.new_usertype< MeshComponent >( "MeshComponent" );
        meshComponent_type["SetMesh"] = &MeshComponent::SetMesh;
        
        REGISTER_COMPONENT_WITH_ECS( state, MeshComponent, static_cast<MeshComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<MeshComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, CameraComponent, static_cast<CameraComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<CameraComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, Physics3DComponent, static_cast<Physics3DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics3DComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, Physics3DComponent, static_cast<Physics3DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics3DComponent > ) );
        
        sol::usertype< Physics2DComponent > physics2DComponent_type = state.new_usertype< Physics2DComponent >( "Physics2DComponent" );
        physics2DComponent_type.set_function( "GetPhysicsObject", &Physics2DComponent::GetPhysicsObjectRaw );

        REGISTER_COMPONENT_WITH_ECS( state, Physics2DComponent, static_cast<Physics2DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics2DComponent > ) );
        
        REGISTER_COMPONENT_WITH_ECS( state, SoundComponent, static_cast<SoundComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<SoundComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, MaterialComponent, static_cast<MaterialComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<MaterialComponent > ) );
        
        state.set_function("LoadMesh", &ModelLoader::LoadModel);
        
        sol::usertype<Graphics::Mesh> mesh_type = state.new_usertype< Graphics::Mesh >( "Mesh" );

        std::initializer_list< std::pair< sol::string_view, Lumos::Graphics::PrimitiveType > > primitives =
        {
            { "Cube", Lumos::Graphics::PrimitiveType::Cube  },
            { "Plane", Lumos::Graphics::PrimitiveType::Plane  },
            { "Quad", Lumos::Graphics::PrimitiveType::Quad  },
            { "Pyramid", Lumos::Graphics::PrimitiveType::Pyramid  },
            { "Sphere", Lumos::Graphics::PrimitiveType::Sphere  },
            { "Capsule", Lumos::Graphics::PrimitiveType::Capsule  },
            { "Cylinder", Lumos::Graphics::PrimitiveType::Cylinder  },

        };
        state.new_enum< Lumos::Graphics::PrimitiveType, false >( "PrimitiveType", primitives );
        state.set_function("LoadMesh", &CreatePrimative);
    }

    void LuaManager::BindSceneLua(sol::state& state)
    {
        sol::usertype<Scene> scene_type = state.new_usertype< Scene >( "Scene" );
        scene_type.set_function( "GetRegistry", &Scene::GetRegistry );
        scene_type.set_function("GetCamera", &Scene::GetCamera);
        
        sol::usertype< ThirdPersonCamera > camera_type = state.new_usertype< ThirdPersonCamera >( "Camera", sol::constructors<sol::types<float, float, float, float>>() );
        camera_type["position"]                 = &ThirdPersonCamera::GetPosition;
        camera_type["yaw"]                      = &ThirdPersonCamera::GetYaw;
        camera_type["fov"]                      = &ThirdPersonCamera::GetFOV;
        camera_type["aspectRatio"]              = &ThirdPersonCamera::GetAspectRatio;
        camera_type["nearPlane"]                = &ThirdPersonCamera::GetNear;
        camera_type["farPlane"]                 = &ThirdPersonCamera::GetFar;
        camera_type["GetForwardDir"]            = &ThirdPersonCamera::GetForwardDirection;
        camera_type["GetUpDir"]                 = &ThirdPersonCamera::GetUpDirection;
        camera_type["GetRightDir"]              = &ThirdPersonCamera::GetRightDirection;
                
        sol::usertype< Graphics::Texture2D > texture2D_type = state.new_usertype< Graphics::Texture2D >( "Texture2D" );
        texture2D_type.set_function("CreateFromFile", &Graphics::Texture2D::CreateFromFile);
    }

    void LuaManager::BindMathsLua(sol::state& state)
    {
        state.new_usertype<Maths::Vector2>("Vector2",
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

        state.new_usertype<Maths::Vector3>("Vector3",
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

        state.new_usertype<Maths::Vector4>("Vector4",
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

        state.new_usertype<Maths::Quaternion>("Quaternion",
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

        state.new_usertype<Maths::Matrix3>("Matrix3",
            sol::constructors<Maths::Matrix3(float, float, float, float, float,float,float,float,float), Maths::Matrix3()>(),
            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector3(Maths::Matrix3::*)(const Maths::Vector3&) const>(&Maths::Matrix3::operator*),
                static_cast<Maths::Matrix3(Maths::Matrix3::*)(const Maths::Matrix3&) const>(&Maths::Matrix3::operator*)
            )
            );

        state.new_usertype<Maths::Matrix4>("Matrix4",
            sol::constructors<Maths::Matrix4(Maths::Matrix3), Maths::Matrix4()>(),
		
  /*          "Scale", &Maths::Matrix4::Scale,
            "Rotation", &Maths::Matrix4::Rotation,
            "Translation", &Maths::Matrix4::Translation,*/

            sol::meta_function::multiplication, sol::overload(
                static_cast<Maths::Vector4(Maths::Matrix4::*)(const Maths::Vector4&) const>(&Maths::Matrix4::operator*),
                static_cast<Maths::Matrix4(Maths::Matrix4::*)(const Maths::Matrix4&) const>(&Maths::Matrix4::operator*)
            )
            );

        state.new_usertype<Maths::Transform>("Transform",
            sol::constructors<Maths::Transform(Maths::Matrix4), Maths::Transform(), Maths::Transform(Maths::Vector3)>(),

            "LocalScale", &Maths::Transform::GetLocalScale,
            "LocalOrientation", &Maths::Transform::GetLocalOrientation,
            "LocalPosition", &Maths::Transform::GetLocalPosition,
            "ApplyTransform", &Maths::Transform::ApplyTransform,
            "UpdateMatrices", &Maths::Transform::UpdateMatrices,
            "SetLocalTransform", &Maths::Transform::SetLocalTransform,
            "SetLocalPosition", &Maths::Transform::SetLocalPosition,
            "SetLocalScale", &Maths::Transform::SetLocalScale,
            "SetLocalOrientation", &Maths::Transform::SetLocalOrientation
            );
    }
}