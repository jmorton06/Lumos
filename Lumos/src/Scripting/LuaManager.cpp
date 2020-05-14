#include "lmpch.h"
#include "LuaManager.h"
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
#include "Graphics/Camera/Camera2D.h"

#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/API/Texture.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Utilities/RandomNumberGenerator.h"

#include "ImGuiLua.h"
#include "PhysicsLua.h"
#include "MathsLua.h"

#include <imgui/imgui.h>

#ifdef CUSTOM_SMART_PTR
namespace sol {
    template <typename T>
    struct unique_usertype_traits<Lumos::Ref<T>> {
            typedef T type;
            typedef Lumos::Ref<T> actual_type;
            static const bool value = true;

            static bool is_null(const actual_type& ptr) {
                    return ptr == nullptr;
            }

            static type* get (const actual_type& ptr) {
                    return ptr.get();
            }
    };
    
    template <typename T>
    struct unique_usertype_traits<Lumos::Scope<T>> {
            typedef T type;
            typedef Lumos::Scope<T> actual_type;
            static const bool value = true;

            static bool is_null(const actual_type& ptr) {
                    return ptr == nullptr;
            }

            static type* get (const actual_type& ptr) {
                    return ptr.get();
            }
    };
    
    template <typename T>
    struct unique_usertype_traits<Lumos::WeakRef<T>> {
            typedef T type;
            typedef Lumos::WeakRef<T> actual_type;
            static const bool value = true;

            static bool is_null(const actual_type& ptr) {
                    return ptr == nullptr;
            }

            static type* get (const actual_type& ptr) {
                    return ptr.get();
            }
    };
}

