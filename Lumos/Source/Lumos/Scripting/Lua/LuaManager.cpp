#include "Precompiled.h"
#include "LuaManager.h"
#include "LuaBindingRegistry.h"
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
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Scene/Component/ModelComponent.h"
#include "Scene/Component/VoxelWorldComponent.h"
#include "Graphics/Material.h"
#include "Graphics/Environment.h"
#include "Audio/AudioManager.h"
#include "Maths/Random.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/EntityFactory.h"
#include "Scene/Component/SoundComponent.h"
#include "Audio/Sound.h"
#include "Audio/SoundNode.h"
#include "Scene/Component/TextureMatrixComponent.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Component/AIComponent.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"

#include "ImGuiLua.h"
#include "PhysicsLua.h"
#include "MathsLua.h"
#include "LuaUtilities.h"
#include "Core/OS/FileDialogs.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/RHI/IMGUIRenderer.h"
#include "Graphics/Renderers/SceneRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "ImGui/ImGuiManager.h"
#include "Utilities/LoadImage.h"
#include "Utilities/ImageExport.h"
#include "Graphics/ShaderCompiler.h"
#include "Graphics/ShaderPreview.h"

#include <imgui/imgui.h>
#include "Sol2Config.h"
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
        LuaRegistry::AddMethod("Entity", "Add" #Comp,         #Comp, "() -> " #Comp);                          \
        LuaRegistry::AddMethod("Entity", "Remove" #Comp,      "",    "() -> void");                            \
        LuaRegistry::AddMethod("Entity", "Get" #Comp,         #Comp, "() -> " #Comp);                          \
        LuaRegistry::AddMethod("Entity", "GetOrAdd" #Comp,    #Comp, "() -> " #Comp);                          \
        LuaRegistry::AddMethod("Entity", "TryGet" #Comp,      #Comp, "() -> " #Comp);                          \
        LuaRegistry::AddMethod("Entity", "AddOrReplace" #Comp,#Comp, "() -> " #Comp);                          \
        LuaRegistry::AddMethod("Entity", "Has" #Comp,         "bool","() -> bool");                            \
    }

    TDArray<std::string> LuaManager::s_Identifiers;

    // Hand-typed catalogue of every binding the editor's autocomplete uses.
    // Component Add/Get/Has/etc are wired via REGISTER_COMPONENT_WITH_ECS; this
    // covers everything that isn't a component.
    static void PopulateRegistryBuiltins()
    {
        using namespace LuaRegistry;

        // ---- Math types ----
        AddConstructor("Vec2", "(x: float, y: float) -> Vec2");
        AddField("Vec2", "x", "float");
        AddField("Vec2", "y", "float");
        AddMethod("Vec2", "Length",    "float", "() -> float");
        AddMethod("Vec2", "Distance",  "float", "(b: Vec2) -> float");
        AddMethod("Vec2", "Distance2", "float", "(b: Vec2) -> float");

        AddConstructor("Vec3", "(x?: float, y?: float, z?: float) -> Vec3");
        AddField("Vec3", "x", "float");
        AddField("Vec3", "y", "float");
        AddField("Vec3", "z", "float");
        AddMethod("Vec3", "Normalise",   "Vec3",  "() -> Vec3");
        AddMethod("Vec3", "Length",      "float", "() -> float");
        AddMethod("Vec3", "Distance",    "float", "(b: Vec3) -> float");
        AddMethod("Vec3", "Distance2",   "float", "(b: Vec3) -> float");
        AddMethod("Vec3", "Dot",         "float", "(a: Vec3, b: Vec3) -> float");
        AddMethod("Vec3", "Cross",       "Vec3",  "(a: Vec3, b: Vec3) -> Vec3");
        AddMethod("Vec3", "Lerp",        "Vec3",  "(a: Vec3, b: Vec3, t: float) -> Vec3");
        AddMethod("Vec3", "MoveTowards", "Vec3",  "(cur: Vec3, tgt: Vec3, step: float) -> Vec3");

        AddConstructor("Vec4", "(x?: float, y?: float, z?: float, w?: float) -> Vec4");
        AddField("Vec4", "x", "float"); AddField("Vec4", "y", "float");
        AddField("Vec4", "z", "float"); AddField("Vec4", "w", "float");
        AddMethod("Vec4", "Normalise",  "Vec4",  "() -> Vec4");
        AddMethod("Vec4", "Length",     "float", "() -> float");
        AddMethod("Vec4", "Distance",   "float", "(b: Vec4) -> float");
        AddMethod("Vec4", "Distance2",  "float", "(b: Vec4) -> float");

        AddConstructor("Quat", "(x: float, y: float, z: float, w: float) -> Quat");
        AddField("Quat", "x", "float"); AddField("Quat", "y", "float");
        AddField("Quat", "z", "float"); AddField("Quat", "w", "float");
        AddMethod("Quat", "Normalise", "Quat", "() -> Quat");
        AddMethod("Quat", "Transform", "Vec3", "(v: Vec3) -> Vec3");
        AddGlobal("QuatLookAt", "Quat", "(from: Vec3, to: Vec3, up?: Vec3) -> Quat");
        AddGlobal("QuatSlerp",  "Quat", "(a: Quat, b: Quat, t: float) -> Quat");

        AddConstructor("Matrix3");
        AddConstructor("Matrix4");

        // Transform component.
        AddConstructor("Transform", "(pos?: Vec3) -> Transform");
        AddField("Transform", "LocalScale",       "Vec3");
        AddField("Transform", "LocalOrientation", "Quat");
        AddField("Transform", "LocalPosition",    "Vec3");
        AddMethod("Transform", "SetLocalTransform",   "",     "(m: Matrix4) -> void");
        AddMethod("Transform", "SetLocalPosition",    "",     "(p: Vec3) -> void");
        AddMethod("Transform", "SetLocalScale",       "",     "(s: Vec3) -> void");
        AddMethod("Transform", "SetLocalOrientation", "",     "(q: Quat) -> void");
        AddMethod("Transform", "GetWorldPosition",    "Vec3", "() -> Vec3");
        AddMethod("Transform", "GetWorldOrientation", "Quat", "() -> Quat");
        AddMethod("Transform", "GetForwardDirection", "Vec3", "() -> Vec3");
        AddMethod("Transform", "GetRightDirection",   "Vec3", "() -> Vec3");

        // Easing / animation globals.
        for(const char* n : { "SineOut","SineIn","SineInOut",
                              "ExponentialOut","ExponentialIn","ExponentialInOut",
                              "ElasticIn","ElasticOut","ElasticInOut" })
            AddGlobal(n, "float", "(t: float) -> float");
        AddGlobal("AnimateToTarget", "float", "(cur: float, tgt: float, speed: float) -> float");
        AddGlobal("Rand",            "float", "(a: float, b: float) -> float");

        // ---- Input ----
        AddMethod("Input", "GetKeyPressed",            "bool",  "(key: Key) -> bool");
        AddMethod("Input", "GetKeyHeld",               "bool",  "(key: Key) -> bool");
        AddMethod("Input", "GetMouseClicked",          "bool",  "(b: MouseButton) -> bool");
        AddMethod("Input", "GetMouseHeld",             "bool",  "(b: MouseButton) -> bool");
        AddMethod("Input", "GetMousePosition",         "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetScrollOffset",          "float", "() -> float");
        AddMethod("Input", "SetMouseCaptured",         "",      "(captured: bool) -> void");
        AddMethod("Input", "GetControllerAxis",        "float", "(id: int, axis: int) -> float");
        AddMethod("Input", "GetControllerName",        "string","(id: int) -> string");
        AddMethod("Input", "GetControllerHat",         "int",   "(id: int, hat: int) -> int");
        AddMethod("Input", "IsControllerButtonPressed","bool",  "(id: int, button: int) -> bool");
        AddMethod("Input", "GetPanActive",             "bool",  "() -> bool");
        AddMethod("Input", "GetPanTranslation",        "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetPanVelocity",           "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetPanTouchCount",         "int",   "() -> int");
        AddMethod("Input", "GetPinchActive",           "bool",  "() -> bool");
        AddMethod("Input", "GetPinchScale",            "float", "() -> float");
        AddMethod("Input", "GetPinchVelocity",         "float", "() -> float");
        AddMethod("Input", "GetLongPressActive",       "bool",  "() -> bool");
        AddMethod("Input", "GetLongPressPosition",     "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetScreenSize",            "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetDPIScale",              "float", "() -> float");
        AddConstructor("Input");

        // Logging.
        for(const char* n : { "Trace","Info","Warn","Error","FATAL" })
            AddMethod("Log", n, "", "(msg: string) -> void");
        AddConstructor("Log");

        // ---- Physics ----
        AddConstructor("RigidBodyParameters");
        AddField("RigidBodyParameters", "mass",       "float");
        AddField("RigidBodyParameters", "shape",      "Shape");
        AddField("RigidBodyParameters", "position",   "Vec3");
        AddField("RigidBodyParameters", "scale",      "Vec3");
        AddField("RigidBodyParameters", "isStatic",   "bool");
        AddField("RigidBodyParameters", "friction",   "float");
        AddField("RigidBodyParameters", "damping",    "float");
        AddField("RigidBodyParameters", "elasticity", "float");

        AddConstructor("RigidBody3D");
        AddMethod("RigidBody3D", "SetForce",            "",     "(f: Vec3) -> void");
        AddMethod("RigidBody3D", "SetPosition",         "",     "(p: Vec3) -> void");
        AddMethod("RigidBody3D", "SetLinearVelocity",   "",     "(v: Vec3) -> void");
        AddMethod("RigidBody3D", "GetLinearVelocity",   "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "SetOrientation",      "",     "(q: Quat) -> void");
        AddMethod("RigidBody3D", "SetAngularVelocity",  "",     "(v: Vec3) -> void");
        AddMethod("RigidBody3D", "GetAngularVelocity",  "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "SetFriction",         "",     "(f: float) -> void");
        AddMethod("RigidBody3D", "GetFriction",         "float","() -> float");
        AddMethod("RigidBody3D", "GetPosition",         "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "GetIsStatic",         "bool", "() -> bool");
        AddMethod("RigidBody3D", "SetIsStatic",         "",     "(b: bool) -> void");
        AddMethod("RigidBody3D", "SetCollisionShape",   "",     "(s: CollisionShapeType) -> void");
        AddMethod("RigidBody3D", "GetLinearFactor",     "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "SetLinearFactor",     "",     "(v: Vec3) -> void");
        AddMethod("RigidBody3D", "GetAngularFactor",    "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "SetAngularFactor",    "",     "(v: Vec3) -> void");
        AddMethod("RigidBody3D", "GetMaterial",         "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("RigidBody3D", "SetMaterial",         "",     "(m: PhysicsMaterial) -> void");
        AddMethod("RigidBody3D", "GetElasticity",       "float","() -> float");
        AddMethod("RigidBody3D", "SetElasticity",       "",     "(e: float) -> void");
        AddMethod("RigidBody3D", "GetIsTrigger",        "bool", "() -> bool");
        AddMethod("RigidBody3D", "SetIsTrigger",        "",     "(b: bool) -> void");
        AddMethod("RigidBody3D", "GetForce",            "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "GetTorque",           "Vec3", "() -> Vec3");
        AddMethod("RigidBody3D", "SetTorque",           "",     "(t: Vec3) -> void");
        AddMethod("RigidBody3D", "GetOrientation",      "Quat", "() -> Quat");
        AddMethod("RigidBody3D", "GetInverseMass",      "float","() -> float");
        AddMethod("RigidBody3D", "GetMass",             "float","() -> float");
        AddMethod("RigidBody3D", "SetMass",             "",     "(m: float) -> void");
        AddMethod("RigidBody3D", "WakeUp",              "",     "() -> void");
        AddMethod("RigidBody3D", "IsAwake",             "bool", "() -> bool");
        AddMethod("RigidBody3D", "GetCollisionLayer",   "int",  "() -> int");
        AddMethod("RigidBody3D", "SetCollisionLayer",   "",     "(l: int) -> void");
        AddMethod("RigidBody3D", "GetCollisionMask",    "int",  "() -> int");
        AddMethod("RigidBody3D", "SetCollisionMask",    "",     "(m: int) -> void");
        AddMethod("RigidBody3D", "AddForce",            "",     "(f: Vec3) -> void");
        AddMethod("RigidBody3D", "AddTorque",           "",     "(t: Vec3) -> void");
        AddMethod("RigidBody3D", "ApplyImpulse",        "",     "(i: Vec3) -> void");
        AddMethod("RigidBody3D", "ApplyAngularImpulse", "",     "(i: Vec3) -> void");

        AddConstructor("RigidBody3DComponent");
        AddMethod("RigidBody3DComponent", "GetRigidBody", "RigidBody3D", "() -> RigidBody3D");

        AddConstructor("RigidBody2D");
        AddMethod("RigidBody2D", "SetForce",          "",     "(f: Vec2) -> void");
        AddMethod("RigidBody2D", "SetPosition",       "",     "(p: Vec2) -> void");
        AddMethod("RigidBody2D", "SetLinearVelocity", "",     "(v: Vec2) -> void");
        AddMethod("RigidBody2D", "GetLinearVelocity", "Vec2", "() -> Vec2");
        AddMethod("RigidBody2D", "SetOrientation",    "",     "(angle: float) -> void");
        AddMethod("RigidBody2D", "SetAngularVelocity","",     "(v: float) -> void");
        AddMethod("RigidBody2D", "SetFriction",       "",     "(f: float) -> void");
        AddMethod("RigidBody2D", "GetPosition",       "Vec2", "() -> Vec2");
        AddMethod("RigidBody2D", "GetAngle",          "float","() -> float");
        AddMethod("RigidBody2D", "GetFriction",       "float","() -> float");
        AddMethod("RigidBody2D", "GetIsStatic",       "bool", "() -> bool");
        AddMethod("RigidBody2D", "SetIsStatic",       "",     "(b: bool) -> void");
        AddMethod("RigidBody2D", "SetLinearDamping",  "",     "(d: float) -> void");
        AddConstructor("RigidBody2DComponent");
        AddMethod("RigidBody2DComponent", "GetRigidBody", "RigidBody2D", "() -> RigidBody2D");

        AddConstructor("RaycastHit");
        AddField("RaycastHit", "body",     "RigidBody3D");
        AddField("RaycastHit", "point",    "Vec3");
        AddField("RaycastHit", "normal",   "Vec3");
        AddField("RaycastHit", "distance", "float");
        AddMethod("RaycastHit", "Hit", "bool", "() -> bool");

        AddGlobal("Raycast",         "RaycastHit", "(o: Vec3, d: Vec3, maxDist: float) -> RaycastHit");
        AddGlobal("RaycastAll",      "table",      "(o: Vec3, d: Vec3, maxDist: float) -> RaycastHit[]");
        AddGlobal("TerrainHeightAt", "float",      "(wx: float, wz: float) -> float|nil");

        AddConstructor("PhysicsMaterial");
        AddField("PhysicsMaterial", "friction",         "float");
        AddField("PhysicsMaterial", "restitution",      "float");
        AddField("PhysicsMaterial", "rolling_friction", "float");
        AddMethod("PhysicsMaterial", "Default",  "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("PhysicsMaterial", "Bouncy",   "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("PhysicsMaterial", "Ice",      "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("PhysicsMaterial", "Rubber",   "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("PhysicsMaterial", "Metal",    "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("PhysicsMaterial", "Wood",     "PhysicsMaterial", "() -> PhysicsMaterial");
        AddMethod("PhysicsMaterial", "Concrete", "PhysicsMaterial", "() -> PhysicsMaterial");

        AddMethod("Physics", "SetGravity3D", "",     "(g: Vec3) -> void");
        AddMethod("Physics", "GetGravity3D", "Vec3", "() -> Vec3");
        AddConstructor("Physics");

        AddGlobal("SetB2DGravity", "", "(g: Vec2) -> void");
        AddGlobal("SetCallback",   "", "(fn, body) -> void");

        // ---- Scene / App ----
        AddMethod("Scene", "GetEntityManager", "EntityManager", "() -> EntityManager");
        AddMethod("Scene", "GetRegistry",      "enttRegistry",  "() -> registry");
        AddMethod("Scene", "CreateEntity",     "Entity",        "(name?: string) -> Entity");
        AddMethod("Scene", "GetSceneName",     "string",        "() -> string");
        AddMethod("EntityManager", "Create",   "Entity",        "(name?: string) -> Entity");
        AddMethod("EntityManager", "GetRegistry", "enttRegistry","() -> registry");

        AddMethod("Application", "GetWindowSize", "Vec2", "() -> Vec2");

        AddGlobal("GetCurrentScene",     "Scene",       "() -> Scene");
        AddGlobal("GetAppInstance",      "Application", "() -> Application");
        AddGlobal("SwitchSceneByIndex",  "",            "(i: int) -> void");
        AddGlobal("SwitchSceneByName",   "",            "(name: string) -> void");
        AddGlobal("SwitchScene",         "",            "() -> void");
        AddGlobal("ExitApp",             "",            "() -> void");
        AddGlobal("SetPhysicsDebugFlags","",            "(flags: int) -> void");
        AddGlobal("SetSkyColour",        "",            "(rgb: Vec3) -> void");
        AddGlobal("DebugLine",           "",            "(a: Vec3, b: Vec3, thickness: float, col: Vec4) -> void");
        AddGlobal("DebugPoint",          "",            "(p: Vec3, r: float, col: Vec4) -> void");

        AddGlobal("GetEntityByName",     "Entity",      "(scene: Scene, name: string) -> Entity");
        AddGlobal("GetAllEntities",      "table",       "() -> Entity[]");
        AddGlobal("EachEntity",          "",            "() -> iterator");

        // EntityFactory globals.
        AddGlobal("AddPyramidEntity",    "Entity", "(scene: Scene) -> Entity");
        AddGlobal("AddSphereEntity",     "Entity", "(scene: Scene) -> Entity");
        AddGlobal("AddLightCubeEntity",  "Entity", "(scene: Scene) -> Entity");
        AddGlobal("AddPlatform",         "Entity", "(scene: Scene) -> Entity");
        AddGlobal("AddTerrain",          "Entity", "(scene: Scene, pos: Vec3, gridSize?: int, scaleXZ?: float, heightScale?: float) -> Entity");
        AddGlobal("AddTerrainEx",        "Entity", "(scene: Scene, pos: Vec3, gridSize: int, scaleXZ: float, heightScale: float, mat?: table) -> Entity");
        AddGlobal("AddArrow",            "Entity", "(scene: Scene, pos: Vec3, velocity: Vec3, radius?: float, length?: float, mass?: float) -> Entity");
        AddGlobal("AddTarget",           "Entity", "(scene: Scene, pos: Vec3, radius?: float, colour?: Vec4) -> Entity");
        AddGlobal("AddDecorSphere",      "Entity", "(scene: Scene, name?: string, pos: Vec3, radius: float, colour: Vec4) -> Entity");
        AddGlobal("AddDecorCube",        "Entity", "(scene: Scene, name?: string, pos: Vec3, scale: Vec3, colour: Vec4) -> Entity");
        AddGlobal("AddPhysicsSphere",    "Entity", "(scene: Scene, name: string, pos: Vec3, radius: float, inverseMass: float, colour: Vec4) -> Entity");
        AddGlobal("SetEntityPulse",      "",       "(e: Entity, albedo: Vec4, emissive: float) -> void");

        // Entity core methods (Has/Add/Get/etc per-component come from the macro).
        AddMethod("Entity", "Valid",       "bool",   "() -> bool");
        AddMethod("Entity", "Destroy",     "",       "() -> void");
        AddMethod("Entity", "SetParent",   "",       "(e: Entity) -> void");
        AddMethod("Entity", "GetParent",   "Entity", "() -> Entity");
        AddMethod("Entity", "IsParent",    "bool",   "(e: Entity) -> bool");
        AddMethod("Entity", "GetChildren", "table",  "() -> Entity[]");
        AddMethod("Entity", "SetActive",   "",       "(b: bool) -> void");
        AddMethod("Entity", "Active",      "bool",   "() -> bool");
        AddConstructor("Entity");

        // NameComponent.
        AddField("NameComponent", "name", "string");

        // LuaScriptComponent.
        AddMethod("LuaScriptComponent", "GetCurrentEntity", "Entity", "() -> Entity");
        AddMethod("LuaScriptComponent", "SetThisComponent", "",       "() -> void");

        // ---- Audio ----
        AddMethod("Sound", "GetLength",   "float", "() -> float");
        AddMethod("Sound", "GetFilePath", "string","() -> string");
        AddMethod("Sound", "Create",      "Sound", "(path: string, ext: string) -> Sound");
        AddConstructor("Sound");
        AddMethod("SoundNode", "Play",       "",        "() -> void");
        AddMethod("SoundNode", "Pause",      "",        "() -> void");
        AddMethod("SoundNode", "Resume",     "",        "() -> void");
        AddMethod("SoundNode", "Stop",       "",        "() -> void");
        AddMethod("SoundNode", "SetVolume",  "",        "(v: float) -> void");
        AddMethod("SoundNode", "GetVolume",  "float",   "() -> float");
        AddMethod("SoundNode", "SetLooping", "",        "(b: bool) -> void");
        AddMethod("SoundNode", "GetLooping", "bool",    "() -> bool");
        AddMethod("SoundNode", "SetPitch",   "",        "(p: float) -> void");
        AddMethod("SoundNode", "GetPitch",   "float",   "() -> float");
        AddMethod("SoundNode", "SetSound",   "",        "(s: Sound) -> void");
        AddMethod("SoundNode", "GetSound",   "Sound",   "() -> Sound");
        AddMethod("SoundNode", "Create",     "SoundNode","(s?: Sound) -> SoundNode");
        AddConstructor("SoundNode");
        AddMethod("SoundComponent", "GetSoundNode", "SoundNode", "() -> SoundNode");

        // ---- Graphics ----
        AddConstructor("Texture2D");
        AddMethod("Texture2D", "GetWidth",  "int", "() -> int");
        AddMethod("Texture2D", "GetHeight", "int", "() -> int");
        AddMethod("Texture2D", "CreateFromFile", "Texture2D", "(name: string, path: string) -> Texture2D");
        AddGlobal("LoadTexture",            "Texture2D", "(name: string, path: string) -> Texture2D");
        AddGlobal("LoadTextureWithParams",  "Texture2D", "(name: string, path: string, filter: TextureFilter, wrap: TextureWrap) -> Texture2D");
        AddGlobal("LoadTexture2D",          "Texture2D", "(path: string) -> Texture2D");
        AddGlobal("SaveTextureToFile",      "",          "(tex: Texture2D, path: string) -> void");
        AddGlobal("LoadMesh",               "Mesh",      "(primitive: PrimitiveType) -> Mesh");
        AddGlobal("SaveScreenshot",         "",          "(path: string) -> void");
        AddGlobal("GetAssetPath",           "string",    "() -> string");
        AddGlobal("ReadFile",               "string",    "(path: string) -> string");
        AddGlobal("WriteFile",              "bool",      "(path: string, content: string) -> bool");

        AddField("Light", "Intensity", "float");
        AddField("Light", "Radius",    "float");
        AddField("Light", "Colour",    "Vec4");
        AddField("Light", "Direction", "Vec3");
        AddField("Light", "Position",  "Vec3");
        AddField("Light", "Type",      "int");
        AddField("Light", "Angle",     "float");

        AddConstructor("Camera", "(fov: float, aspect: float, near?: float, far?: float) -> Camera");
        AddMethod("Camera", "SetIsOrthographic", "",     "(b: bool) -> void");
        AddMethod("Camera", "SetNearPlane",      "",     "(n: float) -> void");
        AddMethod("Camera", "SetFarPlane",       "",     "(f: float) -> void");
        AddMethod("Camera", "SetFOV",            "",     "(f: float) -> void");
        AddMethod("Camera", "SetAspectRatio",    "",     "(r: float) -> void");
        AddMethod("Camera", "SetScale",          "",     "(s: float) -> void");
        AddMethod("Camera", "IsOrthographic",    "bool", "() -> bool");

        AddMethod("Model", "GetMeshCount", "int",  "() -> int");
        AddMethod("Model", "GetTotalStats","table","() -> table");
        AddMethod("Model", "GetBounds",    "table","() -> table");
        AddMethod("Model", "add_mesh",     "",     "(m: Mesh) -> void");
        AddMethod("Model", "load_model",   "",     "(path: string) -> void");
        AddConstructor("Model");
        AddMethod("ModelComponent", "GetModel",       "Model", "() -> Model");
        AddMethod("ModelComponent", "LoadPrimitive",  "",      "(p: PrimitiveType) -> void");

        AddMethod("Mesh", "GetMaterial",   "Material",  "() -> Material");
        AddMethod("Mesh", "SetMaterial",   "",          "(m: Material) -> void");
        AddMethod("Mesh", "GetBoundingBox","BoundingBox","() -> BoundingBox");
        AddMethod("Mesh", "GetStats",      "MeshStats", "() -> MeshStats");
        AddMethod("Mesh", "GetName",       "string",    "() -> string");
        AddMethod("Mesh", "SetName",       "",          "(s: string) -> void");
        AddConstructor("Mesh");
        AddField("MeshStats", "TriangleCount", "int");
        AddField("MeshStats", "VertexCount",   "int");
        AddField("MeshStats", "IndexCount",    "int");
        AddField("BoundingBox", "min", "Vec3");
        AddField("BoundingBox", "max", "Vec3");
        AddMethod("BoundingBox", "GetExtents", "Vec3", "() -> Vec3");

        AddMethod("Material", "set_albedo_texture",    "", "(t: Texture2D) -> void");
        AddMethod("Material", "set_normal_texture",    "", "(t: Texture2D) -> void");
        AddMethod("Material", "set_roughness_texture", "", "(t: Texture2D) -> void");
        AddMethod("Material", "set_metallic_texture",  "", "(t: Texture2D) -> void");
        AddMethod("Material", "set_ao_texture",        "", "(t: Texture2D) -> void");
        AddMethod("Material", "set_emissive_texture",  "", "(t: Texture2D) -> void");
        AddMethod("Material", "get_name",              "string", "() -> string");
        AddMethod("Material", "set_name",              "",       "(s: string) -> void");
        AddMethod("Material", "load_pbr_material",     "",       "(path: string) -> void");
        AddMethod("Material", "load_material",         "",       "(path: string) -> void");

        // Sprite / AnimatedSprite.
        AddConstructor("Sprite", "(min: Vec2, size: Vec2, col: Vec4, tex?: Texture2D) -> Sprite");
        AddMethod("Sprite", "SetTexture",            "", "(t: Texture2D) -> void");
        AddMethod("Sprite", "SetSpriteSheet",        "", "(t: Texture2D) -> void");
        AddMethod("Sprite", "SetSpriteSheetIndex",   "", "(i: int) -> void");
        AddField("Sprite", "SpriteSheetTileSizeX", "int");
        AddField("Sprite", "SpriteSheetTileSizeY", "int");
        AddConstructor("AnimatedSprite");
        AddMethod("AnimatedSprite", "SetTexture",          "", "(t: Texture2D) -> void");
        AddMethod("AnimatedSprite", "SetSpriteSheet",      "", "(t: Texture2D) -> void");
        AddMethod("AnimatedSprite", "SetSpriteSheetIndex", "", "(i: int) -> void");
        AddMethod("AnimatedSprite", "SetState",            "", "(i: int) -> void");

        // Particles.
        AddConstructor("ParticleEmitter", "(count?: int) -> ParticleEmitter");
        for(const char* setter : {
            "SetParticleCount","SetParticleLife","SetParticleSize","SetInitialVelocity",
            "SetInitialColour","SetSpread","SetVelocitySpread","SetGravity",
            "SetNextParticleTime","SetParticleRate","SetNumLaunchParticles","SetIsAnimated",
            "SetAnimatedTextureRows","SetSortParticles","SetBlendType","SetFadeIn",
            "SetFadeOut","SetLifeSpread","SetAlignedType","SetDepthWrite",
            "SetTextureFromFile" })
            AddMethod("ParticleEmitter", setter, "", "(v) -> void");
        AddMethod("ParticleEmitter", "Update", "", "(dt: float) -> void");

        // Audio listener field.
        AddField("Listener", "enabled", "bool");

        // ---- UI ----
        AddGlobal("InitialiseUI",   "", "() -> void");
        AddGlobal("ShutDownUI",     "", "() -> void");
        AddGlobal("UIBeginFrame",   "", "() -> void");
        AddGlobal("UIEndFrame",     "", "() -> void");
        AddGlobal("UIBeginPanel",   "UI_Interaction",
                  "(title: string, wkind?: SizeKind, w?: float, hkind?: SizeKind, h?: float, flags?: int) -> UI_Interaction");
        AddGlobal("UIEndPanel",     "", "() -> void");
        AddGlobal("UIBeginOverlay", "UI_Interaction",
                  "(title: string, wkind?: SizeKind, w?: float, hkind?: SizeKind, h?: float, flags?: int) -> UI_Interaction");
        AddGlobal("UIWindowAnchor", "", "(a: UIAnchor, mx?: float, my?: float) -> void");
        AddGlobal("UIWindowDock",   "", "(pos: UIDock, size?: float) -> void");
        AddGlobal("UIWindowCenter", "", "() -> void");
        AddGlobal("UIWindowFillScreen","","() -> void");
        AddGlobal("UIWindowSetSize","",  "(w: float, h: float) -> void");
        AddGlobal("UILabel",        "", "(name: string, text: string) -> void");
        AddGlobal("UIButton",       "UI_Interaction", "(text: string) -> UI_Interaction");
        AddGlobal("UIImage",        "", "(t: Texture2D) -> void");
        AddGlobal("UISeparator",    "", "(w?: float) -> void");
        AddGlobal("UISpacer",       "", "(h?: float) -> void");
        AddGlobal("UISlider",       "float", "(label: string, value: float, min?: float, max?: float) -> float");
        AddGlobal("UIToggle",       "bool",  "(label: string, value: bool) -> bool");
        AddGlobal("UIProgressBar",  "bool",  "(label: string, p: float, w?: float, h?: float) -> bool");
        AddGlobal("UIArrow",        "bool",  "(dir: int) -> bool");
        AddGlobal("UILayoutRoot",   "", "() -> void");
        AddGlobal("UIPushStyle",    "", "(v: StyleVar, val: float|Vec2|Vec3|Vec4) -> void");
        AddGlobal("UISetNextFlags", "", "(flags: WidgetFlags) -> void");
        AddGlobal("UIPopStyle",     "", "() -> void");
        AddGlobal("GetStringSize",  "Vec2", "(text: string) -> Vec2");

        AddField("UI_Interaction", "widget",   "UI_Widget");
        AddField("UI_Interaction", "hovering", "bool");
        AddField("UI_Interaction", "clicked",  "bool");
        AddField("UI_Interaction", "dragging", "bool");

        // FileDialog table.
        AddMethod("FileDialog", "OpenFile",   "string", "(filter?: string, defaultPath?: string) -> string");
        AddMethod("FileDialog", "SaveFile",   "string", "(filter?: string, defaultPath?: string, defaultName?: string) -> string");
        AddMethod("FileDialog", "PickFolder", "string", "(defaultPath?: string) -> string");
        AddConstructor("FileDialog");

        // ---- Enums ----
        // Keys (subset; covers identifiers people will type).
        for(const char* k : {
            "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
            "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
            "Up","Down","Left","Right","Home","End","PageUp","PageDown",
            "Space","Enter","Tab","Backspace","Escape","Delete","Insert",
            "LeftShift","RightShift","LeftControl","RightControl","LeftAlt","RightAlt","LeftSuper","RightSuper",
            "Keypad0","Keypad1","Keypad2","Keypad3","Keypad4","Keypad5","Keypad6","Keypad7","Keypad8","Keypad9" })
            AddEnum("Key", k);
        for(const char* m : { "Left","Right","Middle" })
            AddEnum("MouseButton", m);
        for(const char* s : { "Square","Circle","Custom" })
            AddEnum("Shape", s);
        for(const char* s : { "Cuboid","Sphere","Pyramid","Capsule","Hull","Terrain" })
            AddEnum("CollisionShapeType", s);
        for(const char* s : { "Additive","Alpha","Off" })
            AddEnum("ParticleBlendType", s);
        for(const char* s : { "Aligned2D","Aligned3D","None" })
            AddEnum("ParticleAlignedType", s);
        for(const char* s : { "Cube","Plane","Quad","Pyramid","Sphere","Capsule","Cylinder","Terrain" })
            AddEnum("PrimitiveType", s);
        for(const char* s : { "NONE","DEPTHTEST","WIREFRAME","FORWARDRENDER","DEFERREDRENDER",
                              "NOSHADOW","TWOSIDED","ALPHABLEND" })
            AddEnum("RenderFlags", s);
        for(const char* s : { "None","TopLeft","TopCenter","TopRight",
                              "MiddleLeft","MiddleCenter","MiddleRight",
                              "BottomLeft","BottomCenter","BottomRight" })
            AddEnum("UIAnchor", s);
        for(const char* s : { "Left","Right","Top","Bottom","Fill" })
            AddEnum("UIDock", s);
        for(const char* s : { "X","Y","Count" })
            AddEnum("UIAxis", s);
        for(const char* s : { "Pixels","TextContent","PercentOfParent","ChildSum","MaxChild","PercentOfViewport" })
            AddEnum("SizeKind", s);
        for(const char* s : { "Padding","Border","BorderColor","BackgroundColor","TextColor",
                              "HotBorderColor","HotBackgroundColor","HotTextColor",
                              "ActiveBorderColor","ActiveBackgroundColor","ActiveTextColor",
                              "FontSize","Count" })
            AddEnum("StyleVar", s);
        for(const char* s : { "Clickable","DrawText","DrawBorder","DrawBackground","Draggable",
                              "StackVertically","StackHorizontally","Floating_X","Floating_Y",
                              "CentreX","CentreY","CentreChildrenX","CentreChildrenY","DragParent" })
            AddEnum("WidgetFlags", s);
        for(const char* s : { "None","Center_X","Center_Y" })
            AddEnum("UITextAlignment", s);
        for(const char* s : { "None","Linear","Nearest" })
            AddEnum("TextureFilter", s);
        for(const char* s : { "None","Repeat","Clamp","MirroredRepeat","ClampToEdge","ClampToBorder" })
            AddEnum("TextureWrap", s);
        for(const char* s : { "CONSTRAINT","MANIFOLD","COLLISIONVOLUMES","COLLISIONNORMALS",
                              "AABB","LINEARVELOCITY","LINEARFORCE","BROADPHASE","BROADPHASE_PAIRS","BOUNDING_RADIUS" })
            AddEnum("PhysicsDebugFlags", s);

        // ---- Lua standard library (sol::lib::base, math, string, table, os) ----
        // math
        AddConstructor("math");
        for(const char* n : { "abs","acos","asin","atan","ceil","cos","deg","exp","floor","fmod",
                              "log","max","min","modf","pow","rad","sin","sqrt","tan","tointeger",
                              "type","ult" })
            AddMethod("math", n, "number", "(n: number) -> number");
        AddMethod("math", "random",     "number", "(m?: int, n?: int) -> number");
        AddMethod("math", "randomseed", "",       "(seed: int) -> void");
        AddField("math", "pi",          "number");
        AddField("math", "huge",        "number");
        AddField("math", "maxinteger",  "number");
        AddField("math", "mininteger",  "number");

        // string
        AddConstructor("string");
        AddMethod("string", "format",   "string", "(fmt: string, ...) -> string");
        AddMethod("string", "len",      "int",    "(s: string) -> int");
        AddMethod("string", "lower",    "string", "(s: string) -> string");
        AddMethod("string", "upper",    "string", "(s: string) -> string");
        AddMethod("string", "rep",      "string", "(s: string, n: int, sep?: string) -> string");
        AddMethod("string", "reverse",  "string", "(s: string) -> string");
        AddMethod("string", "sub",      "string", "(s: string, i: int, j?: int) -> string");
        AddMethod("string", "byte",     "int",    "(s: string, i?: int) -> int");
        AddMethod("string", "char",     "string", "(... ints) -> string");
        AddMethod("string", "find",     "int",    "(s: string, pat: string, init?: int, plain?: bool) -> int,int");
        AddMethod("string", "match",    "string", "(s: string, pat: string, init?: int) -> string");
        AddMethod("string", "gmatch",   "iterator", "(s: string, pat: string) -> iterator");
        AddMethod("string", "gsub",     "string", "(s: string, pat: string, repl, n?: int) -> string,int");
        AddMethod("string", "dump",     "string", "(fn) -> string");
        AddMethod("string", "pack",     "string", "(fmt: string, ...) -> string");
        AddMethod("string", "packsize", "int",    "(fmt: string) -> int");
        AddMethod("string", "unpack",   "table",  "(fmt: string, s: string, pos?: int) -> values");

        // table
        AddConstructor("table");
        AddMethod("table", "insert",  "", "(t: table, [pos], v) -> void");
        AddMethod("table", "remove",  "", "(t: table, pos?: int) -> any");
        AddMethod("table", "concat",  "string", "(t: table, sep?: string, i?: int, j?: int) -> string");
        AddMethod("table", "sort",    "",       "(t: table, lt?: fn) -> void");
        AddMethod("table", "unpack",  "values", "(t: table, i?: int, j?: int) -> values");
        AddMethod("table", "pack",    "table",  "(...) -> table");
        AddMethod("table", "move",    "table",  "(a, f, e, t, a2?) -> table");

        // os
        AddConstructor("os");
        AddMethod("os", "time",     "int",    "(t?: table) -> int");
        AddMethod("os", "clock",    "number", "() -> number");
        AddMethod("os", "date",     "string", "(fmt?: string, t?: int) -> string");
        AddMethod("os", "difftime", "number", "(t2: int, t1: int) -> number");
        AddMethod("os", "getenv",   "string", "(name: string) -> string");
        AddMethod("os", "remove",   "bool",   "(path: string) -> bool");
        AddMethod("os", "rename",   "bool",   "(old: string, new: string) -> bool");
        AddMethod("os", "tmpname",  "string", "() -> string");
        AddMethod("os", "exit",     "",       "(code?: int) -> void");

        // Lua global functions (sol::lib::base).
        AddGlobal("print",       "",       "(...) -> void");
        AddGlobal("tostring",    "string", "(v: any) -> string");
        AddGlobal("tonumber",    "number", "(v: any, base?: int) -> number");
        AddGlobal("type",        "string", "(v: any) -> string");
        AddGlobal("ipairs",      "iterator","(t: table) -> iterator");
        AddGlobal("pairs",       "iterator","(t: table) -> iterator");
        AddGlobal("next",        "k,v",    "(t: table, k?: any) -> k,v");
        AddGlobal("select",      "any",    "(n, ...) -> values");
        AddGlobal("pcall",       "bool",   "(fn, ...) -> bool, results...");
        AddGlobal("xpcall",      "bool",   "(fn, msgh, ...) -> bool, results...");
        AddGlobal("error",       "",       "(msg, level?: int) -> void");
        AddGlobal("assert",      "any",    "(v, msg?) -> v");
        AddGlobal("setmetatable","table",  "(t: table, mt: table) -> table");
        AddGlobal("getmetatable","table",  "(t: table) -> table");
        AddGlobal("rawget",      "any",    "(t: table, k) -> any");
        AddGlobal("rawset",      "table",  "(t: table, k, v) -> table");
        AddGlobal("rawequal",    "bool",   "(a, b) -> bool");
        AddGlobal("rawlen",      "int",    "(v) -> int");
        AddGlobal("require",     "any",    "(modname: string) -> any");
        AddGlobal("collectgarbage","int",  "(opt?: string, arg?: int) -> int");
        AddGlobal("unpack",      "values", "(t: table, i?: int, j?: int) -> values");
    }

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
    // Custom Lua package searcher backed by the engine VFS. Lets require() resolve modules
    // whether they live on the physical filesystem or inside a mounted asset pack.
    // Search roots tried in order: "//Assets/Scripts/<name>.lua", "//Assets/Scripts/lua/<name>.lua".
    // Submodule paths work as-is — require("game/Camera") -> "//Assets/Scripts/game/Camera.lua".
    static sol::object VFSScriptSearcher(sol::this_state ts, const std::string& moduleName)
    {
        sol::state_view state(ts);

        // Lua module names may use '.' as separator; normalise to '/'
        std::string rel = moduleName;
        for(auto& c : rel) if(c == '.') c = '/';

        static const char* kSearchRoots[] = {
            "//Assets/Scripts/",
            "//Assets/Scripts/lua/",
        };

        ArenaTemp scratch = ScratchBegin(0, 0);
        for(const char* root : kSearchRoots)
        {
            char pathBuf[512];
            snprintf(pathBuf, sizeof(pathBuf), "%s%s.lua", root, rel.c_str());
            String8 path = Str8C(pathBuf);
            if(!FileSystem::Get().FileExistsVFS(path))
                continue;

            String8 text = FileSystem::Get().ReadTextFileVFS(scratch.arena, path);
            std::string source((const char*)text.str, text.size);
            std::string chunkName = std::string("@") + pathBuf;

            sol::load_result loaded = state.load(source, chunkName);
            if(!loaded.valid())
            {
                sol::error err = loaded;
                std::string msg = std::string("\n\t[VFS loader] compile error in ") + pathBuf + ": " + err.what();
                ScratchEnd(scratch);
                return sol::make_object(state, msg);
            }
            sol::protected_function fn = loaded;
            ScratchEnd(scratch);
            return sol::make_object(state, fn);
        }
        ScratchEnd(scratch);
        std::string msg = std::string("\n\t[VFS loader] no module '") + moduleName + "' under //Assets/Scripts/{,lua/}";
        return sol::make_object(state, msg);
    }

    void LuaManager::OnInit()
    {
        LUMOS_PROFILE_FUNCTION();

        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);

        // Install VFS-aware require loader (works with packed assets too).
        // package.searchers exists from Lua 5.2; Lumos uses 5.3.
        {
            sol::table searchers = (*m_State)["package"]["searchers"];
            searchers[searchers.size() + 1] = &VFSScriptSearcher;
        }
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
            "GetAllEntities",
            "EachEntity",
            "AddPyramidEntity",
            "AddSphereEntity",
            "AddLightCubeEntity",
            "AddPlatform",
            "NameComponent",
            "GetNameComponent",
            "GetCurrentEntity",
            "SetThisComponent",
            "LuaScriptComponent",
            "GetLuaScriptComponent",
            "Transform",
            "GetTransform"
        };

        // Rebuilds typed binding catalogue used by the editor's autocomplete.
        LuaRegistry::Clear();
        PopulateRegistryBuiltins();

        BindInputLua(*m_State);
        BindMathsLua(*m_State);
        BindImGuiLua(*m_State);
        BindECSLua(*m_State);
        BindLogLua(*m_State);
        BindSceneLua(*m_State);
        BindPhysicsLua(*m_State);
        BindUILua(*m_State);
        BindAppLua(*m_State);
        BindVoxelLua(*m_State);

        //GenerateLuaStubs(*m_State, "/Users/jmorton/Dev/Lumos-Dev/Lumos/Source/Lumos/Scripting/Lua/LuaStubs.lua");

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

        static int s_OnUpdateInvocationsLogged = 0;
        if(s_OnUpdateInvocationsLogged < 3)
        {
            LINFO("[LuaManager] OnUpdate scene='%s' viewSize=%zu",
                  scene->GetSceneName().c_str(), (size_t)view.size());
            s_OnUpdateInvocationsLogged++;
        }

        if(view.empty())
            return;

        float dt          = (float)Engine::Get().GetTimeStep().GetSeconds();
        bool checkReload  = (m_ReloadFrameCounter++ % 60) == 0;

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);

            if(checkReload)
                luaScript.CheckForReload();
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

        // Lua require() search paths. `?` is replaced with the dotted module name (with '.' -> '/').
        // The leading "<Scripts>/?.lua" handles flat names like require("util/Math") -> <Scripts>/util/Math.lua.
        // The "<Scripts>/lua/?.lua" entry keeps legacy layouts (luarocks-style) working.
        std::string scripts = std::string((const char*)ScriptsPath.str);
        std::string package_path;
        package_path += scripts + "/?.lua;";
        package_path += scripts + "/lua/?.lua;";

        std::string currentPaths = state["package"]["path"];
        state["package"]["path"] = package_path + currentPaths;

        ScratchEnd(Scratch);

        // VFS searcher — handles require() from mounted .lpak (and any //Assets path).
        // Lua passes "foo.bar" → try //Assets/Scripts/foo/bar.lua and //Assets/Scripts/foo/bar/init.lua.
        auto vfsSearcher = [](sol::this_state s, const std::string& modName) -> sol::object
        {
            sol::state_view lua(s);

            std::string rel = modName;
            for(auto& c : rel) if(c == '.') c = '/';

            const std::string candidates[] = {
                "//Assets/Scripts/" + rel + ".lua",
                "//Assets/Scripts/" + rel + "/init.lua",
                "//Assets/Scripts/lua/" + rel + ".lua"
            };

            for(const auto& vfsPath : candidates)
            {
                ArenaTemp scratch = ScratchBegin(0, 0);
                String8 path = Str8((u8*)vfsPath.c_str(), vfsPath.size());
                String8 contents = FileSystem::Get().ReadTextFileVFS(scratch.arena, path);
                if(contents.size > 0 && contents.str != nullptr)
                {
                    std::string code((const char*)contents.str, contents.size);
                    std::string chunkName = "@" + vfsPath;
                    ScratchEnd(scratch);

                    // Loader closure: when require() calls it, compile + run the chunk.
                    return sol::make_object(lua, [code, chunkName, vfsPath](sol::this_state ts) -> sol::object
                    {
                        sol::state_view l(ts);
                        sol::load_result lr = l.load_buffer(code.data(), code.size(), chunkName);
                        if(!lr.valid())
                        {
                            sol::error e = lr;
                            LERROR("require('%s') compile failed: %s", vfsPath.c_str(), e.what());
                            return sol::make_object(l, sol::lua_nil);
                        }
                        sol::protected_function fn = lr;
                        sol::protected_function_result r = fn();
                        if(!r.valid())
                        {
                            sol::error e = r;
                            LERROR("require('%s') run failed: %s", vfsPath.c_str(), e.what());
                            return sol::make_object(l, sol::lua_nil);
                        }
                        return r.get<sol::object>();
                    });
                }
                ScratchEnd(scratch);
            }

            return sol::make_object(lua, "\n\tno VFS module '" + modName + "'");
        };

        sol::table searchers = state["package"]["searchers"];
        if(!searchers.valid())
            searchers = state["package"]["loaders"]; // Lua 5.1 fallback
        // Append rather than prepend so disk-based modules (debug iteration) keep priority.
        searchers[searchers.size() + 1] = vfsSearcher;
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

    sol::table GetAllEntities(sol::this_state s)
    {
        sol::state_view lua(s);
        sol::table result = lua.create_table();

        Scene* scene = Application::Get().GetCurrentScene();
        if(scene)
        {
            auto& registry = scene->GetRegistry();
            int i = 1;
            for(auto [e] : registry.storage<entt::entity>().each())
                result[i++] = Entity(e, scene);
        }
        return result;
    }

    std::tuple<sol::object, sol::object, sol::object> EachEntity(sol::this_state s)
    {
        sol::state_view lua(s);
        Scene* scene = Application::Get().GetCurrentScene();

        sol::object nilObj = sol::make_object(lua, sol::lua_nil);

        if(!scene)
            return std::make_tuple(nilObj, nilObj, nilObj);

        auto entities = std::make_shared<std::vector<entt::entity>>();
        for(auto [e] : scene->GetRegistry().storage<entt::entity>().each())
            entities->push_back(e);

        Scene* scenePtr = scene;

        sol::function iterator = lua.script(R"(
            return function(state, _)
                local idx = state.index
                if idx > #state.entities then
                    return nil
                end
                state.index = idx + 1
                return state.entities[idx]
            end
        )");

        sol::table state = lua.create_table();
        sol::table entitiesTable = lua.create_table();
        int i = 1;
        for(auto e : *entities)
        {
            entitiesTable[i++] = Entity(e, scenePtr);
        }
        state["entities"] = entitiesTable;
        state["index"] = 1;

        return std::make_tuple(sol::make_object(lua, iterator), sol::make_object(lua, state), nilObj);
    }

    void LuaManager::BindVoxelLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        auto voxel = state.create_table("Voxel");

        // Resolve the active scene's voxel world (first VoxelWorldComponent).
        auto getWorld = []() -> Graphics::VoxelWorld*
        {
            Scene* scene = Application::Get().GetCurrentScene();
            if(!scene)
                return nullptr;
            auto& reg = scene->GetRegistry();
            auto v    = reg.view<VoxelWorldComponent>();
            if(v.begin() == v.end())
                return nullptr;
            return reg.get<VoxelWorldComponent>(v.front()).World.get();
        };

        voxel.set_function("GetBlock", [getWorld](int x, int y, int z) -> int
                           { auto w = getWorld(); return w ? (int)w->GetBlock(x, y, z) : 0; });

        voxel.set_function("SetBlock", [getWorld](int x, int y, int z, int id)
                           { if(auto w = getWorld()) w->SetBlock(x, y, z, (Graphics::BlockID)id); });

        voxel.set_function("Raycast", [getWorld](sol::this_state s, float ox, float oy, float oz, float dx, float dy, float dz, float dist) -> sol::table
                           {
                               sol::state_view lua(s);
                               sol::table t = lua.create_table();
                               auto w       = getWorld();
                               Graphics::VoxelWorld::RayHit hit;
                               bool ok = w && w->Raycast(Vec3(ox, oy, oz), Vec3(dx, dy, dz), dist, hit);
                               t["hit"] = ok;
                               if(ok)
                               {
                                   t["x"]  = hit.bx; t["y"]  = hit.by; t["z"]  = hit.bz;
                                   t["nx"] = hit.nx; t["ny"] = hit.ny; t["nz"] = hit.nz;
                                   t["distance"] = hit.distance;
                               }
                               return t;
                           });

        voxel.set_function("Place", [getWorld](float ox, float oy, float oz, float dx, float dy, float dz, float dist, int id) -> bool
                           { auto w = getWorld(); return w ? w->PlaceBlock(Vec3(ox, oy, oz), Vec3(dx, dy, dz), dist, (Graphics::BlockID)id) : false; });

        voxel.set_function("Remove", [getWorld](float ox, float oy, float oz, float dx, float dy, float dz, float dist) -> bool
                           { auto w = getWorld(); return w ? w->RemoveBlock(Vec3(ox, oy, oz), Vec3(dx, dy, dz), dist) : false; });

        voxel.set_function("OverlapAABB", [getWorld](float minx, float miny, float minz, float maxx, float maxy, float maxz) -> bool
                           { auto w = getWorld(); return w ? w->OverlapAABB(Vec3(minx, miny, minz), Vec3(maxx, maxy, maxz)) : false; });

        // Transform of the camera the renderer actually draws through (the first in
        // the scene). Lets a controller script drive the real view regardless of
        // which entity it's attached to, and avoids the "two cameras" trap.
        state.set_function("GetMainCameraTransform", []() -> Maths::Transform*
                           {
                               Scene* scene = Application::Get().GetCurrentScene();
                               if(!scene)
                                   return nullptr;
                               auto& reg = scene->GetRegistry();
                               auto view = reg.view<Camera>();
                               if(view.begin() == view.end())
                                   return nullptr;
                               return reg.try_get<Maths::Transform>(view.front());
                           });

        // First directional light in the scene (the sun) + its transform. Colour and
        // Intensity can be set on the Light directly, but Direction is recomputed from
        // the entity's transform every frame, so rotate the transform to move the sun.
        auto findSun = []() -> entt::entity
        {
            Scene* scene = Application::Get().GetCurrentScene();
            if(!scene)
                return entt::null;
            auto& reg = scene->GetRegistry();
            auto view = reg.view<Graphics::Light>();
            for(auto e : view)
                if(view.get<Graphics::Light>(e).Type == (float)Graphics::LightType::DirectionalLight)
                    return e;
            return entt::null;
        };
        state.set_function("GetSunLight", [findSun]() -> Graphics::Light*
                           {
                               entt::entity e = findSun();
                               if(e == entt::null)
                                   return nullptr;
                               return &Application::Get().GetCurrentScene()->GetRegistry().get<Graphics::Light>(e);
                           });
        state.set_function("GetSunTransform", [findSun]() -> Maths::Transform*
                           {
                               entt::entity e = findSun();
                               if(e == entt::null)
                                   return nullptr;
                               return Application::Get().GetCurrentScene()->GetRegistry().try_get<Maths::Transform>(e);
                           });

        // First Environment component in the scene — sky colours, fog, stars, clouds.
        state.set_function("GetSceneEnvironment", []() -> Graphics::Environment*
                           {
                               Scene* scene = Application::Get().GetCurrentScene();
                               if(!scene)
                                   return nullptr;
                               auto& reg  = scene->GetRegistry();
            auto  view = reg.view<Graphics::Environment>();
                               if(view.begin() == view.end())
                                   return nullptr;
            return &reg.get<Graphics::Environment>(view.front());
                           });
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
                           {
                               // Scale to framebuffer pixels so scripts can compare
                               // against GetScreenSize() — raw GLFW cursor coords are
                               // logical points (half-res on retina), which broke any
                               // touch-pad half-screen split on desktop.
                               Vec2 pos = Input::Get().GetMousePosition();
                               Window* w = Application::Get().GetWindow();
                               return w ? pos * w->GetDPIScale() : pos;
                           });

        input.set_function("GetScrollOffset", []() -> float
                           { return Input::Get().Get().GetScrollOffset(); });

        // Capture (lock + hide) the cursor for relative free-look, like the editor
        // camera. Captured mode reports virtual deltas via GetMousePosition and the
        // cursor can't reach the screen edge.
        input.set_function("SetMouseCaptured", [](bool captured)
                           {
                               Window* w = Application::Get().GetWindow();
                               if(w) w->HideMouse(captured);
                               Input::Get().SetMouseMode(captured ? MouseMode::Captured : MouseMode::Visible);
                           });

        input.set_function("GetControllerAxis", [](int id, int axis) -> float
                           { return Input::Get().GetControllerAxis(id, axis); });

        input.set_function("GetControllerName", [](int id) -> std::string
                           { return Input::Get().GetControllerName(id); });

        input.set_function("GetControllerHat", [](int id, int hat) -> int
                           { return Input::Get().GetControllerHat(id, hat); });

        input.set_function("IsControllerButtonPressed", [](int id, int button) -> bool
                           { return Input::Get().IsControllerButtonPressed(id, button); });

        // Touch / gesture
        input.set_function("GetPanActive", []() -> bool
                           { return Input::Get().GetGesturePanActive(); });
        input.set_function("GetPanTranslation", []() -> Vec2
                           { return Input::Get().GetGesturePanTranslation(); });
        input.set_function("GetPanVelocity", []() -> Vec2
                           { return Input::Get().GetGesturePanVelocity(); });
        input.set_function("GetPanTouchCount", []() -> uint32_t
                           { return Input::Get().GetGesturePanTouchCount(); });
        input.set_function("GetPinchActive", []() -> bool
                           { return Input::Get().GetGesturePinchActive(); });
        input.set_function("GetPinchScale", []() -> float
                           { return Input::Get().GetGesturePinchScale(); });
        input.set_function("GetPinchVelocity", []() -> float
                           { return Input::Get().GetGesturePinchVelocity(); });
        input.set_function("GetLongPressActive", []() -> bool
                           { return Input::Get().GetGestureLongPressActive(); });
        input.set_function("GetLongPressPosition", []() -> Vec2
                           { return Input::Get().GetGestureLongPressLocation(); });

        input.set_function("GetScreenSize", []() -> Vec2
                           { Window* w = Application::Get().GetWindow(); return w ? Vec2((float)w->GetWidth(), (float)w->GetHeight()) : Vec2(0.0f, 0.0f); });

        // Framebuffer / logical-point ratio (2 on retina). Widget helpers like
        // UIProgressBar take LOGICAL sizes and scale internally — scripts that
        // size off GetScreenSize() (framebuffer px) must divide by this.
        input.set_function("GetDPIScale", []() -> float
                           { Window* w = Application::Get().GetWindow(); return w ? w->GetDPIScale() : 1.0f; });

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
        state.set_function("GetAllEntities", &GetAllEntities);
        state.set_function("EachEntity", &EachEntity);

        state.set_function("AddPyramidEntity", &EntityFactory::AddPyramid);
        state.set_function("AddSphereEntity", &EntityFactory::AddSphere);
        state.set_function("AddLightCubeEntity", &EntityFactory::AddLightCube);
        state.set_function("AddPlatform", &EntityFactory::AddPlatform);
        // Wrap with lambdas so sol2-callers can omit trailing args (sol2 ignores C++ defaults).
        state.set_function("AddTerrain", [](Scene* scene, const Vec3& pos,
                                            sol::optional<int> gridSize,
                                            sol::optional<float> scaleXZ,
                                            sol::optional<float> heightScale) -> Entity
                           {
                               return EntityFactory::AddTerrain(scene, pos,
                                   gridSize.value_or(256),
                                   scaleXZ.value_or(1.0f),
                                   heightScale.value_or(60.0f));
                           });
        // Terrain with a caller-supplied material table. Lua call shape:
        //   AddTerrainEx(scene, pos, gridSize, scaleXZ, heightScale, {
        //       albedoColour = Vec4(r, g, b, 1),
        //       roughness = 0..1, metallic = 0..1, uvTile = float,
        //       albedo       = "//Assets/Textures/.../albedo.png",
        //       normal       = "//Assets/Textures/.../normal.png",
        //       roughnessMap = "//Assets/Textures/.../roughness.png" })
        // Any field is optional; omitted texture paths skip that texture binding.
        state.set_function("AddTerrainEx", [](Scene* scene, const Vec3& pos, int gridSize,
                                              float scaleXZ, float heightScale,
                                              sol::optional<sol::table> matTbl) -> Entity
                           {
                               EntityFactory::TerrainMaterial mat;
                               if(matTbl)
                               {
                                   sol::table t = matTbl.value();
                                   if(t["albedoColour"].valid()) mat.albedoColour = t.get<Vec4>("albedoColour");
                                   if(t["roughness"].valid())    mat.roughness    = t.get<float>("roughness");
                                   if(t["metallic"].valid())     mat.metallic     = t.get<float>("metallic");
                                   if(t["uvTile"].valid())       mat.uvTile       = t.get<float>("uvTile");
                                   if(t["albedo"].valid())       mat.albedoPath   = t.get<std::string>("albedo");
                                   if(t["normal"].valid())       mat.normalPath   = t.get<std::string>("normal");
                                   if(t["roughnessMap"].valid()) mat.roughnessPath = t.get<std::string>("roughnessMap");
                                   if(t["noCollider"].valid())   mat.noCollider   = t.get<bool>("noCollider");
                               }
                               return EntityFactory::AddTerrainEx(scene, pos, gridSize, scaleXZ, heightScale, mat);
                           });
        state.set_function("AddArrow", [](Scene* scene, const Vec3& pos, const Vec3& velocity,
                                          sol::optional<float> radius,
                                          sol::optional<float> length,
                                          sol::optional<float> mass) -> Entity
                           {
                               return EntityFactory::AddArrow(scene, pos, velocity,
                                   radius.value_or(0.08f),
                                   length.value_or(0.9f),
                                   mass.value_or(0.2f));
                           });
        state.set_function("AddTarget", [](Scene* scene, const Vec3& pos,
                                           sol::optional<float> radius,
                                           sol::optional<Vec4> colour) -> Entity
                           {
                               return EntityFactory::AddTarget(scene, pos,
                                   radius.value_or(4.0f),
                                   colour.value_or(Vec4(1.0f, 0.85f, 0.15f, 1.0f)));
                           });

        // Deterministic, no-physics primitives for decor/visual stand-ins
        // (scene props, target ring discs, cloud blobs). Caller passes exact
        // position/scale/colour so visuals stay stable across runs. Optional
        // name lets the hierarchy panel identify the entity (e.g.
        // "TargetRing_3", "Rock", "Cloud") instead of a generic "Decor".
        state.set_function("AddDecorSphere", sol::overload(
            [](Scene* scene, const Vec3& pos, float radius, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildSphereObject(scene, "Decor", pos, radius, false, 0.0f, false, colour);
            },
            [](Scene* scene, const std::string& name, const Vec3& pos, float radius, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildSphereObject(scene, name, pos, radius, false, 0.0f, false, colour);
            }));
        state.set_function("AddDecorCube", sol::overload(
            [](Scene* scene, const Vec3& pos, const Vec3& scale, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildCuboidObject(scene, "Decor", pos, scale, false, 0.0f, false, colour);
            },
            [](Scene* scene, const std::string& name, const Vec3& pos, const Vec3& scale, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildCuboidObject(scene, name, pos, scale, false, 0.0f, false, colour);
            }));

        // Physics-enabled sphere — collidable rigid body. inverseMass=0 makes it
        // static. Used by pyramid stacks and any prop that needs to react to
        // collisions / fall.
        state.set_function("AddPhysicsSphere",
            [](Scene* scene, const std::string& name, const Vec3& pos, float radius,
               float inverseMass, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildSphereObject(scene, name, pos, radius, true, inverseMass, true, colour);
            });

        // Physics-enabled cuboid with a coloured material. inverseMass=0 => static
        // (infinite mass). halfExtents are half-dimensions. Returns the entity.
        state.set_function("AddPhysicsCube",
            [](Scene* scene, const std::string& name, const Vec3& pos, const Vec3& halfExtents,
               float inverseMass, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildCuboidObject(scene, name, pos, halfExtents, true, inverseMass, true, colour);
            });

        // Visual-only primitive (no physics) with a coloured per-instance material.
        // scale is the full local scale. Use for decor of any PrimitiveType
        // (cylinders, etc.) where AddDecorCube/Sphere don't cover the shape.
        state.set_function("AddColouredPrimitive",
            [](Scene* scene, const std::string& name, const Vec3& pos, const Vec3& scale,
               Graphics::PrimitiveType prim, const Vec4& colour) -> Entity
            {
                auto e = scene->GetEntityManager()->Create(name);
                e.AddComponent<Maths::Transform>(Mat4::Translation(pos) * Mat4::Scale(scale));
                auto& mc = e.AddComponent<Graphics::ModelComponent>(prim);

                auto mat = CreateSharedPtr<Graphics::Material>();
                Graphics::MaterialProperties props;
                props.albedoColour       = colour;
                props.roughness          = 0.7f;
                props.metallic           = 0.0f;
                props.emissive           = 0.0f;
                props.albedoMapFactor    = 0.0f;
                props.roughnessMapFactor = 0.0f;
                props.normalMapFactor    = 0.0f;
                props.metallicMapFactor  = 0.0f;
                props.emissiveMapFactor  = 0.0f;
                props.occlusionMapFactor = 0.0f;
                mat->SetMaterialProperites(props);
                mc.InstanceMaterials[0] = mat;
                return e;
            });

        // Quick material-pulse helper for game scripts: rewrite albedo +
        // emissive on the entity's first instanced material and push to GPU.
        // Cheap enough to call every frame on a handful of bead entities.
        state.set_function("SetEntityPulse",
            [](Entity entity, const Vec4& albedo, float emissive)
            {
                if(!entity.Valid()) return;
                if(!entity.HasComponent<Graphics::ModelComponent>()) return;
                auto& modelComp = entity.GetComponent<Graphics::ModelComponent>();
                if(!modelComp.ModelRef) return;
                auto it = modelComp.InstanceMaterials.find(0);
                if(it == modelComp.InstanceMaterials.end() || !it->second) return;
                auto& mat   = it->second;
                auto* props = mat->GetProperties();
                if(!props) return;
                props->albedoColour = albedo;
                props->emissive     = emissive;
                mat->UpdateMaterialPropertiesData();
            });

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
        // Expose as properties so Lua can do `light.Intensity = v` (matches the
        // pattern used in Script.lua and Transform field bindings).
        lightType["Intensity"] = &Light::Intensity;
        lightType["Radius"]    = &Light::Radius;
        lightType["Colour"]    = &Light::Colour;
        lightType["Direction"] = &Light::Direction;
        lightType["Position"]  = &Light::Position;
        lightType["Type"]      = &Light::Type;
        lightType["Angle"]     = &Light::Angle;

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
        Modeltype["file_path"]      = &Lumos::Graphics::Model::GetFilePath;
        Modeltype["primitive_type"] = sol::property(&Lumos::Graphics::Model::GetPrimitiveType, &Lumos::Graphics::Model::SetPrimitiveType);

        // Methods
        Modeltype["add_mesh"]   = &Lumos::Graphics::Model::AddMesh;
        Modeltype["load_model"] = &Lumos::Graphics::Model::LoadModel;

        Modeltype["GetMeshCount"] = [](Lumos::Graphics::Model& model) -> int {
            return static_cast<int>(model.GetMeshes().Size());
        };

        Modeltype["GetTotalStats"] = [](Lumos::Graphics::Model& model, sol::this_state s) -> sol::table {
            sol::state_view lua(s);
            sol::table stats = lua.create_table();
            uint32_t totalVerts = 0, totalTris = 0, totalIndices = 0;
            auto& meshes = model.GetMeshes();
            for(size_t i = 0; i < meshes.Size(); i++)
            {
                auto meshStats = meshes[i]->GetStats();
                totalVerts += meshStats.VertexCount;
                totalTris += meshStats.TriangleCount;
                totalIndices += meshStats.IndexCount;
            }
            stats["vertices"]  = totalVerts;
            stats["triangles"] = totalTris;
            stats["indices"]   = totalIndices;
            stats["meshes"]    = meshes.Size();
            return stats;
        };

        Modeltype["GetBounds"] = [](Lumos::Graphics::Model& model, sol::this_state s) -> sol::table {
            sol::state_view lua(s);
            sol::table bounds = lua.create_table();
            Vec3 bMin(FLT_MAX), bMax(-FLT_MAX);
            auto& meshes = model.GetMeshes();
            for(size_t i = 0; i < meshes.Size(); i++)
            {
                auto& bb = meshes[i]->GetBoundingBox();
                bMin = Vec3(std::min(bMin.x, bb.m_Min.x), std::min(bMin.y, bb.m_Min.y), std::min(bMin.z, bb.m_Min.z));
                bMax = Vec3(std::max(bMax.x, bb.m_Max.x), std::max(bMax.y, bb.m_Max.y), std::max(bMax.z, bb.m_Max.z));
            }
            bounds["min"] = bMin;
            bounds["max"] = bMax;
            Vec3 center = (bMin + bMax) * 0.5f;
            Vec3 extents = bMax - bMin;
            bounds["center"] = center;
            bounds["extents"] = extents;
            bounds["radius"] = extents.Length() * 0.5f;
            return bounds;
        };

        auto modelCompType = state.new_usertype<ModelComponent>("ModelComponent");
        modelCompType["GetModel"] = [](ModelComponent& mc) -> Model* { return mc.ModelRef.get(); };
        modelCompType["ModelRef"]  = &ModelComponent::ModelRef;
        modelCompType["LoadPrimitive"] = &ModelComponent::LoadPrimitive;

        REGISTER_COMPONENT_WITH_ECS(state, ModelComponent, sol::overload(
            static_cast<ModelComponent& (Entity::*)(const std::string&)>(&Entity::AddComponent<ModelComponent, const std::string&>),
            [](Entity& e, PrimitiveType p) -> ModelComponent& { return e.AddComponent<ModelComponent>(p); }
        ));

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
        camera_type["SetFOV"]            = &Camera::SetFOV;
        camera_type["SetAspectRatio"]    = &Camera::SetAspectRatio;
        camera_type["SetScale"]          = &Camera::SetScale;
        camera_type["IsOrthographic"]    = &Camera::IsOrthographic;

        REGISTER_COMPONENT_WITH_ECS(state, Camera, static_cast<Camera& (Entity::*)(const float&, const float&)>(&Entity::AddComponent<Camera, const float&, const float&>));

        sol::usertype<RigidBody3DComponent> RigidBody3DComponent_type = state.new_usertype<RigidBody3DComponent>("RigidBody3DComponent", sol::constructors<sol::types<RigidBody3D*>>());
        RigidBody3DComponent_type.set_function("GetRigidBody", &RigidBody3DComponent::GetRigidBody);

        REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)(const RigidBody3DProperties&)>(&Entity::AddComponent<RigidBody3DComponent, const RigidBody3DProperties&>));
        // REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)>(&Entity::AddComponent<RigidBody3DComponent));

        sol::usertype<RigidBody2DComponent> RigidBody2DComponent_type = state.new_usertype<RigidBody2DComponent>("RigidBody2DComponent", sol::constructors<sol::types<const RigidBodyParameters&>>());
        RigidBody2DComponent_type.set_function("GetRigidBody", &RigidBody2DComponent::GetRigidBody);

        REGISTER_COMPONENT_WITH_ECS(state, RigidBody2DComponent, static_cast<RigidBody2DComponent& (Entity::*)(const RigidBodyParameters&)>(&Entity::AddComponent<RigidBody2DComponent, const RigidBodyParameters&>));

        REGISTER_COMPONENT_WITH_ECS(state, SoundComponent, static_cast<SoundComponent& (Entity::*)()>(&Entity::AddComponent<SoundComponent>));

        // Expose GetSoundNode so Lua can swap the per-entity sound + drive it
        // (Play/Stop/SetVolume/Pitch) without needing a separate SoundComponent
        // constructor binding.
        sol::usertype<SoundComponent> soundCompType = state.new_usertype<SoundComponent>("SoundComponent");
        soundCompType.set_function("GetSoundNode", &SoundComponent::GetSoundNode);

        // Sound
        sol::usertype<Sound> sound_type = state.new_usertype<Sound>("Sound");
        sound_type.set_function("GetLength", &Sound::GetLength);
        sound_type.set_function("GetFilePath", &Sound::GetFilePath);

        auto soundTable         = state["Sound"].get_or_create<sol::table>();
        soundTable["Create"]    = sol::overload(
            [](const std::string& filePath, const std::string& ext) -> SharedPtr<Sound>
            {
                return Sound::Create(filePath, ext);
            },
            [](const std::string& name, const std::string& filePath, const std::string& ext) -> SharedPtr<Sound>
            {
                return Sound::Create(filePath, ext);
            });

        // SoundNode
        sol::usertype<SoundNode> soundNode_type = state.new_usertype<SoundNode>("SoundNode");
        soundNode_type.set_function("Play", [](SoundNode& node) { node.SetPaused(false); });
        soundNode_type.set_function("Pause", &SoundNode::Pause);
        soundNode_type.set_function("Resume", &SoundNode::Resume);
        soundNode_type.set_function("Stop", &SoundNode::Stop);
        soundNode_type.set_function("SetVolume", &SoundNode::SetVolume);
        soundNode_type.set_function("GetVolume", &SoundNode::GetVolume);
        soundNode_type.set_function("SetLooping", &SoundNode::SetLooping);
        soundNode_type.set_function("GetLooping", &SoundNode::GetLooping);
        soundNode_type.set_function("SetPitch", &SoundNode::SetPitch);
        soundNode_type.set_function("GetPitch", &SoundNode::GetPitch);
        soundNode_type.set_function("SetSound", &SoundNode::SetSound);
        soundNode_type.set_function("GetSound", &SoundNode::GetSound);

        auto soundNodeTable      = state["SoundNode"].get_or_create<sol::table>();
        soundNodeTable["Create"] = sol::overload(
            []() -> SharedPtr<SoundNode>
            {
                return SharedPtr<SoundNode>(SoundNode::Create());
            },
            [](SharedPtr<Sound> sound) -> SharedPtr<SoundNode>
            {
                auto node = SharedPtr<SoundNode>(SoundNode::Create());
                if(node)
                    node->SetSound(sound);
                return node;
            });

        sol::usertype<Environment> environment_type = state.new_usertype<Environment>("Environment");
        environment_type.set_function("GetFilePath", &Environment::GetFilePath);
        environment_type.set_function("GetFileType", &Environment::GetFileType);
        environment_type.set_function("GetNumMips", &Environment::GetNumMips);
        environment_type.set_function("GetWidth", &Environment::GetWidth);
        environment_type.set_function("GetHeight", &Environment::GetHeight);
        environment_type.set_function("GetMode", &Environment::GetMode);
        environment_type.set_function("GetParameters", &Environment::GetParameters);
        environment_type.set_function("SetFilePath", &Environment::SetFilePath);
        environment_type.set_function("SetFileType", &Environment::SetFileType);
        environment_type.set_function("SetNumMips", &Environment::SetNumMips);
        environment_type.set_function("SetWidth", &Environment::SetWidth);
        environment_type.set_function("SetHeight", &Environment::SetHeight);
        environment_type.set_function("SetMode", &Environment::SetMode);
        environment_type.set_function("SetParameters", &Environment::SetParameters);
        environment_type.set_function("GetHorizonColour", &Environment::GetHorizonColour);
        environment_type.set_function("GetZenithColour",  &Environment::GetZenithColour);
        environment_type.set_function("GetSunDirection",  &Environment::GetSunDirection);
        environment_type.set_function("SetHorizonColour", &Environment::SetHorizonColour);
        environment_type.set_function("SetZenithColour",  &Environment::SetZenithColour);
        environment_type.set_function("SetSunDirection",  &Environment::SetSunDirection);
        // Fog. FogParams: (density, heightFalloff, linearStart, linearEnd).
        // FogColour.a is strength (0 disables).
        environment_type.set_function("GetFogColour",    &Environment::GetFogColour);
        environment_type.set_function("GetFogParams",    &Environment::GetFogParams);
        environment_type.set_function("SetFogColour",    &Environment::SetFogColour);
        environment_type.set_function("SetFogParams",    &Environment::SetFogParams);
        // Clouds. CloudParams: (coverage, density, speed, styleMode -1/0/1).
        environment_type.set_function("GetCloudColour",  &Environment::GetCloudColour);
        environment_type.set_function("GetCloudParams",  &Environment::GetCloudParams);
        environment_type.set_function("GetCloudWindDir", &Environment::GetCloudWindDir);
        environment_type.set_function("SetCloudColour",  &Environment::SetCloudColour);
        environment_type.set_function("SetCloudParams",  &Environment::SetCloudParams);
        environment_type.set_function("SetCloudWindDir", &Environment::SetCloudWindDir);
        // Stars. StarParams: (density 0..1, brightness, twinkleSpeed, skyLumaThreshold).
        // density==0 disables. Mostly visible above the horizon when the sky tint
        // is darker than the threshold (so daytime skies hide them automatically).
        environment_type.set_function("GetStarParams",  &Environment::GetStarParams);
        environment_type.set_function("GetStarColour",  &Environment::GetStarColour);
        environment_type.set_function("SetStarParams",  &Environment::SetStarParams);
        environment_type.set_function("SetStarColour",  &Environment::SetStarColour);
        // Aurora. AuroraParams: (intensity 0=off, verticalCentre, speed, width).
        // AuroraColour: rgb base tint, a = blend toward magenta tip.
        environment_type.set_function("GetAuroraColour", &Environment::GetAuroraColour);
        environment_type.set_function("GetAuroraParams", &Environment::GetAuroraParams);
        environment_type.set_function("SetAuroraColour", &Environment::SetAuroraColour);
        environment_type.set_function("SetAuroraParams", &Environment::SetAuroraParams);
        environment_type.set_function("Load", static_cast<void (Environment::*)()>(&Environment::Load));

        REGISTER_COMPONENT_WITH_ECS(state, Environment, static_cast<Environment& (Entity::*)()>(&Entity::AddComponent<Environment>));

        sol::usertype<Listener> listener_type = state.new_usertype<Listener>("Listener");
        listener_type["enabled"] = &Listener::m_Enabled;

        REGISTER_COMPONENT_WITH_ECS(state, Listener, static_cast<Listener& (Entity::*)()>(&Entity::AddComponent<Listener>));

        auto mesh_type = state.new_usertype<Lumos::Graphics::Mesh>("Mesh",
                                                                   sol::constructors<Lumos::Graphics::Mesh(), Lumos::Graphics::Mesh(const Lumos::Graphics::Mesh&),
                                                                                     Lumos::Graphics::Mesh(const TDArray<uint32_t>&, const TDArray<Vertex>&)>());

        // Bind the member functions and variables
        mesh_type["GetMaterial"]    = &Lumos::Graphics::Mesh::GetMaterial;
        mesh_type["SetMaterial"]    = &Lumos::Graphics::Mesh::SetMaterial;
        mesh_type["GetBoundingBox"] = &Lumos::Graphics::Mesh::GetBoundingBox;
        mesh_type["SetName"]        = &Lumos::Graphics::Mesh::SetName;
        mesh_type["GetName"]        = &Lumos::Graphics::Mesh::GetName;
        mesh_type["GetStats"]       = &Lumos::Graphics::Mesh::GetStats;

        auto meshStats_type              = state.new_usertype<Lumos::Graphics::MeshStats>("MeshStats");
        meshStats_type["TriangleCount"]  = &Lumos::Graphics::MeshStats::TriangleCount;
        meshStats_type["VertexCount"]    = &Lumos::Graphics::MeshStats::VertexCount;
        meshStats_type["IndexCount"]     = &Lumos::Graphics::MeshStats::IndexCount;

        auto bbox_type = state.new_usertype<Lumos::Maths::BoundingBox>("BoundingBox");
        bbox_type["min"]        = &Lumos::Maths::BoundingBox::m_Min;
        bbox_type["max"]        = &Lumos::Maths::BoundingBox::m_Max;
        bbox_type["GetExtents"] = &Lumos::Maths::BoundingBox::GetExtents;

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
        scene_type.set_function("CreateEntity", sol::overload(
            static_cast<Entity (Scene::*)()>(&Scene::CreateEntity),
            static_cast<Entity (Scene::*)(const std::string&)>(&Scene::CreateEntity)));

        state.set_function("GetCurrentScene", []() -> Scene* {
            return Application::Get().GetCurrentScene();
        });

        sol::usertype<Graphics::Texture2D> texture2D_type = state.new_usertype<Graphics::Texture2D>("Texture2D");
        texture2D_type.set_function("CreateFromFile", &Graphics::Texture2D::CreateFromFile);
        texture2D_type.set_function("GetWidth", [](Graphics::Texture2D& tex) { return tex.GetWidth(); });
        texture2D_type.set_function("GetHeight", [](Graphics::Texture2D& tex) { return tex.GetHeight(); });

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
        Application::Get().ExitApp();
    }

    static void SetSkyColour(const Vec3& rgb)
    {
        if(auto* r = Application::Get().GetSceneRenderer())
            r->SetClearColour(Vec4(rgb.x, rgb.y, rgb.z, 1.0f));
    }

    static void DebugLine(const Vec3& a, const Vec3& b, float thickness, const Vec4& colour)
    {
        DebugRenderer::DrawThickLine(a, b, thickness, true, colour, 0.0f);
    }

    static void DebugPoint(const Vec3& p, float radius, const Vec4& colour)
    {
        DebugRenderer::DrawPoint(p, radius, true, colour, 0.0f);
    }

    void LuaManager::BindAppLua(sol::state& state)
    {
        sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
        state.set_function("SwitchSceneByIndex", &SwitchSceneByIndex);
        state.set_function("SwitchSceneByName", &SwitchSceneByName);
        state.set_function("SwitchScene", &SwitchScene);
        state.set_function("SetPhysicsDebugFlags", &SetPhysicsDebugFlags);
        state.set_function("ExitApp", &ExitApp);
        state.set_function("SetSkyColour", &SetSkyColour);
        state.set_function("DebugLine", &DebugLine);
        state.set_function("DebugPoint", &DebugPoint);

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

        // File Dialogs
        auto fileDialog = state["FileDialog"].get_or_create<sol::table>();

        fileDialog.set_function("OpenFile", [](sol::optional<std::string> filter, sol::optional<std::string> defaultPath) -> std::string {
            return FileDialogs::OpenFile(filter.value_or(""), defaultPath.value_or(""));
        });

        fileDialog.set_function("SaveFile", [](sol::optional<std::string> filter, sol::optional<std::string> defaultPath, sol::optional<std::string> defaultName) -> std::string {
            return FileDialogs::SaveFile(filter.value_or(""), defaultPath.value_or(""), defaultName.value_or(""));
        });

        fileDialog.set_function("PickFolder", [](sol::optional<std::string> defaultPath) -> std::string {
            return FileDialogs::PickFolder(defaultPath.value_or(""));
        });

        // Screenshot / Image export
        state.set_function("SaveScreenshot", [](const std::string& path) {

            auto ScreenshotCB = Graphics::CommandBuffer::Create();
            ScreenshotCB->Init(true);
            ScreenshotCB->BeginRecording();
            auto renderer = Graphics::Renderer::GetRenderer();
            auto sceneRenderer = Application::Get().GetSceneRenderer();
            if(renderer && sceneRenderer)
                renderer->SaveScreenshot(path, sceneRenderer->GetMainTexture());

            ScreenshotCB->EndRecording();
            ScreenshotCB->Submit();
            ScreenshotCB->Flush();

            delete ScreenshotCB;
        });

        // imgui.image - display a Texture2D in ImGui
        sol::table imgui = state["imgui"].get_or_create<sol::table>();
        imgui.set_function("image", sol::overload(
            [](Graphics::Texture2D* texture, float w, float h)
            {
                if(!texture) return;
                auto imguiRenderer = Application::Get().GetImGuiManager()->GetImGuiRenderer();
                bool flip = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
                ImGui::Image(
                    imguiRenderer->AddTexture(texture),
                    ImVec2(w, h),
                    ImVec2(0.0f, flip ? 1.0f : 0.0f),
                    ImVec2(1.0f, flip ? 0.0f : 1.0f));
            },
            [](Graphics::Texture2D* texture)
            {
                if(!texture) return;
                float w = (float)texture->GetWidth();
                float h = (float)texture->GetHeight();
                auto imguiRenderer = Application::Get().GetImGuiManager()->GetImGuiRenderer();
                bool flip = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
                ImGui::Image(
                    imguiRenderer->AddTexture(texture),
                    ImVec2(w, h),
                    ImVec2(0.0f, flip ? 1.0f : 0.0f),
                    ImVec2(1.0f, flip ? 0.0f : 1.0f));
            }));

        // Write string to file
        state.set_function("WriteFile", [](const std::string& path, const std::string& content) -> bool {
            FILE* f = fopen(path.c_str(), "w");
            if(!f) return false;
            fwrite(content.c_str(), 1, content.size(), f);
            fclose(f);
            return true;
        });

        // Read file to string (supports VFS paths like //Assets/...)
        state.set_function("ReadFile", [](const std::string& path) -> std::string {
            std::string physicalPath = path;
            if(path.size() >= 2 && path[0] == '/' && path[1] == '/')
            {
                ArenaTemp scratch = ScratchBegin(0, 0);
                String8 resolved;
                if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8C((char*)path.c_str()), &resolved))
                    physicalPath = std::string((const char*)resolved.str, resolved.size);
                ScratchEnd(scratch);
            }
            FILE* f = fopen(physicalPath.c_str(), "r");
            if(!f) return "";
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            std::string content(sz, '\0');
            fread(&content[0], 1, sz, f);
            fclose(f);
            return content;
        });

        // Pack sprite sheet: takes output path, array of image paths, cols, frameW, frameH
        state.set_function("PackSpriteSheet", [](const std::string& outputPath, sol::table paths, int cols, int frameW, int frameH) -> bool {
            int count = (int)paths.size();
            if(count == 0 || cols <= 0 || frameW <= 0 || frameH <= 0)
                return false;

            int rows = (count + cols - 1) / cols;
            int atlasW = cols * frameW;
            int atlasH = rows * frameH;

            // Allocate atlas (RGBA, zeroed)
            std::vector<uint8_t> atlas(atlasW * atlasH * 4, 0);

            for(int i = 0; i < count; i++)
            {
                std::string imgPath = paths[i + 1]; // Lua 1-indexed
                uint32_t w = 0, h = 0, bits = 0;
                uint8_t* pixels = LoadImageFromFile(imgPath.c_str(), &w, &h, &bits);
                if(!pixels)
                    continue;

                int dstX = (i % cols) * frameW;
                int dstY = (i / cols) * frameH;

                // Copy sprite into atlas, clamp to frame bounds
                int copyW = std::min((int)w, frameW);
                int copyH = std::min((int)h, frameH);

                for(int y = 0; y < copyH; y++)
                {
                    int srcOffset = y * w * 4;
                    int dstOffset = ((dstY + y) * atlasW + dstX) * 4;
                    memcpy(&atlas[dstOffset], &pixels[srcOffset], copyW * 4);
                }

                delete[] pixels;
            }

            return ImageExport::SavePNG(outputPath, atlasW, atlasH, atlas.data());
        });

        // CompileGLSL - runtime GLSL to SPIR-V compilation
        state.set_function("CompileGLSL", [](const std::string& source, const std::string& typeStr) -> sol::table {
            sol::state_view sv(LuaManager::Get().GetState());
            sol::table result = sv.create_table();

            Graphics::ShaderType type = Graphics::ShaderType::FRAGMENT;
            if(typeStr == "vertex")
                type = Graphics::ShaderType::VERTEX;
            else if(typeStr == "compute")
                type = Graphics::ShaderType::COMPUTE;

            auto compileResult = Graphics::ShaderCompiler::Compile(source, type);
            result["success"]  = compileResult.success;
            result["error"]    = compileResult.error;
            return result;
        });

        // ShaderPreview usertype
        state.new_usertype<Graphics::ShaderPreview>("ShaderPreview",
            sol::constructors<Graphics::ShaderPreview(uint32_t, uint32_t)>(),
            "Compile", &Graphics::ShaderPreview::Compile,
            "Render", &Graphics::ShaderPreview::Render,
            "RenderAtResolution", &Graphics::ShaderPreview::RenderAtResolution,
            "GetTexture", &Graphics::ShaderPreview::GetOutputTexture,
            "GetExportTexture", &Graphics::ShaderPreview::GetExportTexture,
            "GetError", &Graphics::ShaderPreview::GetError,
            "IsCompiled", &Graphics::ShaderPreview::IsCompiled,
            "SetTexture", &Graphics::ShaderPreview::SetChannelTexture,
            "ClearTexture", &Graphics::ShaderPreview::ClearChannelTexture);

        // Get the project's asset directory as a physical path
        state.set_function("GetAssetPath", []() -> std::string {
            auto& path = FileSystem::Get().GetAssetPath();
            return std::string((const char*)path.str, path.size);
        });

        // Load a texture from file path
        state.set_function("LoadTexture2D", [](const std::string& path) -> Graphics::Texture2D* {
            return Graphics::Texture2D::CreateFromFile(path, path);
        });

        // Save an arbitrary texture to file
        state.set_function("SaveTextureToFile", [](Graphics::Texture2D* texture, const std::string& path) {
            auto renderer = Graphics::Renderer::GetRenderer();
            if(renderer && texture)
                renderer->SaveScreenshot(path, texture);
        });
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
                     "CentreChildrenX", Lumos::WidgetFlags_CentreChildrenX,
                     "CentreChildrenY", Lumos::WidgetFlags_CentreChildrenY,
                     "DragParent", Lumos::WidgetFlags_DragParent,
                     "AnimateScale", Lumos::WidgetFlags_AnimateScale,
                     "AnimateAppear", Lumos::WidgetFlags_AnimateAppear);

        lua.new_enum("UITextAlignment",
                     "None", Lumos::UI_Text_Alignment_None,
                     "Center_X", Lumos::UI_Text_Alignment_Center_X,
                     "Center_Y", Lumos::UI_Text_Alignment_Center_Y);

        lua.new_enum("SizeKind",
                     "Pixels", Lumos::SizeKind_Pixels,
                     "TextContent", Lumos::SizeKind_TextContent,
                     "PercentOfParent", Lumos::SizeKind_PercentOfParent,
                     "ChildSum", Lumos::SizeKind_ChildSum,
                     "MaxChild", Lumos::SizeKind_MaxChild,
                     "PercentOfViewport", Lumos::SizeKind_PercentOfViewport);

        lua.new_enum("UIAnchor",
                     "None", Lumos::UIAnchor_None,
                     "TopLeft", Lumos::UIAnchor_TopLeft,
                     "TopCenter", Lumos::UIAnchor_TopCenter,
                     "TopRight", Lumos::UIAnchor_TopRight,
                     "MiddleLeft", Lumos::UIAnchor_MiddleLeft,
                     "MiddleCenter", Lumos::UIAnchor_MiddleCenter,
                     "MiddleRight", Lumos::UIAnchor_MiddleRight,
                     "BottomLeft", Lumos::UIAnchor_BottomLeft,
                     "BottomCenter", Lumos::UIAnchor_BottomCenter,
                     "BottomRight", Lumos::UIAnchor_BottomRight);

        lua.new_enum("UIDock",
                     "Left", Lumos::Dock_Left,
                     "Right", Lumos::Dock_Right,
                     "Top", Lumos::Dock_Top,
                     "Bottom", Lumos::Dock_Bottom,
                     "Fill", Lumos::Dock_Fill);

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
                     "CornerRadius", Lumos::StyleVar_CornerRadius,
                     "ShadowColor", Lumos::StyleVar_ShadowColor,
                     "ShadowOffset", Lumos::StyleVar_ShadowOffset,
                     "ShadowBlur", Lumos::StyleVar_ShadowBlur,
                     "ItemSpacing", Lumos::StyleVar_ItemSpacing,
                     "Alpha", Lumos::StyleVar_Alpha,
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
        lua["UIEndPanel"]      = &Lumos::UIEndPanel;
        lua.set_function("UIBeginOverlay", [](const char* str,
                                              sol::optional<Lumos::SizeKind> sx, sol::optional<float> xv,
                                              sol::optional<Lumos::SizeKind> sy, sol::optional<float> yv,
                                              sol::optional<u32> flags) {
            return Lumos::UIBeginOverlay(str,
                                         sx.value_or(Lumos::SizeKind_MaxChild), xv.value_or(1.0f),
                                         sy.value_or(Lumos::SizeKind_ChildSum), yv.value_or(1.0f),
                                         flags.value_or((u32)Lumos::WidgetFlags_StackVertically));
        });
        lua.set_function("UIArrow", &Lumos::UIArrow);
        lua.set_function("UIWindowAnchor", [](Lumos::UIAnchor a, sol::optional<float> mx, sol::optional<float> my) {
            Lumos::UIWindowAnchor(a, mx.value_or(0.0f), my.value_or(0.0f));
        });
        lua.set_function("UIWindowDock", [](Lumos::UIDockPosition pos, sol::optional<float> sz) {
            Lumos::UIWindowDock(pos, sz.value_or(0.5f));
        });
        lua["UIWindowCenter"]     = &Lumos::UIWindowCenter;
        lua["UIWindowFillScreen"] = &Lumos::UIWindowFillScreen;
        lua["UIWindowSetSize"]    = &Lumos::UIWindowSetSize;
        lua["UIPushStyle"] = sol::overload(
            static_cast<void (*)(Lumos::StyleVar, float)>(&Lumos::UIPushStyle),
            static_cast<void (*)(Lumos::StyleVar, const Vec2&)>(&Lumos::UIPushStyle),
            static_cast<void (*)(Lumos::StyleVar, const Vec3&)>(&Lumos::UIPushStyle),
            static_cast<void (*)(Lumos::StyleVar, const Vec4&)>(&Lumos::UIPushStyle));

        lua["UIPopStyle"] = &Lumos::UIPopStyle;
        lua["UISetNextFlags"] = &Lumos::UISetNextFlags;
        lua["UILabel"]    = &Lumos::UILabelCStr;
        lua["UIButton"]   = &Lumos::UIButton;
        lua["UIImage"]    = &Lumos::UIImage;
        lua.set_function("UISeparator", [](sol::optional<float> w) { Lumos::UISeparator(w.value_or(0.0f)); });
        lua.set_function("UISpacer",    [](sol::optional<float> h) { Lumos::UISpacer(h.value_or(10.0f)); });
        // Raw UISlider takes float*; unsafe to call from Lua directly. Use UISliderValue below.
        lua["UISliderRaw"]  = &Lumos::UISlider;
        lua["UILayoutRoot"] = &Lumos::UILayoutRoot;

        // Lua-friendly UISlider: pass value by value, returns the (possibly user-modified) value.
        lua.set_function("UISlider", [](const char* label, float value, sol::optional<float> min,
                                         sol::optional<float> max, sol::optional<float> width,
                                         sol::optional<float> height, sol::optional<float> handle) -> float
                         {
                             float tmp = value;
                             Lumos::UISlider(label, &tmp,
                                             min.value_or(0.0f),
                                             max.value_or(1.0f),
                                             width.value_or(250.0f),
                                             height.value_or(20.0f),
                                             handle.value_or(0.1f));
                             return tmp;
                         });

        lua.set_function("UIProgressBar", [](const char* label, float progress,
                                              sol::optional<float> width, sol::optional<float> height)
                         {
                             return Lumos::UIProgressBar(label, progress,
                                                          width.value_or(200.0f),
                                                          height.value_or(20.0f));
                         });

        lua.set_function("UIToggle", [](const char* label, const bool& value)
                         {
                    bool tempValue = value;
                    Lumos::UIToggle(label, &tempValue);
                    return tempValue; });
    }

}