#endif

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
        BindPhysicsLua(m_State);
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
    
    Ref<Graphics::Texture2D> LoadTexture(const String& name, const String& path)
    {
        return Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
    }
    
    Ref<Graphics::Texture2D> LoadTextureWithParams(const String& name, const String& path, Lumos::Graphics::TextureFilter filter, Lumos::Graphics::TextureWrap wrapMode)
    {
        return Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path, Graphics::TextureParameters(filter, wrapMode)));
    }

    void LuaManager::BindECSLua(sol::state& state)
    {
        sol::usertype<entt::registry> reg_type = state.new_usertype< entt::registry >( "Registry" );
        reg_type.set_function( "Create", static_cast< entt::entity( entt::registry::* )() >( &entt::registry::create ) );
        reg_type.set_function( "Destroy", static_cast< void( entt::registry::* )( entt::entity ) >( &entt::registry::destroy ) );
        reg_type.set_function( "Valid", &entt::registry::valid );

        state.set_function( "GetEntityByName", &GetEntityByName );
        
        sol::usertype< NameComponent > nameComponent_type = state.new_usertype< NameComponent >( "NameComponent" );
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS( state, NameComponent, static_cast< NameComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace< NameComponent > ) );
        
        sol::usertype< ScriptComponent > script_type = state.new_usertype< ScriptComponent >( "ScriptComponent", sol::constructors<sol::types<String, Scene*>>() );
        REGISTER_COMPONENT_WITH_ECS( state, ScriptComponent, static_cast< ScriptComponent&( entt::registry::* )( const entt::entity, String&&, Scene*&& )> ( &entt::registry::emplace< ScriptComponent, String, Scene* > ) );
        
        using namespace Maths;
        REGISTER_COMPONENT_WITH_ECS( state, Transform, static_cast<Transform&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Transform > ) );
        
        using namespace Graphics;
        sol::usertype< Sprite > sprite_type = state.new_usertype< Sprite >( "Sprite", sol::constructors<sol::types<Maths::Vector2, Maths::Vector2, Maths::Vector4>, Sprite(const Ref<Graphics::Texture2D>&, const Maths::Vector2&, const Maths::Vector2&, const Maths::Vector4&)>() );
        sprite_type.set_function("SetTexture", &Sprite::SetTexture);
    
        REGISTER_COMPONENT_WITH_ECS( state, Sprite, static_cast<Sprite&( entt::registry::* )( const entt::entity, const Vector2&,  const Vector2&,  const Vector4& )> ( &entt::registry::emplace<Sprite, const Vector2&,  const Vector2&,   const Vector4& > ) );
    
        REGISTER_COMPONENT_WITH_ECS( state, Light, static_cast<Light&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Light > ) );
        
        sol::usertype< MeshComponent > meshComponent_type = state.new_usertype< MeshComponent >( "MeshComponent" );
        meshComponent_type["SetMesh"] = &MeshComponent::SetMesh;
        
        REGISTER_COMPONENT_WITH_ECS( state, MeshComponent, static_cast<MeshComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<MeshComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, CameraComponent, static_cast<CameraComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<CameraComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, Physics3DComponent, static_cast<Physics3DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics3DComponent > ) );
        REGISTER_COMPONENT_WITH_ECS( state, Physics3DComponent, static_cast<Physics3DComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::emplace<Physics3DComponent > ) );
        
        sol::usertype< Physics2DComponent > physics2DComponent_type = state.new_usertype< Physics2DComponent >( "Physics2DComponent", sol::constructors<sol::types<const Ref<PhysicsObject2D>&>>());
        physics2DComponent_type.set_function( "GetPhysicsObject", &Physics2DComponent::GetPhysicsObject );

        REGISTER_COMPONENT_WITH_ECS( state, Physics2DComponent, static_cast<Physics2DComponent&( entt::registry::* )( const entt::entity , Ref<PhysicsObject2D>&)> ( &entt::registry::emplace<Physics2DComponent, Ref<PhysicsObject2D>& > ) );
        
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
    
        enum class TextureWrap
        {
            NONE,
            REPEAT,
            CLAMP,
            MIRRORED_REPEAT,
            CLAMP_TO_EDGE,
            CLAMP_TO_BORDER
        };

        enum class TextureFilter
        {
            NONE,
            LINEAR,
            NEAREST
        };
    
        std::initializer_list< std::pair< sol::string_view, Lumos::Graphics::TextureFilter > > textureFilter =
        {
              { "None", Lumos::Graphics::TextureFilter::NONE  },
              { "Linear", Lumos::Graphics::TextureFilter::LINEAR  },
              { "Nearest", Lumos::Graphics::TextureFilter::NEAREST  }
        };
    
        std::initializer_list< std::pair< sol::string_view, Lumos::Graphics::TextureWrap > > textureWrap =
        {
            { "None", Lumos::Graphics::TextureWrap::NONE  },
            { "Repeat", Lumos::Graphics::TextureWrap::REPEAT  },
            { "Clamp", Lumos::Graphics::TextureWrap::CLAMP  },
            { "MorroredRepeat", Lumos::Graphics::TextureWrap::MIRRORED_REPEAT  },
            { "ClampToEdge", Lumos::Graphics::TextureWrap::CLAMP_TO_EDGE  },
            { "ClampToBorder", Lumos::Graphics::TextureWrap::CLAMP_TO_BORDER  }
        };
    
        state.new_enum< Lumos::Graphics::PrimitiveType, false >( "PrimitiveType", primitives );
        state.set_function("LoadMesh", &CreatePrimative);
    
        state.new_enum< Lumos::Graphics::TextureWrap, false >( "TextureWrap", textureWrap );
        state.new_enum< Lumos::Graphics::TextureFilter, false >( "TextureFilter", textureFilter );

        state.set_function("LoadTexture", &LoadTexture);
        state.set_function("LoadTextureWithParams", &LoadTextureWithParams);
    }
    
    static float LuaRand(float a, float b)
    {
        return RandomNumberGenerator32::Rand(a,b);
    }

    void LuaManager::BindSceneLua(sol::state& state)
    {
        sol::usertype<Scene> scene_type = state.new_usertype< Scene >( "Scene" );
        scene_type.set_function( "GetRegistry", &Scene::GetRegistry );
        scene_type.set_function("GetCamera", &Scene::GetCamera);
        
        sol::usertype< Camera > camera_type = state.new_usertype< Camera >( "Camera", sol::constructors<sol::types<float, float, float, float>>() );
        camera_type["position"]                 = &Camera::GetPosition;
        camera_type["yaw"]                      = &Camera::GetYaw;
        camera_type["fov"]                      = &Camera::GetFOV;
        camera_type["aspectRatio"]              = &Camera::GetAspectRatio;
        camera_type["nearPlane"]                = &Camera::GetNear;
        camera_type["farPlane"]                 = &Camera::GetFar;
        camera_type["GetForwardDir"]            = &Camera::GetForwardDirection;
        camera_type["GetUpDir"]                 = &Camera::GetUpDirection;
        camera_type["GetRightDir"]              = &Camera::GetRightDirection;
        camera_type["SetPosition"]              = &Camera::SetPosition;

        sol::usertype< Graphics::Texture2D > texture2D_type = state.new_usertype< Graphics::Texture2D >( "Texture2D" );
        texture2D_type.set_function("CreateFromFile", &Graphics::Texture2D::CreateFromFile);
    
        state.set_function("Rand", &LuaRand);
    }
}
