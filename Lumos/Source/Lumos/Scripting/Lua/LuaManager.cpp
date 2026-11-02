#include "Precompiled.h"
#include "LuaManager.h"
#include "LuaBindingRegistry.h"
#include "Maths/Transform.h"
#include "Maths/Ray.h"
#include "Core/OS/Window.h"
#include "Core/OS/FileSystem.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Core/OS/OS.h"
#include "Core/Engine.h"
#include "Core/DebugMenu.h"
#include "Core/ToolPanels.h"
#include "Core/BusyOverlay.h"
#include "Core/Undo.h"
#include "Graphics/Gizmo.h"
#include "Core/OS/Input.h"
#include "Core/TestRunner.h"
#include "Scene/SceneManager.h"
#include "LuaScriptComponent.h"
#include "Scene/SceneGraph.h"
#include "Graphics/Camera/ThirdPersonCamera.h"
#include "Graphics/UI.h"
#include "Graphics/MDIIcons.h"

#include "Scene/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/Camera2D.h"

#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/Light2D.h"
#include "Graphics/Light.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Scene/Component/ModelComponent.h"
#include "Graphics/Animation/AnimationController.h"
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
#include "Scene/Component/DecalComponent.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Component/AIComponent.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/MeshCollisionShape.h"
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
#include "Utilities/CSV.h"
#include "Utilities/Colour.h"
#include "Graphics/PointCloud.h"
#include "Graphics/LineCloud.h"
#include "Graphics/WorldText.h"
#include "Graphics/PlanetField.h"
#include "Graphics/CosmoBackdrop.h"
#include <cmath>
#include <cstring>
#include <unordered_map>
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

#include "LuaSmartPtrTraits.h"
#include "MeshLua.h"
#include "AudioLua.h"
#include "SystemLua.h"
#include "WidgetsLua.h"

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
        AddMethod("Transform", "GetUpDirection",      "Vec3", "() -> Vec3");

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
        AddMethod("Input", "GetScrollOffsetX",         "float", "() -> float");
        AddMethod("Input", "SetMouseCaptured",         "",      "(captured: bool) -> void");
        AddMethod("Input", "GetMouseDelta",            "Vec2",  "() -> Vec2  -- movement this frame, pixels");
        AddMethod("Input", "Haptic",                   "",      "(kind?: string) -> void");
        AddMethod("Input", "GetControllerAxis",        "float", "(id: int, axis: int) -> float");
        AddMethod("Input", "GetControllerName",        "string","(id: int) -> string");
        AddMethod("Input", "GetControllerHat",         "int",   "(id: int, hat: int) -> int");
        AddMethod("Input", "IsControllerButtonPressed","bool",  "(id: int, button: int) -> bool");
        AddMethod("Input", "SetMousePosition",         "void",  "(x: float, y: float) - drives UI hit-testing");
        AddMethod("Input", "SetMouseClicked",          "void",  "(button: MouseButton, down: bool)");
        AddMethod("Input", "SetMouseHeld",             "void",  "(button: MouseButton, down: bool)");
        AddMethod("Input", "SetKeyPressed",            "void",  "(key: Key, down: bool)");
        AddMethod("Input", "SetKeyHeld",               "void",  "(key: Key, down: bool)");
        AddMethod("Input", "SetScrollOffset",          "void",  "(offset: float)");
        AddMethod("Input", "SetScrollOffsetX",         "void",  "(offset: float)");
        AddMethod("Input", "SetTouchCount",            "void",  "(count: int)");
        AddMethod("Input", "SetMouseOnScreen",         "void",  "(onScreen: bool)");
        AddMethod("Input", "GamepadPresent",           "bool",  "() -> bool");
        AddMethod("Input", "GamepadName",              "string","() -> string");
        AddMethod("Input", "GamepadAxis",              "float", "(axis: GamepadAxis) -> float (-1..1; triggers 0..1)");
        AddMethod("Input", "GamepadHeld",              "bool",  "(button: GamepadButton) -> bool");
        AddMethod("Input", "GamepadPressed",           "bool",  "(button: GamepadButton) -> bool (this frame only)");
        AddMethod("Input", "SetGamepadDeadZone",       "void",  "(deadZone: float)");
        AddMethod("Input", "AddGamepadMappings",       "bool",  "(sdlMappings: string) -> bool");
        AddMethod("Input", "GetPanActive",             "bool",  "() -> bool");
        AddMethod("Input", "GetPanTranslation",        "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetPanVelocity",           "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetPanTouchCount",         "int",   "() -> int");
        AddMethod("Input", "GetPinchActive",           "bool",  "() -> bool");
        AddMethod("Input", "GetPinchScale",            "float", "() -> float");
        AddMethod("Input", "GetPinchVelocity",         "float", "() -> float");
        AddMethod("Input", "GetLongPressActive",       "bool",  "() -> bool");
        AddMethod("Input", "GetLongPressPosition",     "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetTouchCount",            "int",   "() -> int");
        AddMethod("Input", "GetTouches",               "table", "() -> { {x, y, id}, ... } every finger down");
        AddMethod("Input", "SetTouches",               "void",  "(touches: table) -> fake fingers, for tests");
        AddMethod("Input", "GetScreenSize",            "Vec2",  "() -> Vec2");
        AddMethod("Input", "GetDPIScale",              "float", "() -> float");
        AddMethod("Input", "GetContentScale",          "float", "() -> float");
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

        AddGlobal("Raycast",         "RaycastHit", "(o: Vec3, d: Vec3, maxDist: float, ignore?: RigidBody3D) -> RaycastHit  -- ignore: skip this body (the one casting)");
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
        AddMethod("Physics", "SetContinuousCollision", "", "(enabled: bool) -> void  -- sweep fast 3D bodies so they cannot tunnel (default on)");
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
        AddGlobal("RemoveSceneFromProject", "",         "(index: int) -> bool");
        AddGlobal("ExitApp",             "",            "() -> void");
        AddGlobal("GetPlatformName",     "string",      "() -> string  -- iOS/Android/macOS/Windows/Linux");
        AddGlobal("SetPhysicsDebugFlags","",            "(flags: int) -> void");
        AddGlobal("SetSkyColour",        "",            "(rgb: Vec3) -> void");
        AddGlobal("Debug",               "table",       "-- in-game debug overlay: RegisterCommand/RegisterToggle/RegisterFloat/Run/Open/ShowPanel, frame explorer: SetPassCutoff/GetPassCutoff/StepPass/GetPassCount/GetPassName/GetPassState, physics explorer: SetPhysicsStepping/IsPhysicsStepping/StepPhysicsStage/StepPhysicsSubstep/GetPhysicsStage");
        AddGlobal("DebugLine",           "",            "(a: Vec3, b: Vec3, thickness: float, col: Vec4) -> void");
        AddGlobal("DebugPoint",          "",            "(p: Vec3, r: float, col: Vec4) -> void");
        AddGlobal("DebugRect",           "",            "(min: Vec2, max: Vec2, col: Vec4, z?: float, thickness?: float) -> void");
        AddGlobal("DebugBox",            "",            "(centre: Vec2, halfSize: Vec2, col: Vec4, z?: float, thickness?: float) -> void");
        AddGlobal("DebugCircle",         "",            "(centre: Vec2, r: float, col: Vec4, z?: float, thickness?: float) -> void");

        AddGlobal("GetEntityByName",     "Entity",      "(scene: Scene, name: string) -> Entity");
        AddGlobal("FindEntityByName",    "Entity",      "(scene: Scene, name: string) -> Entity");
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
        AddMethod("Scene", "Serialise",   "", "(folder: string) -> void  -- writes <folder><scene name>.lsn; physical path ending in /");
        AddMethod("Scene", "Deserialise", "", "(folder: string) -> void  -- replaces the scene's entities with the saved ones");
        AddGlobal("ImportGLTFScene",     "Entity", "(scene: Scene, path: string) -> Entity  -- a glTF as entities (nodes, models, lights, cameras) under one parent; invalid if it will not load");
        AddGlobal("AddMeshCollider",     "bool",   "(entity: Entity, lcolPath?: string, rebake?: bool) -> bool  -- static triangle-mesh collision from the entity's model. With a path: loaded from that .lcol, or built and written there if it is missing (or rebake)");
        AddGlobal("GetMeshColliderTriangles", "int", "(entity: Entity) -> int  -- 0 until the mesh collider has been built");
        AddGlobal("AddPhysicsCapsule",   "Entity", "(scene: Scene, name: string, pos: Vec3, radius: float, segment: float, inverseMass: float, colour: Vec4) -> Entity  -- upright, radius*2 + segment tall");
        AddGlobal("SetEntityPulse",      "",       "(e: Entity, albedo: Vec4, emissive: float) -> void");
        AddGlobal("SetEntityMaterial",   "",       "(e: Entity, albedo: Vec4, roughness: float, metallic: float) -> void");
        AddGlobal("SetEntityVisible",    "",       "(e: Entity, visible: bool) -> void  -- hidden meshes keep their body and shadow");
        AddGlobal("SphereCast",          "RaycastHit", "(o: Vec3, d: Vec3, radius: float, maxDist: float, ignore?: RigidBody3D) -> RaycastHit");
        AddGlobal("UIRect",              "",       "(id: string, x: float, y: float, w: float, h: float, colour: Vec4, radius?: float) -> void  -- free-standing, screen px");
        AddGlobal("UIRing",              "",       "(id: string, x: float, y: float, radius: float, colour: Vec4, thickness?: float) -> void");
        AddGlobal("SetModelEmissive",    "",       "(m: Model, colour: Vec4, intensity: float) -> void");
        AddGlobal("UISetClickOnRelease", "",       "(enable: bool) -> void  -- touch: clicks fire on release-inside w/ drag-slop cancel");
        AddGlobal("UIPushMonoText", "",       "() -> void  -- widgets built until UIPopMonoText use the monospace font");
        AddGlobal("UIPopMonoText", "",        "() -> void");
        AddGlobal("UISetScrollbarFade", "",        "(enable: bool) -> void  -- scroll bars show only while scrolling");
        AddGlobal("UISetLayeredBatching", "",      "(enable: bool) -> void  -- draw each panel's quads then its text (default on); off = strict tree order");
        AddGlobal("UISetScrollPhysics", "",        "(enable: bool) -> void  -- momentum + rubber band scrolling");
        AddGlobal("UIBeginScrollArea", "",         "(id: string, height: float) -> void  -- scrollable clipped region; height in px (DPI-scale it yourself)");
        AddGlobal("UIEndScrollArea", "",           "() -> void");
        AddGlobal("UIFocusNextTextInput", "",      "() -> void  -- next text input built claims focus (opens iOS keyboard)");
        AddGlobal("UIClickableRects", "table",     "() -> { {x, y, w, h}, ... }  -- clickable widgets on screen, in mouse coordinates");

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
        // Global/bus volume. Applies to sounds already playing.
        AddMethod("AudioMixer", "SetVolume",    "",      "(v: float) -> void");
        AddMethod("AudioMixer", "GetVolume",    "float", "() -> float");
        AddMethod("AudioMixer", "SetBusVolume", "",      "(bus: string, v: float) -> void");
        AddMethod("AudioMixer", "GetBusVolume", "float", "(bus: string) -> float");

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
        AddGlobal("AppendFile",             "bool",      "(path: string, content: string) -> bool");

        AddField("Light", "Intensity", "float");
        AddField("Light", "Radius",    "float");
        AddField("Light", "Colour",    "Vec4");
        AddField("Light", "Direction", "Vec3");
        AddField("Light", "Position",  "Vec3");
        AddField("Light", "Type",      "int");
        AddField("Light", "Angle",     "float");
        AddField("Light", "CastShadows",      "bool");
        AddField("Light", "ShadowResolution", "int");
        AddField("Light", "ShadowUpdate",     "int");
        AddField("Light", "ShadowBias",       "float");
        AddField("Light", "ShadowNormalBias", "float");
        AddField("Light", "ShadowNear",       "float");

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
        AddMethod("ModelComponent", "GetAnimationCount",    "int",    "() -> int");
        AddMethod("ModelComponent", "GetAnimationName",     "string", "(i: int) -> string  -- 0-based");
        AddMethod("ModelComponent", "HasAnimation",         "bool",   "(name: string) -> bool");
        AddMethod("ModelComponent", "PlayAnimation",        "bool",   "(name: string, fade?: float, loop?: bool, speed?: float, restart?: bool) -> bool  -- fades every other clip out");
        AddMethod("ModelComponent", "SetAnimationWeight",   "",       "(name: string, weight: float, loop?: bool) -> void  -- blend directly; 0 removes it");
        AddMethod("ModelComponent", "FadeAnimation",        "",       "(name: string, weight: float, seconds: float) -> void");
        AddMethod("ModelComponent", "SetAnimationSpeed",    "",       "(name: string, speed: float) -> void");
        AddMethod("ModelComponent", "SetAnimationTime",     "",       "(name: string, seconds: float) -> void");
        AddMethod("ModelComponent", "GetAnimationTime",     "float",  "(name: string) -> float  -- -1 when not playing");
        AddMethod("ModelComponent", "GetAnimationWeight",   "float",  "(name: string) -> float");
        AddMethod("ModelComponent", "GetAnimationDuration", "float",  "(name: string) -> float");
        AddMethod("ModelComponent", "IsAnimationFinished",  "bool",   "(name: string) -> bool  -- a non-looping clip at its end");
        AddMethod("ModelComponent", "SetRootMotionBone",    "bool",   "(bone: string, lockRotation?: bool) -> bool  -- that bone's horizontal travel (and turning) leaves the pose; read it with ConsumeRootMotion / ConsumeRootRotation");
        AddMethod("ModelComponent", "ConsumeRootRotation",  "float",  "() -> float  -- degrees the root turned since the last call (lockRotation)");
        AddMethod("ModelComponent", "ConsumeRootMotion",    "Vec3",   "() -> Vec3  -- root-motion travel since the last call, in the model's space");
        AddGlobal("GetBoneWorldPosition", "Vec3|nil", "(entity: Entity, bone: string) -> Vec3|nil");
        AddGlobal("GetBoneWorldRotation", "Quat|nil", "(entity: Entity, bone: string) -> Quat|nil");

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
        AddMethod("Sprite", "SetPosition",           "", "(p: Vec2) -> void");
        AddMethod("Sprite", "SetScale",              "", "(s: Vec2) -> void");
        AddMethod("Sprite", "SetColour",             "", "(c: Vec4) -> void");
        AddConstructor("AnimatedSprite");
        AddMethod("AnimatedSprite", "SetTexture",          "", "(t: Texture2D) -> void");
        AddMethod("AnimatedSprite", "SetTextureFromFile",  "", "(path: string) -> void");
        AddMethod("AnimatedSprite", "SetSpriteSheet",      "", "(index: Vec2, cellSize: Vec2, spriteSize: Vec2, border?: float) -> void");
        AddMethod("AnimatedSprite", "SetSpriteSheetIndex", "", "(x: int, y: int) -> void");
        AddMethod("AnimatedSprite", "SetState",            "", "(name: string) -> void");
        AddMethod("AnimatedSprite", "GetState",            "string", "() -> string");
        AddMethod("AnimatedSprite", "SetPosition",         "", "(p: Vec2) -> void");
        AddMethod("AnimatedSprite", "SetScale",            "", "(s: Vec2) -> void");
        AddMethod("AnimatedSprite", "SetColour",           "", "(c: Vec4) -> void");
        AddMethod("AnimatedSprite", "AddState",            "", "(name: string, frames: Vec2[], frameDuration: float, pingPong?: bool) -> void");
        AddMethod("AnimatedSprite", "AddGridState",        "", "(name: string, cellSize: Vec2, row: int, firstColumn: int, frameCount: int, frameDuration: float, pingPong?: bool) -> void");
        AddMethod("AnimatedSprite", "SetPlayMode",         "", "(name: string, mode: int) -> void  -- 0 loop, 1 ping-pong");
        AddMethod("AnimatedSprite", "SetFrameDuration",   "", "(name: string, seconds: float) -> void");
        AddField("AnimatedSprite", "FrameSize", "Vec2");
        AddField("AnimatedSprite", "ReceivesLight", "bool");

        // Particles.
        AddConstructor("ParticleEmitter", "(count?: int) -> ParticleEmitter");
        for(const char* setter : {
            "SetParticleCount","SetParticleLife","SetParticleSize","SetInitialVelocity",
            "SetInitialColour","SetSpread","SetVelocitySpread","SetGravity",
            "SetNextParticleTime","SetParticleRate","SetNumLaunchParticles","SetIsAnimated",
            "SetAnimatedTextureRows","SetSortParticles","SetBlendType","SetFadeIn",
            "SetFadeOut","SetLifeSpread","SetAlignedType","SetDepthWrite",
            "SetLocalSpace","SetSpawnOffset","SetTextureFromFile" })
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
        AddGlobal("UILabelWrapped", "", "(name: string, text: string, maxWidth: float) -> void");
        AddGlobal("UILabelEllipsis","", "(name: string, text: string, maxWidth: float) -> void");
        AddGlobal("UILabelLink",    "UI_Interaction", "(name: string, text: string, maxWidth: float) -> UI_Interaction  -- clickable label, looks like plain text");
        AddGlobal("UIButton",       "UI_Interaction", "(text: string) -> UI_Interaction");
        AddGlobal("UIImage",        "UI_Interaction", "(name: string, t: Texture2D, size: Vec2) -> UI_Interaction");
        AddGlobal("UISeparator",    "", "(w?: float) -> void");
        AddGlobal("UISpacer",       "", "(h?: float) -> void");
        AddGlobal("UISlider",       "float", "(label: string, value: float, min?: float, max?: float) -> float");
        AddGlobal("UIToggle",       "bool",  "(label: string, value: bool) -> bool");
        AddGlobal("UIProgressBar",  "bool",  "(label: string, p: float, w?: float, h?: float) -> bool");
        AddGlobal("UIArrow",        "bool",  "(dir: int) -> bool");
        AddGlobal("UIBeginRadar",   "UI_Interaction", "(id: string, diameter: float, bg: Vec4, ring: Vec4) -> UI_Interaction");
        AddGlobal("UIRadarBlip",    "UI_Interaction", "(id: string, x: float, y: float, size: float, tint: Vec4, texture?: Texture2D) -> UI_Interaction");
        AddGlobal("UIRadarRing",    "void", "(id: string, radius: float, colour: Vec4, thickness?: float)");
        AddGlobal("UIEndRadar",     "", "() -> void");
        AddGlobal("UILayoutRoot",   "", "() -> void");
        AddGlobal("UIPushStyle",    "", "(v: StyleVar, val: float|Vec2|Vec3|Vec4) -> void");
        AddGlobal("UISetNextFlags", "", "(flags: WidgetFlags) -> void");
        AddGlobal("UIPopStyle",     "", "() -> void");
        AddGlobal("GetStringSize",  "Vec2", "(text: string) -> Vec2");

        AddField("UI_Interaction", "widget",   "UI_Widget");
        AddField("UI_Interaction", "hovering", "bool");
        AddField("UI_Interaction", "clicked",  "bool");
        AddField("UI_Interaction", "pressed",  "bool");
        AddField("UI_Interaction", "dragging", "bool");

        // FileDialog table.
        AddMethod("FileDialog", "OpenFile",   "string", "(filter?: string, defaultPath?: string) -> string");
        AddMethod("FileDialog", "SaveFile",   "string", "(filter?: string, defaultPath?: string, defaultName?: string) -> string");
        AddMethod("FileDialog", "PickFolder", "string", "(defaultPath?: string) -> string");
        AddMethod("FileDialog", "ImportFiles", "table", "(extensions: string, destDir: string) -> table (iOS only)");
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
        for(const char* s : { "Pixels","TextContent","PercentOfParent","ChildSum","MaxChild","PercentOfViewport","Grow" })
            AddEnum("SizeKind", s);
        for(const char* s : { "Padding","Border","BorderColor","BackgroundColor","TextColor",
                              "HotBorderColor","HotBackgroundColor","HotTextColor",
                              "ActiveBorderColor","ActiveBackgroundColor","ActiveTextColor",
                              "FontSize","Count" })
            AddEnum("StyleVar", s);
        for(const char* s : { "Clickable","DrawText","DrawBorder","DrawBackground","Draggable",
                              "StackVertically","StackHorizontally","Floating_X","Floating_Y",
                              "CentreX","CentreY","CentreChildrenX","CentreChildrenY","DragParent",
                              "AnimateScale","AnimateAppear","AlignRight","AnimateExit" })
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
        m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string, sol::lib::coroutine);

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
            "FindEntityByName",
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
        // no ImGui context in the editor: the bindings would crash, so they are not there (scripts see nil)
        if(Application::IsImGuiEnabled())
            BindImGuiLua(*m_State);
        BindECSLua(*m_State);
        BindLogLua(*m_State);
        BindSceneLua(*m_State);
        BindPhysicsLua(*m_State);
        BindUILua(*m_State);
        BindAppLua(*m_State);
        BindVoxelLua(*m_State);
        BindCSVLua(*m_State);
        BindMeshLua(*m_State);
        BindAudioLua(*m_State);
        BindSystemLua(*m_State);
        BindWidgetsLua(*m_State);
        TestRunner::Get().BindLua(*m_State);

        //GenerateLuaStubs(*m_State, "/Users/jmorton/Dev/Lumos-Dev/Lumos/Source/Lumos/Scripting/Lua/LuaStubs.lua");

        LINFO("Initialised Lua Manager");
    }

    LuaManager::~LuaManager()
    {
        // Debug commands can hold Lua callbacks - drop them before the state goes.
        DebugMenu::Get().UnregisterScriptCommands();
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
        m_State->set_function("ScriptEnv", [this, scene](sol::optional<int> index) -> sol::object
        {
            auto& reg   = scene->GetRegistry();
            auto scripts = reg.view<LuaScriptComponent>();
            int want    = index.value_or(0);
            int i       = 0;
            for(auto e : scripts)
            {
                auto& sc = reg.get<LuaScriptComponent>(e);
                if(i++ != want)
                    continue;
                if(!sc.Loaded())
                    break;
                return sol::object(sc.GetSolEnvironment());
            }
            LWARN("ScriptEnv(%d): no loaded script at that index", want);
            return sol::lua_nil;
        });
    }

    void LuaManager::OnBackgroundUpdate(Scene* scene, float dt)
    {
        if(!scene)
            return;
        auto& registry = scene->GetRegistry();
        for(auto entity : registry.view<LuaScriptComponent>())
            registry.get<LuaScriptComponent>(entity).OnBackgroundUpdate(dt);
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

        std::string scripts = std::string((const char*)ScriptsPath.str);
        std::string package_path;
        package_path += scripts + "/?.lua;";
        package_path += scripts + "/lua/?.lua;";

        std::string currentPaths = state["package"]["path"];
        state["package"]["path"] = package_path + currentPaths;

        ScratchEnd(Scratch);

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
                            TestRunner::ReportScriptError(vfsPath.c_str(), e.what());
                            return sol::make_object(l, sol::lua_nil);
                        }
                        sol::protected_function fn = lr;
                        sol::protected_function_result r = fn();
                        if(!r.valid())
                        {
                            sol::error e = r;
                            LERROR("require('%s') run failed: %s", vfsPath.c_str(), e.what());
                            TestRunner::ReportScriptError(vfsPath.c_str(), e.what());
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

    // No warning on a miss: for callers that are only asking whether it exists.
    static Entity FindEntityByName(Scene* scene, const std::string& name)
    {
        entt::entity e = entt::null;
        scene->GetRegistry().view<NameComponent>().each([&](const entt::entity& entity, const NameComponent& component)
                                                        {
                                                            if(name == component.name)
                                                                e = entity; });
        return { e, scene };
    }

    Entity GetEntityByName(Scene* scene, const std::string& name)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!scene) // asked before a scene has loaded
            return {};
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

    struct LuaCSV
    {
        Arena* arena = nullptr;
        CSV csv      = {};
        ~LuaCSV()
        {
            if(arena)
                ArenaRelease(arena);
        }
    };

    void LuaManager::RunScriptFile(const std::string& vfsPath)
    {
        if(vfsPath.empty())
            return;

        ArenaTemp scratch = ScratchBegin(0, 0);
        String8 path      = PushStr8Copy(scratch.arena, vfsPath.c_str());
        String8 code      = FileSystem::Get().ReadTextFileVFS(scratch.arena, path);
        if(code.size == 0)
        {
            LWARN("[Lua] Script not found: %s", vfsPath.c_str());
            ScratchEnd(scratch);
            return;
        }

        auto result = m_State->safe_script(std::string_view((const char*)code.str, code.size), sol::script_pass_on_error);
        if(!result.valid())
        {
            sol::error err = result;
            LERROR("[Lua] Script error in '%s': %s", vfsPath.c_str(), err.what());
            TestRunner::ReportScriptError(vfsPath.c_str(), err.what());
        }
        ScratchEnd(scratch);
    }

    void LuaManager::BindCSVLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();

        // 0-based row/column indices (matches data-processing loops).
        state.new_usertype<LuaCSV>(
            "CSVData",
            "RowCount", [](LuaCSV& c)
            { return (int)c.csv.rowCount; },
            "ColumnCount", [](LuaCSV& c)
            { return (int)c.csv.columnCount; },
            "Column", [](LuaCSV& c, std::string_view name)
            { return CSVColumn(c.csv, Str8((uint8_t*)name.data(), name.size())); },
            "Get", [](LuaCSV& c, int row, int col)
            { String8 f = CSVGet(c.csv, (uint32_t)row, (uint32_t)col); return std::string((const char*)f.str, f.size); },
            "Number", [](LuaCSV& c, int row, int col)
            { return CSVF64(c.csv, (uint32_t)row, (uint32_t)col); },
            "Int", [](LuaCSV& c, int row, int col)
            { return (int)CSVI64(c.csv, (uint32_t)row, (uint32_t)col); });

        auto csv = state.create_table("CSV");
        csv.set_function("Load", [](std::string_view path) -> std::shared_ptr<LuaCSV>
                         {
            auto wrap   = std::make_shared<LuaCSV>();
            wrap->arena = ArenaAlloc(256 * 1024 * 1024);
            // Cached: parses once, writes a .lcsv sidecar, later boots bulk-load it.
            wrap->csv   = CSVParseFileCached(wrap->arena, Str8((uint8_t*)path.data(), path.size()), true);
            return wrap; });

        auto pc = state.create_table("PointCloud");
        // Star appearance, all in one place. Sizes are multiples of the caller's
        // baseSize, which is the size of a magnitude 5 star; brightness is a
        // compressed magnitude law (see below). Tuned against a naked-eye sky
        // reference: a few big saturated discs over a field of small dim dots.
        constexpr float SIZE_AT_MAG0   = 2.30f; // size multiplier at magnitude 0
        constexpr float SIZE_PER_MAG   = 0.22f; // ...shrinking by this per magnitude
        constexpr float SIZE_MIN_MULT  = 0.72f;
        constexpr float SIZE_MAX_MULT  = 3.40f;
        constexpr float BRIGHT_K       = 0.53f; // 1 = the true 10^(-0.4 mag) law
        constexpr float BRIGHT_REF_MAG = 4.4f;  // magnitude that renders at peak 1.0
        constexpr float COLOUR_SAT     = 3.6f;  // blackbody saturation boost
        // magLimit (optional): skip rows fainter than this magnitude. A full
        // star catalogue is mostly things too faint to be worth a quad, and at
        // sky densities they pile into mush rather than reading as more stars.
        pc.set_function("FromCSV", [](LuaCSV& c, int xCol, int yCol, int zCol, double posScale, double baseSize, int magCol, int ciCol, sol::optional<double> magLimit) -> int
                        {
            uint32_t rows = c.csv.rowCount;
            if(rows == 0 || xCol < 0 || yCol < 0 || zCol < 0)
                return 0;

            // Dedicated arena - too big for scratch (rows * 32B).
            Arena* arena = ArenaAlloc(rows * (sizeof(Graphics::PointCloudVertex) + sizeof(uint32_t)) + 1024);
            Graphics::PointCloudVertex* pts = PushArrayNoZero(arena, Graphics::PointCloudVertex, rows);
            // magLimit skips rows, so the Nth point is NOT the Nth row. Keep the
            // row each point came from or every pick would name the wrong star.
            uint32_t* srcRows = PushArrayNoZero(arena, uint32_t, rows);
            uint32_t n = 0;
            for(uint32_t r = 0; r < rows; r++)
            {
                float x = (float)(CSVF64(c.csv, r, (uint32_t)xCol) * posScale);
                float y = (float)(CSVF64(c.csv, r, (uint32_t)yCol) * posScale);
                float z = (float)(CSVF64(c.csv, r, (uint32_t)zCol) * posScale);

                float mag = magCol >= 0 ? (float)CSVF64(c.csv, r, (uint32_t)magCol) : 4.0f;

                if(magCol >= 0 && magLimit && mag > (float)magLimit.value())
                    continue;

                // Size in pixels. baseSize is the size of a MAGNITUDE 5 star -
                // the ordinary ones - and brighter stars grow from there. What
                // sells a sky is the spread: a handful of big bright discs over a
                // field of small dots, not one uniform dot size (the old law only
                // spanned 1.8x once the shader's floor was applied, so every star
                // came out the same).
                float size = (float)baseSize;
                if(magCol >= 0)
                {
                    float t = SIZE_AT_MAG0 - SIZE_PER_MAG * mag;
                    t       = t < SIZE_MIN_MULT ? SIZE_MIN_MULT : (t > SIZE_MAX_MULT ? SIZE_MAX_MULT : t);
                    size    = (float)baseSize * t;
                }

                Vec4 colour(1.0f, 1.0f, 1.0f, 1.0f);
                if(ciCol >= 0)
                {
                    // Colour index (B-V) -> temperature (Ballesteros) -> blackbody.
                    float bv = (float)CSVF64(c.csv, r, (uint32_t)ciCol);
                    bv       = bv < -0.4f ? -0.4f : (bv > 2.0f ? 2.0f : bv);
                    float kelvin = 4600.0f * (1.0f / (0.92f * bv + 1.7f) + 1.0f / (0.92f * bv + 0.62f));
                    colour       = Colour::BlackbodyColour(kelvin);

                    // Push it away from grey, then normalise so the largest
                    // channel is 1: the shader wants a pure HUE with brightness
                    // as a separate scalar. Blackbody colours are nearly white to
                    // begin with, and a star field that keeps brightness in the
                    // rgb ends up saturating every channel and rendering white.
                    float lum = 0.2126f * colour.x + 0.7152f * colour.y + 0.0722f * colour.z;
                    colour.x  = lum + (colour.x - lum) * COLOUR_SAT;
                    colour.y  = lum + (colour.y - lum) * COLOUR_SAT;
                    colour.z  = lum + (colour.z - lum) * COLOUR_SAT;
                    colour.x  = colour.x < 0.0f ? 0.0f : colour.x;
                    colour.y  = colour.y < 0.0f ? 0.0f : colour.y;
                    colour.z  = colour.z < 0.0f ? 0.0f : colour.z;
                    float mx  = colour.x > colour.y ? colour.x : colour.y;
                    mx        = mx > colour.z ? mx : colour.z;
                    if(mx > 0.0001f)
                    {
                        colour.x /= mx;
                        colour.y /= mx;
                        colour.z /= mx;
                    }
                }

                if(magCol >= 0)
                {
                    // Peak brightness, linear. The real law is 10^(-0.4*mag),
                    // which over the catalogue is 2500:1 and unrenderable; BRIGHT_K
                    // compresses it to about 60:1, which fits between "clips to a
                    // white core and blooms" and "still has visible colour".
                    colour.w = std::pow(10.0f, -0.4f * BRIGHT_K * (mag - BRIGHT_REF_MAG));
                }

                pts[n].PosSize = Vec4(x, y, z, size);
                pts[n].Colour  = colour;
                srcRows[n]     = r;
                n++;
            }

            auto cloud = CreateSharedPtr<Graphics::PointCloud>();
            cloud->SetPoints(pts, n, srcRows);
            Graphics::RegisterPointCloud(cloud);
            ArenaRelease(arena);
            return (int)n; });

        pc.set_function("Clear", []()
                        { Graphics::ClearPointClouds(); });

        pc.set_function("SetSkyFade", [](float f)
                        { Graphics::SetPointCloudSkyFade(f); });

        pc.set_function("SetStreak", [](const Vec3& dir, float amount)
                        { Graphics::SetPointCloudStreak(dir, amount); });

        pc.set_function("SetLook", [](float spike, float spikeLen, float sigmaFloor)
                        { Graphics::SetPointCloudLook(spike, spikeLen, sigmaFloor); });

        // Hide one star's sprite (source row as in FromCSV); -1 = none.
        pc.set_function("SetHiddenRow", [](int row)
                        { Graphics::SetPointCloudHiddenSource(row); });

        // Returns the CSV ROW of the star under (nx, ny) in 0..1 screen space, or
        // -1. Row, not point index: FromCSV filters, so the two differ (see
        // PointCloud::SetPoints).
        pc.set_function("PickScreen", [](float nx, float ny) -> int
                        {
            Scene* scene = Application::Get().GetCurrentScene();
            if(!scene)
                return -1;
            auto& reg = scene->GetRegistry();
            auto view = reg.view<Camera>();
            if(view.begin() == view.end())
                return -1;

            entt::entity ce       = view.front();
            Camera* cam           = &reg.get<Camera>(ce);
            Maths::Transform* tr  = reg.try_get<Maths::Transform>(ce);
            if(!cam || !tr)
                return -1;

            Vec3 fwd        = tr->GetForwardDirection();
            Vec3 right      = tr->GetRightDirection();
            Vec3 up         = tr->GetUpDirection();
            float halfFov   = 0.5f * cam->GetFOV() * 3.14159265358979323846f / 180.0f;
            float tanHalf   = tanf(halfFov);
            float aspect    = cam->GetAspectRatio();
            float ndcX      = 2.0f * nx - 1.0f;
            float ndcY      = 1.0f - 2.0f * ny; // screen y is top-down; NDC up is +y
            Maths::Ray ray;
            ray.Origin      = tr->GetWorldPosition();
            ray.Direction   = (fwd + right * (ndcX * tanHalf * aspect) + up * (ndcY * tanHalf)).Normalised();

            int best     = -1;
            float bestTan = 1e9f;
            for(auto& cloud : Graphics::GetActivePointClouds())
            {
                if(!cloud)
                    continue;
                float tan = 0.0f;
                // distWeight biases toward closer stars when several cluster near the ray.
                int idx   = cloud->PickRay(ray.Origin, ray.Direction, 0.03f, &tan, 0.4f);
                if(idx >= 0 && tan < bestTan)
                {
                    bestTan = tan;
                    best    = idx;
                }
            }
            return best; });

        pc.set_function("NearestPoint", [](const Vec3& pos) -> int
                        {
            int best        = -1;
            float bestDist  = 1e30f;
            for(auto& cloud : Graphics::GetActivePointClouds())
            {
                if(!cloud)
                    continue;
                float dist = 0.0f;
                int idx    = cloud->NearestPoint(pos, &dist);
                if(idx >= 0 && dist < bestDist)
                {
                    bestDist = dist;
                    best     = idx;
                }
            }
            return best; });

        auto lc = state.create_table("LineCloud");
        static SharedPtr<Graphics::LineCloud> s_buildingLineCloud;

        lc.set_function("Begin", []()
                        { s_buildingLineCloud = CreateSharedPtr<Graphics::LineCloud>(); });

        lc.set_function("AddLine", [](const Vec3& a, const Vec3& b, const Vec4& colour, double width,
                                      sol::optional<double> dashPeriod, sol::optional<double> duty, sol::optional<double> arcAtA)
                        {
            if(s_buildingLineCloud)
                s_buildingLineCloud->AddLine(a, b, colour, (float)width,
                                             (float)dashPeriod.value_or(0.0),
                                             (float)duty.value_or(0.5),
                                             (float)arcAtA.value_or(0.0)); });

        lc.set_function("End", []()
                        {
            if(s_buildingLineCloud)
            {
                s_buildingLineCloud->Finish();
                Graphics::RegisterLineCloud(s_buildingLineCloud);
                s_buildingLineCloud = nullptr;
            } });

        lc.set_function("Clear", []()
                        { Graphics::ClearLineClouds(); });

        lc.set_function("SetLocal", [](bool local)
                        {
            if(s_buildingLineCloud)
                s_buildingLineCloud->SetLocal(local); });

        lc.set_function("Draw", [](const Vec3& a, const Vec3& b, const Vec4& colour, double width,
                                   sol::optional<bool> local, sol::optional<double> widthB,
                                   sol::optional<double> dashPeriod, sol::optional<double> duty,
                                   sol::optional<double> arcAtA)
                        { Graphics::DrawImmediateLine(a, b, colour, (float)width, local.value_or(false),
                                                      (float)dashPeriod.value_or(0.0), (float)duty.value_or(0.5),
                                                      (float)widthB.value_or(0.0), (float)arcAtA.value_or(0.0)); });

        auto wt = state.create_table("WorldText");
        wt.set_function("Add", [](const Vec3& pos, const std::string& text, double size, const Vec4& colour, sol::optional<bool> local, sol::optional<Vec4> background)
                        { Graphics::AddWorldLabel(pos, text.c_str(), (float)size, colour, local.value_or(false), background.value_or(Vec4(0.0f))); });
        wt.set_function("Clear", []()
                        { Graphics::ClearWorldLabels(); });
        wt["MultiLine"] = true; // "\n" in the text is honoured

        auto pl = state.create_table("Planet");
        pl.set_function("Add", [](const Vec3& pos, double radius, const Vec3& starPos, int type,
                                  const Vec4& colour, double seed, sol::optional<double> cloud, sol::optional<double> spin,
                                  sol::optional<double> tilt, sol::optional<bool> rings)
                        {
            Graphics::AddPlanet(pos, (float)radius, starPos, type, colour, (float)seed,
                                (float)cloud.value_or(0.5), (float)spin.value_or(0.1),
                                (float)tilt.value_or(0.0), rings.value_or(false)); });
        pl.set_function("Clear", []()
                        { Graphics::ClearPlanets(); });
        pl["Terrestrial"] = 0;
        pl["Gas"]         = 1;
        pl["Procedural"]  = 2;
        pl["Star"]        = 3;
        pl["Rocky"]       = 4; // airless cratered body (moons, asteroids)

        auto sv = state.create_table("SystemView");
        sv.set_function("Set", [](const Vec3& camLocal)
                        { Graphics::SetSystemFrame(true, camLocal); });
        sv.set_function("Clear", []()
                        { Graphics::SetSystemFrame(false, Vec3(0.0f)); });
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

        // game-view space: in the editor the game draws into the Game tab, not the whole window
        input.set_function("GetMousePosition", []() -> Vec2
                           {
                               Vec2 pos = Input::Get().GetMousePosition();
                               Window* w = Application::Get().GetWindow();
                               return (w ? pos * w->GetMousePosScale() : pos) - Application::Get().GetGameViewPosition();
                           });

        input.set_function("GetScrollOffset", []() -> float
                           { return Input::Get().Get().GetScrollOffset(); });
        input.set_function("GetScrollOffsetX", []() -> float
                           { return Input::Get().GetScrollOffsetX(); });

        // Injected pointer state - what a gamepad's virtual cursor drives. The
        // UI hit-tests against Input's stored position, so setting it here (and
        // the click flags below) makes every existing panel controller-usable
        // without a focus-navigation system. Undoes GetMousePosition's scale so
        // the two are symmetric.
        input.set_function("SetMousePosition", [](float x, float y)
                           {
                               Window* w = Application::Get().GetWindow();
                               const float s = w ? w->GetMousePosScale() : 1.0f;
                               const float inv = (s != 0.0f) ? 1.0f / s : 1.0f;
                               Vec2 view       = Application::Get().GetGameViewPosition();
                               Input::Get().StoreMousePosition((x + view.x) * inv, (y + view.y) * inv);
                           });

        input.set_function("SetMouseClicked", [](Lumos::InputCode::MouseKey key, bool down)
                           { Input::Get().SetMouseClicked(key, down); });

        input.set_function("SetMouseHeld", [](Lumos::InputCode::MouseKey key, bool down)
                           { Input::Get().SetMouseHeld(key, down); });

        input.set_function("SetKeyPressed", [](Lumos::InputCode::Key key, bool down)
                           { Input::Get().SetKeyPressed(key, down); });

        input.set_function("SetKeyHeld", [](Lumos::InputCode::Key key, bool down)
                           { Input::Get().SetKeyHeld(key, down); });

        input.set_function("SetScrollOffset", [](float offset)
                           { Input::Get().SetScrollOffset(offset); });

        input.set_function("SetScrollOffsetX", [](float offset)
                           { Input::Get().SetScrollOffsetX(offset); });

        input.set_function("SetTouchCount", [](int count)
                           { Input::Get().SetTouchCount(count > 0 ? (uint32_t)count : 0u); });

        input.set_function("SetMouseOnScreen", [](bool onScreen)
                           { Input::Get().SetMouseOnScreen(onScreen); });

        // "selection" (default), "light", "medium", "heavy". Does nothing where there's no haptic engine.
        input.set_function("Haptic", [](sol::optional<std::string> kind)
                           {
                               int k = 0;
                               if(kind)
                                   k = *kind == "light" ? 1 : (*kind == "medium" ? 2 : (*kind == "heavy" ? 3 : 0));
                               Lumos::OS::Get().Haptic(k);
                           });

        input.set_function("SetMouseCaptured", [](bool captured)
                           {
                               Window* w = Application::Get().GetWindow();
                               if(w) w->HideMouse(captured);
                               Input::Get().SetMouseMode(captured ? MouseMode::Captured : MouseMode::Visible);
                               // capturing can move the cursor; that jump is not the player's hand
                               Input::Get().ResetMouseDelta();
                           });
        input.set_function("GetMouseDelta", []() -> Vec2
                           { return Input::Get().GetMouseDelta(); });

        input.set_function("GetControllerAxis", [](int id, int axis) -> float
                           { return Input::Get().GetControllerAxis(id, axis); });

        input.set_function("GetControllerName", [](int id) -> std::string
                           { return Input::Get().GetControllerName(id); });

        input.set_function("GetControllerHat", [](int id, int hat) -> int
                           { return Input::Get().GetControllerHat(id, hat); });

        input.set_function("IsControllerButtonPressed", [](int id, int button) -> bool
                           { return Input::Get().IsControllerButtonPressed(id, button); });

        // Gamepads: standardised layout, so one set of bindings covers a
        // DualSense, an Xbox pad and a Steam Deck. No id argument - these all
        // read the first connected gamepad.
        input.set_function("GamepadPresent", []() -> bool
                           { return Input::Get().IsGamepadPresent(); });

        input.set_function("GamepadName", []() -> std::string
                           { return Input::Get().GetGamepadName(); });

        input.set_function("GamepadAxis", [](Lumos::InputCode::GamepadAxis axis) -> float
                           { return Input::Get().GetGamepadAxis(axis); });

        input.set_function("GamepadHeld", [](Lumos::InputCode::GamepadButton button) -> bool
                           { return Input::Get().GetGamepadButtonHeld(button); });

        input.set_function("GamepadPressed", [](Lumos::InputCode::GamepadButton button) -> bool
                           { return Input::Get().GetGamepadButtonPressed(button); });

        input.set_function("SetGamepadDeadZone", [](float dz)
                           { Input::Get().SetGamepadDeadZone(dz); });

        input.set_function("AddGamepadMappings", [](const std::string& mappings) -> bool
                           { return Input::Get().AddGamepadMappings(mappings.c_str()); });

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
        input.set_function("GetTouchCount", []() -> uint32_t
                           { return Input::Get().GetTouchCount(); });
        // Fake fingers for scripted tests on desktop: same shape as GetTouches
        input.set_function("SetTouches", [](sol::table list)
                           {
            Input::TouchPoint pts[Input::MaxTouchPoints];
            uint32_t n = 0;
            for(auto& kv : list)
            {
                if(n >= Input::MaxTouchPoints || !kv.second.is<sol::table>())
                    continue;
                sol::table t   = kv.second.as<sol::table>();
                pts[n].Position = Vec2(t.get_or("x", 0.0f), t.get_or("y", 0.0f));
                pts[n].Id       = (uint64_t)t.get_or("id", (double)(n + 1));
                n++;
            }
            Input::Get().SetTouchPoints(pts, n);
            Input::Get().SetTouchCount(n); });
        // { {x=, y=, id=}, ... } for every finger down, in GetMousePosition space
        input.set_function("GetTouches", [](sol::this_state ts) -> sol::table
                           {
            sol::state_view lua(ts);
            sol::table out = lua.create_table();
            const Input& in = Input::Get();
            for(uint32_t i = 0; i < in.GetTouchPointCount(); i++)
            {
                const auto& t = in.GetTouchPoint(i);
                sol::table e  = lua.create_table();
                Vec2 view     = Application::Get().GetGameViewPosition();
                e["x"]  = t.Position.x - view.x;
                e["y"]  = t.Position.y - view.y;
                e["id"] = (double)(t.Id & 0xFFFFFFFFFFFFull);
                out[i + 1] = e;
            }
            return out; });

        input.set_function("GetScreenSize", []() -> Vec2
                           { return Application::Get().GetGameViewSize(); });

        input.set_function("GetDPIScale", []() -> float
                           { Window* w = Application::Get().GetWindow(); return w ? w->GetDPIScale() : 1.0f; });

        // What to SIZE the UI by. GetDPIScale is the coordinate conversion
        // (framebuffer px per window unit) and is 1.0 on X11 whatever the panel;
        // this is the desktop's own scaling factor, so a 200% 4K display reports 2
        // here and 1 there. Identical on macOS/iOS.
        input.set_function("GetContentScale", []() -> float
                           { Window* w = Application::Get().GetWindow(); return w ? w->GetContentScale() : 1.0f; });

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
            { "LeftAlt", Lumos::InputCode::Key::LeftAlt },
            { "LeftSuper", Lumos::InputCode::Key::LeftSuper },
            { "RightAlt", Lumos::InputCode::Key::RightAlt },
            { "RightSuper", Lumos::InputCode::Key::RightSuper },
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
            { "KP_EQUAL", Lumos::InputCode::Key::KPEqual },
            { "Equal", Lumos::InputCode::Key::Equal },
            { "Minus", Lumos::InputCode::Key::Minus }
        };
        state.new_enum<Lumos::InputCode::Key, false>("Key", keyItems); // false makes it read/write in Lua, but its faster

        std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::MouseKey>> mouseItems = {
            { "Left", Lumos::InputCode::MouseKey::ButtonLeft },
            { "Right", Lumos::InputCode::MouseKey::ButtonRight },
            { "Middle", Lumos::InputCode::MouseKey::ButtonMiddle },
        };
        state.new_enum<Lumos::InputCode::MouseKey, false>("MouseButton", mouseItems);

        // Face buttons are named by POSITION (South/East/West/North), not by
        // the glyph on the pad - South is Cross on PlayStation, A on Xbox.
        std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::GamepadButton>> padItems = {
            { "South", Lumos::InputCode::GamepadSouth },
            { "East", Lumos::InputCode::GamepadEast },
            { "West", Lumos::InputCode::GamepadWest },
            { "North", Lumos::InputCode::GamepadNorth },
            { "LeftBumper", Lumos::InputCode::GamepadLeftBumper },
            { "RightBumper", Lumos::InputCode::GamepadRightBumper },
            { "Back", Lumos::InputCode::GamepadBack },
            { "Start", Lumos::InputCode::GamepadStart },
            { "Guide", Lumos::InputCode::GamepadGuide },
            { "LeftThumb", Lumos::InputCode::GamepadLeftThumb },
            { "RightThumb", Lumos::InputCode::GamepadRightThumb },
            { "DPadUp", Lumos::InputCode::GamepadDPadUp },
            { "DPadRight", Lumos::InputCode::GamepadDPadRight },
            { "DPadDown", Lumos::InputCode::GamepadDPadDown },
            { "DPadLeft", Lumos::InputCode::GamepadDPadLeft },
        };
        state.new_enum<Lumos::InputCode::GamepadButton, false>("GamepadButton", padItems);

        std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::GamepadAxis>> padAxes = {
            { "LeftX", Lumos::InputCode::GamepadAxisLeftX },
            { "LeftY", Lumos::InputCode::GamepadAxisLeftY },
            { "RightX", Lumos::InputCode::GamepadAxisRightX },
            { "RightY", Lumos::InputCode::GamepadAxisRightY },
            { "LeftTrigger", Lumos::InputCode::GamepadAxisLeftTrigger },
            { "RightTrigger", Lumos::InputCode::GamepadAxisRightTrigger },
        };
        state.new_enum<Lumos::InputCode::GamepadAxis, false>("GamepadAxis", padAxes);
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
        // Both overloads: em:Create("name") used to bind to the no-arg version and
        // silently drop the name, leaving every scripted entity unnamed.
        entityManagerType.set_function("Create", sol::overload(
                                                     static_cast<Entity (EntityManager::*)()>(&EntityManager::Create),
                                                     static_cast<Entity (EntityManager::*)(const std::string&)>(&EntityManager::Create)));
        entityManagerType.set_function("GetRegistry", &EntityManager::GetRegistry);

        entityType.set_function("Valid", &Entity::Valid);
        entityType.set_function("Destroy", &Entity::Destroy);
        entityType.set_function("SetParent", &Entity::SetParent);
        entityType.set_function("GetParent", &Entity::GetParent);
        entityType.set_function("IsParent", &Entity::IsParent);
        entityType.set_function("GetChildren", [](Entity& e, sol::this_state ts) -> sol::table
                                {
            sol::state_view lua(ts);
            sol::table out    = lua.create_table();
            Entity* children  = e.GetChildrenTemp(); // counts them as it goes
            const u32 count   = children ? e.GetChildCount() : 0;
            for(u32 i = 0; i < count; i++)
                out[i + 1] = children[i];
            return out; });
        entityType.set_function("SetActive", &Entity::SetActive);
        entityType.set_function("Active", &Entity::Active);

        state.set_function("GetEntityByName", &GetEntityByName);
        state.set_function("FindEntityByName", &FindEntityByName);
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

        state.set_function("AddPhysicsSphere",
            [](Scene* scene, const std::string& name, const Vec3& pos, float radius,
               float inverseMass, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildSphereObject(scene, name, pos, radius, true, inverseMass, true, colour);
            });

        // Upright capsule body: `radius` and `segment` (distance between the two cap centres),
        // so it stands radius * 2 + segment tall. The usual shape for a character.
        state.set_function("AddPhysicsCapsule",
            [](Scene* scene, const std::string& name, const Vec3& pos, float radius, float segment,
               float inverseMass, const Vec4& colour) -> Entity
            {
                auto e = scene->GetEntityManager()->Create(name);
                // the capsule primitive is radius 0.5 with a 2 unit segment (3 tall)
                const float total = radius * 2.0f + segment;
                e.AddComponent<Maths::Transform>(Mat4::Translation(pos) * Mat4::Scale(Vec3(radius * 2.0f, total / 3.0f, radius * 2.0f)));
                auto& mc = e.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Capsule);

                auto mat = CreateSharedPtr<Graphics::Material>();
                Graphics::MaterialProperties props;
                props.albedoColour       = colour;
                props.roughness          = 0.7f;
                props.metallic           = 0.0f;
                props.albedoMapFactor    = 0.0f;
                props.roughnessMapFactor = 0.0f;
                props.normalMapFactor    = 0.0f;
                props.metallicMapFactor  = 0.0f;
                props.emissiveMapFactor  = 0.0f;
                props.occlusionMapFactor = 0.0f;
                mat->SetMaterialProperites(props);
                mc.InstanceMaterials[0] = mat;

                RigidBody3D* body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
                body->SetPosition(pos);
                body->SetInverseMass(inverseMass);
                body->SetCollisionShape(CreateSharedPtr<CapsuleCollisionShape>(radius, segment));
                body->SetInverseInertia(body->GetCollisionShape()->BuildInverseInertia(inverseMass));
                e.AddComponent<RigidBody3DComponent>(body);
                return e;
            });

        state.set_function("ImportGLTFScene", [](Scene* scene, const std::string& path) -> Entity
                           { return EntityFactory::ImportGLTFScene(scene, path); });

        state.set_function("AddMeshCollider", [](Entity e, sol::optional<std::string> lcolPath, sol::optional<bool> rebake) -> bool
            {
                if(!e.Valid() || !e.HasComponent<Graphics::ModelComponent>())
                    return false;

                auto shape = CreateSharedPtr<MeshCollisionShape>();
                if(lcolPath)
                {
                    shape->SetFilePath(lcolPath->c_str());
                    shape->SetShouldLoadFile(!rebake.value_or(false));
                }

                if(e.HasComponent<RigidBody3DComponent>())
                {
                    e.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetCollisionShape(shape);
                    return true;
                }
                RigidBody3D* body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
                body->SetIsStatic(true);
                body->SetCollisionShape(shape);
                e.AddComponent<RigidBody3DComponent>(body);
                return true;
            });

        state.set_function("GetMeshColliderTriangles", [](Entity e) -> int
            {
                if(!e.Valid() || !e.HasComponent<RigidBody3DComponent>())
                    return 0;
                auto shape = e.GetComponent<RigidBody3DComponent>().GetRigidBody()->GetCollisionShape();
                if(!shape || shape->GetType() != CollisionShapeType::CollisionMesh)
                    return 0;
                return (int)static_cast<MeshCollisionShape*>(shape.get())->GetTriangleCount();
            });

        state.set_function("AddPhysicsCube",
            [](Scene* scene, const std::string& name, const Vec3& pos, const Vec3& halfExtents,
               float inverseMass, const Vec4& colour) -> Entity
            {
                return EntityFactory::BuildCuboidObject(scene, name, pos, halfExtents, true, inverseMass, true, colour);
            });

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
                props.emissiveMapFactor  = 0.0f;
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
                props->emissiveColour = Vec4(albedo.x, albedo.y, albedo.z, emissive);
                mat->UpdateMaterialPropertiesData();
            });

        // Surface response of a primitive entity: roughness / metallic 0..1.
        state.set_function("SetEntityMaterial",
            [](Entity entity, const Vec4& albedo, float roughness, float metallic)
            {
                if(!entity.Valid() || !entity.HasComponent<Graphics::ModelComponent>()) return;
                auto& modelComp = entity.GetComponent<Graphics::ModelComponent>();
                auto it         = modelComp.InstanceMaterials.find(0);
                if(it == modelComp.InstanceMaterials.end() || !it->second) return;
                auto* props = it->second->GetProperties();
                if(!props) return;
                props->albedoColour = albedo;
                props->roughness    = roughness < 0.0f ? 0.0f : (roughness > 1.0f ? 1.0f : roughness);
                props->metallic     = metallic < 0.0f ? 0.0f : (metallic > 1.0f ? 1.0f : metallic);
                it->second->UpdateMaterialPropertiesData();
            });

        // Hide an entity's mesh without touching anything else: it keeps its physics body and
        // still casts a shadow. For a first person character's own body.
        state.set_function("SetEntityVisible",
            [](Entity entity, bool visible)
            {
                if(entity.Valid() && entity.HasComponent<Graphics::ModelComponent>())
                    entity.GetComponent<Graphics::ModelComponent>().Visible = visible;
            });

        state.set_function("SetModelEmissive",
            [](Graphics::Model* model, const Vec4& colour, float intensity)
            {
                if(!model) return;
                for(auto& mesh : model->GetMeshesRef())
                {
                    auto& mat = mesh->GetMaterial();
                    if(!mat) continue;
                    auto* props = mat->GetProperties();
                    if(!props) continue;
                    props->emissiveColour = Vec4(colour.x, colour.y, colour.z, intensity);
                    mat->UpdateMaterialPropertiesData();
                }
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
        sprite_type.set_function("SetTextureFromFile", &Sprite::SetTextureFromFile);
        sprite_type.set_function("SetNormalTexture", &Sprite::SetNormalTexture);
        sprite_type.set_function("SetNormalTextureFromFile", &Sprite::SetNormalTextureFromFile);
        sprite_type.set_function("SetPosition", &Sprite::SetPosition);
        sprite_type.set_function("SetScale", &Sprite::SetScale);
        sprite_type.set_function("SetColour", &Sprite::SetColour);
        sprite_type["SpriteSheetTileSizeX"] = &Sprite::SpriteSheetTileSizeX;
        sprite_type["SpriteSheetTileSizeY"] = &Sprite::SpriteSheetTileSizeY;
        sprite_type["ReceivesLight"]        = &Sprite::ReceivesLight;

        REGISTER_COMPONENT_WITH_ECS(state, Sprite, static_cast<Sprite& (Entity::*)(const Vec2&, const Vec2&, const Vec4&)>(&Entity::AddComponent<Sprite, const Vec2&, const Vec2&, const Vec4&>));

        sol::usertype<AnimatedSprite> AnimatedSpriteType = state.new_usertype<AnimatedSprite>("AnimatedSprite");
        AnimatedSpriteType.set_function("SetTexture", &AnimatedSprite::SetTexture);
        AnimatedSpriteType.set_function("SetTextureFromFile", &AnimatedSprite::SetTextureFromFile);
        AnimatedSpriteType.set_function("SetNormalTexture", &AnimatedSprite::SetNormalTexture);
        AnimatedSpriteType.set_function("SetNormalTextureFromFile", &AnimatedSprite::SetNormalTextureFromFile);
        AnimatedSpriteType.set_function("SetSpriteSheet", &AnimatedSprite::SetSpriteSheet);
        AnimatedSpriteType.set_function("SetSpriteSheetIndex", &AnimatedSprite::SetSpriteSheetIndex);
        AnimatedSpriteType.set_function("SetState", &AnimatedSprite::SetState);
        AnimatedSpriteType.set_function("GetState", &AnimatedSprite::GetState);
        AnimatedSpriteType.set_function("SetPosition", &AnimatedSprite::SetPosition);
        AnimatedSpriteType.set_function("SetScale", &AnimatedSprite::SetScale);
        AnimatedSpriteType.set_function("SetColour", &AnimatedSprite::SetColour);
        AnimatedSpriteType["SpriteSheetTileSizeX"] = &AnimatedSprite::SpriteSheetTileSizeX;
        AnimatedSpriteType["SpriteSheetTileSizeY"] = &AnimatedSprite::SpriteSheetTileSizeY;
        AnimatedSpriteType["FrameSize"]            = &AnimatedSprite::FrameSize;
        AnimatedSpriteType["ReceivesLight"]        = &AnimatedSprite::ReceivesLight;

        // AddState(name, frames, frameDuration [, pingPong]) - frames are {Vec2(px, py), ...} texture-pixel
        // top-left corners. Frame size comes from FrameSize (set it, or use AddGridState).
        AnimatedSpriteType.set_function("AddState",
            [](AnimatedSprite& sprite, const std::string& name, sol::table frames, float frameDuration, sol::optional<bool> pingPong)
            {
                std::vector<Vec2> parsed;
                parsed.reserve(frames.size());
                for(u32 i = 1; i <= (u32)frames.size(); i++)
                {
                    sol::optional<Vec2> f = frames[i];
                    parsed.push_back(f.value_or(Vec2(0.0f)));
                }

                sprite.AddState(parsed, frameDuration, name);
                if(pingPong.value_or(false))
                    sprite.SetPlayMode(name, AnimatedSprite::PlayMode::PingPong);
            });

        // AddGridState(name, cellSize, row, firstColumn, frameCount, frameDuration [, pingPong])
        // Convenience for uniform sprite sheets - builds the frame list from a grid row.
        AnimatedSpriteType.set_function("AddGridState",
            [](AnimatedSprite& sprite, const std::string& name, const Vec2& cellSize, int row, int firstColumn,
               int frameCount, float frameDuration, sol::optional<bool> pingPong)
            {
                std::vector<Vec2> frames;
                frames.reserve(frameCount > 0 ? (size_t)frameCount : 0);
                for(int i = 0; i < frameCount; i++)
                    frames.push_back(Vec2((firstColumn + i) * cellSize.x, row * cellSize.y));

                sprite.FrameSize = cellSize;
                sprite.AddState(frames, frameDuration, name);
                if(pingPong.value_or(false))
                    sprite.SetPlayMode(name, AnimatedSprite::PlayMode::PingPong);
            });

        // Retime a state at runtime - lets locomotion animation track movement
        // speed instead of sliding.
        AnimatedSpriteType.set_function("SetFrameDuration", &AnimatedSprite::SetFrameDuration);

        AnimatedSpriteType.set_function("SetPlayMode",
            [](AnimatedSprite& sprite, const std::string& name, int mode)
            { sprite.SetPlayMode(name, mode == 1 ? AnimatedSprite::PlayMode::PingPong : AnimatedSprite::PlayMode::Loop); });

        REGISTER_COMPONENT_WITH_ECS(state, AnimatedSprite, static_cast<AnimatedSprite& (Entity::*)()>(&Entity::AddComponent<AnimatedSprite>));

        sol::usertype<Light> lightType = state.new_usertype<Light>("Light");
        lightType["Intensity"] = &Light::Intensity;
        lightType["Radius"]    = &Light::Radius;
        lightType["Colour"]    = &Light::Colour;
        lightType["Direction"] = &Light::Direction;
        lightType["Position"]  = &Light::Position;
        lightType["Type"]      = &Light::Type;
        lightType["Angle"]     = &Light::Angle;
        lightType["CastShadows"]      = &Light::CastShadows;
        lightType["ShadowBias"]       = &Light::ShadowBias;
        lightType["ShadowNormalBias"] = &Light::ShadowNormalBias;
        lightType["ShadowNear"]       = &Light::ShadowNear;
        // 0 = auto, 1..4 = 256 / 512 / 1024 / 2048
        lightType["ShadowResolution"] = sol::property([](Light& l) { return (int)l.ShadowResolution; }, [](Light& l, int v) { l.ShadowResolution = (u8)(v < 0 ? 0 : (v > 4 ? 4 : v)); });
        // 0 = every frame, 1 = only when the light or something it lights moves
        lightType["ShadowUpdate"]     = sol::property([](Light& l) { return (int)l.ShadowUpdate; }, [](Light& l, int v) { l.ShadowUpdate = (u8)(v != 0 ? 1 : 0); });

        REGISTER_COMPONENT_WITH_ECS(state, Light, static_cast<Light& (Entity::*)()>(&Entity::AddComponent<Light>));

        sol::usertype<Light2D> light2DType = state.new_usertype<Light2D>("Light2D");
        light2DType["Colour"]     = &Light2D::Colour;
        light2DType["Intensity"]  = &Light2D::Intensity;
        light2DType["Radius"]     = &Light2D::Radius;
        light2DType["Type"]       = &Light2D::Type;
        light2DType["Height"]     = &Light2D::Height;
        light2DType["InnerAngle"] = &Light2D::InnerAngle;
        light2DType["OuterAngle"] = &Light2D::OuterAngle;
        light2DType["Falloff"]    = &Light2D::Falloff;
        light2DType["Position"]   = &Light2D::Position;
        light2DType["Direction"]  = &Light2D::Direction;

        REGISTER_COMPONENT_WITH_ECS(state, Light2D, static_cast<Light2D& (Entity::*)()>(&Entity::AddComponent<Light2D>));

        // box decal: the entity's transform is the box (scale = half size), projected down its -Y
        sol::usertype<DecalComponent> decalType = state.new_usertype<DecalComponent>("Decal");
        decalType["Colour"]     = &DecalComponent::Colour;
        decalType["AngleFade"]  = &DecalComponent::AngleFade;
        decalType["Roughness"]  = &DecalComponent::Roughness;
        decalType["Emissive"]   = &DecalComponent::Emissive;
        decalType["Order"]      = &DecalComponent::Order;
        decalType.set_function("SetTexture", &DecalComponent::SetTexture);
        decalType.set_function("GetTexturePath", [](DecalComponent& d) { return d.TexturePath; });

        REGISTER_COMPONENT_WITH_ECS(state, DecalComponent, static_cast<DecalComponent& (Entity::*)()>(&Entity::AddComponent<DecalComponent>));

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
            particleEmitter_type.set_function("SetLocalSpace", &ParticleEmitter::SetLocalSpace);
            particleEmitter_type.set_function("GetLocalSpace", &ParticleEmitter::GetLocalSpace);
            particleEmitter_type.set_function("SetSpawnOffset", &ParticleEmitter::SetSpawnOffset);
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

        // per-entity skeletal animation: clips are played, faded and blended by name
        auto clipOf = [](ModelComponent& mc, const std::string& name) -> int
        {
            AnimationController* animator = mc.GetAnimator();
            return animator ? animator->FindClip(name) : -1;
        };
        modelCompType["GetAnimationCount"] = [](ModelComponent& mc) -> int
        { return mc.ModelRef ? (int)mc.ModelRef->GetAnimations().Size() : 0; };
        modelCompType["GetAnimationName"] = [](ModelComponent& mc, int i) -> std::string
        {
            if(!mc.ModelRef || i < 0 || i >= (int)mc.ModelRef->GetAnimations().Size())
                return "";
            return mc.ModelRef->GetAnimations()[i]->GetName();
        };
        modelCompType["HasAnimation"] = [clipOf](ModelComponent& mc, const std::string& name) -> bool
        { return clipOf(mc, name) >= 0; };
        modelCompType["PlayAnimation"] = [clipOf](ModelComponent& mc, const std::string& name, sol::optional<float> fade, sol::optional<bool> loop, sol::optional<float> speed, sol::optional<bool> restart) -> bool
        {
            const int clip = clipOf(mc, name);
            if(clip < 0)
                return false;
            mc.AnimatorScripted = true;
            mc.GetAnimator()->Play(clip, fade.value_or(0.2f), loop.value_or(true), speed.value_or(1.0f), restart.value_or(false));
            return true;
        };
        modelCompType["SetAnimationWeight"] = [clipOf](ModelComponent& mc, const std::string& name, float weight, sol::optional<bool> loop)
        {
            const int clip = clipOf(mc, name);
            if(clip < 0)
                return;
            mc.AnimatorScripted = true;
            mc.GetAnimator()->SetLayerWeight(clip, weight, loop.value_or(true));
        };
        modelCompType["FadeAnimation"] = [clipOf](ModelComponent& mc, const std::string& name, float weight, float seconds)
        {
            const int clip = clipOf(mc, name);
            if(clip < 0)
                return;
            mc.AnimatorScripted = true;
            mc.GetAnimator()->FadeLayer(clip, weight, seconds);
        };
        modelCompType["SetAnimationSpeed"] = [clipOf](ModelComponent& mc, const std::string& name, float speed)
        {
            const int clip = clipOf(mc, name);
            if(clip >= 0)
                mc.GetAnimator()->SetLayerSpeed(clip, speed);
        };
        modelCompType["SetAnimationTime"] = [clipOf](ModelComponent& mc, const std::string& name, float seconds)
        {
            const int clip = clipOf(mc, name);
            if(clip >= 0)
                mc.GetAnimator()->SetLayerTime(clip, seconds);
        };
        modelCompType["GetAnimationTime"] = [clipOf](ModelComponent& mc, const std::string& name) -> float
        {
            const int clip = clipOf(mc, name);
            return clip >= 0 ? mc.GetAnimator()->GetLayerTime(clip) : -1.0f;
        };
        modelCompType["GetAnimationWeight"] = [clipOf](ModelComponent& mc, const std::string& name) -> float
        {
            const int clip = clipOf(mc, name);
            return clip >= 0 ? mc.GetAnimator()->GetLayerWeight(clip) : 0.0f;
        };
        modelCompType["GetAnimationDuration"] = [clipOf](ModelComponent& mc, const std::string& name) -> float
        {
            const int clip = clipOf(mc, name);
            return clip >= 0 ? mc.GetAnimator()->GetClipDuration(clip) : 0.0f;
        };
        modelCompType["IsAnimationFinished"] = [clipOf](ModelComponent& mc, const std::string& name) -> bool
        {
            const int clip = clipOf(mc, name);
            return clip >= 0 && mc.GetAnimator()->IsLayerFinished(clip);
        };

        modelCompType["SetRootMotionBone"] = [](ModelComponent& mc, const std::string& bone, sol::optional<bool> lockRotation) -> bool
        {
            AnimationController* animator = mc.GetAnimator();
            const int joint               = animator ? animator->FindJoint(bone) : -1;
            if(joint >= 0)
                animator->SetRootMotionJoint(joint, lockRotation.value_or(false));
            return joint >= 0;
        };
        modelCompType["ConsumeRootRotation"] = [](ModelComponent& mc) -> float
        {
            AnimationController* animator = mc.GetAnimator();
            return animator ? animator->ConsumeRootRotation() : 0.0f;
        };
        modelCompType["ConsumeRootMotion"] = [](ModelComponent& mc) -> Vec3
        {
            AnimationController* animator = mc.GetAnimator();
            return animator ? animator->ConsumeRootMotion() : Vec3(0.0f);
        };

        // where a bone is this frame, for hanging effects and aiming off a skeleton
        auto boneWorld = [](Entity entity, const std::string& bone, Mat4& out) -> bool
        {
            if(!entity.Valid() || !entity.HasComponent<ModelComponent>())
                return false;
            AnimationController* animator = entity.GetComponent<ModelComponent>().GetAnimator();
            Mat4 local;
            if(!animator || !animator->GetJointModelMatrix(animator->FindJoint(bone), local))
                return false;
            out = entity.GetTransform().GetWorldMatrix() * local;
            return true;
        };
        state.set_function("GetBoneWorldPosition", [boneWorld](Entity entity, const std::string& bone, sol::this_state ts) -> sol::object
                           {
            Mat4 m;
            if(!boneWorld(entity, bone, m))
                return sol::make_object(ts, sol::lua_nil);
            return sol::make_object(ts, m.Translation()); });
        state.set_function("GetBoneWorldRotation", [boneWorld](Entity entity, const std::string& bone, sol::this_state ts) -> sol::object
                           {
            Mat4 m;
            if(!boneWorld(entity, bone, m))
                return sol::make_object(ts, sol::lua_nil);
            return sol::make_object(ts, m.Rotation()); });

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
        soundNode_type.set_function("Play", [](SoundNode& node) { node.Resume(); });
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

        // AudioMixer: bus volumes, i.e. the global volume control. Every
        // SoundNode multiplies its own volume by its bus (default "Master")
        // in GetEffectiveVolume, and the AL source gain is refreshed from that
        // each update - so setting this takes effect on sounds already playing.
        // Named AudioMixer rather than Audio so it can't collide with a game's
        // own Lua audio module.
        auto audioMixer            = state["AudioMixer"].get_or_create<sol::table>();
        audioMixer["SetVolume"]    = [](float v)
        {
            if(auto* am = Application::Get().GetSystem<AudioManager>())
                am->SetBusVolume("Master", v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v));
        };
        audioMixer["GetVolume"]    = []() -> float
        {
            auto* am = Application::Get().GetSystem<AudioManager>();
            return am ? am->GetBusVolume("Master") : 1.0f;
        };
        audioMixer["SetBusVolume"] = [](const std::string& bus, float v)
        {
            if(auto* am = Application::Get().GetSystem<AudioManager>())
                am->SetBusVolume(bus, v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v));
        };
        audioMixer["GetBusVolume"] = [](const std::string& bus) -> float
        {
            auto* am = Application::Get().GetSystem<AudioManager>();
            return am ? am->GetBusVolume(bus) : 1.0f;
        };

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
        // Build the cube maps. Scene deserialisation does this for saved
        // environments; a script-created one has to ask for it, or there is no
        // sky and no image-based ambient at all.
        environment_type.set_function("Load", static_cast<void (Environment::*)()>(&Environment::Load));
        environment_type.set_function("LoadProcedural",
            [](Environment& env, sol::optional<Vec4> horizon, sol::optional<Vec4> zenith, sol::optional<Vec4> sunDir)
            {
                env.SetMode(1);
                if(horizon) env.SetHorizonColour(horizon.value());
                if(zenith)  env.SetZenithColour(zenith.value());
                if(sunDir)  env.SetSunDirection(sunDir.value());
                env.Load();
            });
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
        environment_type.set_function("GetStarParams",  &Environment::GetStarParams);
        environment_type.set_function("GetStarColour",  &Environment::GetStarColour);
        environment_type.set_function("SetStarParams",  &Environment::SetStarParams);
        environment_type.set_function("SetStarColour",  &Environment::SetStarColour);
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
        scene_type.set_function("GetSceneName", [](Scene& scene) -> std::string
                                { return scene.GetSceneName(); });
        // folder is a physical path ending in a slash; the file is <folder><scene name>.lsn
        scene_type.set_function("Serialise", [](Scene& scene, const std::string& folder)
                                { scene.Serialise(folder, false); });
        scene_type.set_function("Deserialise", [](Scene& scene, const std::string& folder)
                                { scene.Deserialise(folder, false); });
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

    static bool RemoveSceneFromProject(int index)
    {
        return index >= 0 && Application::Get().GetSceneManager()->RemoveScene((uint32_t)index);
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

    // 2D helpers: games doing their own AABB collision have no engine bodies for
    // the physics debug draw to show, so they draw their own boxes with these.
    static void DebugRect(const Vec2& min, const Vec2& max, const Vec4& colour, sol::optional<float> z, sol::optional<float> thickness)
    {
        const float depth = z.value_or(0.0f);
        const float t     = thickness.value_or(0.02f);
        const Vec3 bl(min.x, min.y, depth);
        const Vec3 br(max.x, min.y, depth);
        const Vec3 tr(max.x, max.y, depth);
        const Vec3 tl(min.x, max.y, depth);
        DebugRenderer::DrawThickLine(bl, br, t, true, colour, 0.0f);
        DebugRenderer::DrawThickLine(br, tr, t, true, colour, 0.0f);
        DebugRenderer::DrawThickLine(tr, tl, t, true, colour, 0.0f);
        DebugRenderer::DrawThickLine(tl, bl, t, true, colour, 0.0f);
    }

    static void DebugBox(const Vec2& centre, const Vec2& halfSize, const Vec4& colour, sol::optional<float> z, sol::optional<float> thickness)
    {
        DebugRect(Vec2(centre.x - halfSize.x, centre.y - halfSize.y),
                  Vec2(centre.x + halfSize.x, centre.y + halfSize.y),
                  colour, z, thickness);
    }

    static void DebugCircle2D(const Vec2& centre, float radius, const Vec4& colour, sol::optional<float> z, sol::optional<float> thickness)
    {
        DebugRenderer::DebugDrawCircle(24, radius, Vec3(centre.x, centre.y, z.value_or(0.0f)),
                                       Quat(), colour, thickness.value_or(0.02f));
    }

    static void DebugText3D(const Vec3& pos, const std::string& text, float size, const Vec4& colour)
    {
        DebugRenderer::DrawTextWs(pos, size, false, colour, 0.0f, text);
    }

    void LuaManager::BindAppLua(sol::state& state)
    {
        sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
        state.set_function("SwitchSceneByIndex", &SwitchSceneByIndex);
        state.set_function("RemoveSceneFromProject", &RemoveSceneFromProject);
        state.set_function("SwitchSceneByName", &SwitchSceneByName);
        state.set_function("SwitchScene", &SwitchScene);
        state.set_function("SetPhysicsDebugFlags", &SetPhysicsDebugFlags);
        state.set_function("ExitApp", &ExitApp);
        state.set_function("SetSkyColour", &SetSkyColour);

        // Platform query so games can pick touch vs desktop control schemes.
        state.set_function("GetPlatformName", []() -> std::string
                           {
#if defined(LUMOS_PLATFORM_IOS)
                               return "iOS";
#elif defined(LUMOS_PLATFORM_ANDROID)
                               return "Android";
#elif defined(LUMOS_PLATFORM_MACOS)
                               return "macOS";
#elif defined(LUMOS_PLATFORM_WINDOWS)
                               return "Windows";
#else
                               return "Linux";
#endif
                           });


        // ---- Debug overlay / command palette ----
        {
            auto dt = state["Debug"].get_or_create<sol::table>();

            auto panelFromName = [](const std::string& name) -> DebugPanel
            {
                if(name == "Stats")      return DebugPanel_Stats;
                if(name == "FrameGraph") return DebugPanel_FrameGraph;
                if(name == "Console")    return DebugPanel_Log;
                if(name == "Toggles")    return DebugPanel_Toggles;
                if(name == "Tweaks")     return DebugPanel_Tweaks;
                if(name == "Entities")   return DebugPanel_Entities;
                if(name == "Inspector")  return DebugPanel_Inspector;
                if(name == "FrameExplorer") return DebugPanel_FrameExplorer;
                if(name == "PhysicsExplorer") return DebugPanel_PhysicsExplorer;
                return DebugPanel_Count;
            };

            dt.set_function("RegisterCommand", [](const std::string& name, const std::string& category, sol::protected_function fn)
                            {
                                DebugMenu& menu = DebugMenu::Get();
                                menu.MarkNextAsScript(true);
                                menu.RegisterAction(name.c_str(), category.c_str(), [fn]()
                                                    {
                                                        sol::protected_function_result r = fn();
                                                        if(!r.valid())
                                                        {
                                                            sol::error err = r;
                                                            LERROR("Debug command error: %s", err.what());
                                                        }
                                                    });
                                menu.MarkNextAsScript(false);
                            });

            dt.set_function("RegisterToggle", [](const std::string& name, const std::string& category,
                                                 sol::protected_function get, sol::protected_function set)
                            {
                                DebugMenu& menu = DebugMenu::Get();
                                menu.MarkNextAsScript(true);
                                menu.RegisterToggle(
                                    name.c_str(), category.c_str(),
                                    [get]() -> bool
                                    {
                                        sol::protected_function_result r = get();
                                        if(!r.valid())
                                            return false;
                                        return r.get<bool>();
                                    },
                                    [set](bool v)
                                    {
                                        sol::protected_function_result r = set(v);
                                        if(!r.valid())
                                        {
                                            sol::error err = r;
                                            LERROR("Debug toggle error: %s", err.what());
                                        }
                                    });
                                menu.MarkNextAsScript(false);
                            });

            dt.set_function("RegisterFloat", [](const std::string& name, const std::string& category,
                                                sol::protected_function get, sol::protected_function set,
                                                float minValue, float maxValue)
                            {
                                DebugMenu& menu = DebugMenu::Get();
                                menu.MarkNextAsScript(true);
                                menu.RegisterFloat(
                                    name.c_str(), category.c_str(),
                                    [get]() -> float
                                    {
                                        sol::protected_function_result r = get();
                                        if(!r.valid())
                                            return 0.0f;
                                        return r.get<float>();
                                    },
                                    [set](float v)
                                    {
                                        sol::protected_function_result r = set(v);
                                        if(!r.valid())
                                        {
                                            sol::error err = r;
                                            LERROR("Debug float error: %s", err.what());
                                        }
                                    },
                                    minValue, maxValue);
                                menu.MarkNextAsScript(false);
                            });

            dt.set_function("Unregister", [](const std::string& name)
                            { DebugMenu::Get().Unregister(name.c_str()); });

            dt.set_function("Run", [](const std::string& name) -> bool
                            { return DebugMenu::Get().Run(name.c_str()); });

            dt.set_function("SetToggle", [](const std::string& name, bool v) -> bool
                            { return DebugMenu::Get().SetToggle(name.c_str(), v); });

            dt.set_function("Open", []() { DebugMenu::Get().OpenPalette(); });
            dt.set_function("Close", []() { DebugMenu::Get().ClosePalette(); });
            dt.set_function("Toggle", []() { DebugMenu::Get().TogglePalette(); });
            dt.set_function("IsOpen", []() { return DebugMenu::Get().IsPaletteOpen(); });
            dt.set_function("SetEnabled", [](bool v) { DebugMenu::Get().SetEnabled(v); });
            dt.set_function("IsEnabled", []() { return DebugMenu::Get().IsEnabled(); });
            dt.set_function("SetOpenKey", [](int key) { DebugMenu::Get().SetToggleKey(key); });

            // Selection is a raw entity id, matching Entity:GetID()-free code paths:
            // pass an Entity and we pull the handle out of it.
            dt.set_function("Select", [](Entity entity)
                            { DebugMenu::Get().SelectEntity((u32)entt::to_integral(entity.GetHandle())); });
            dt.set_function("ClearSelection", []() { DebugMenu::Get().ClearSelection(); });
            dt.set_function("GetSelected", [](sol::this_state s) -> sol::object
                            {
                                sol::state_view lua(s);
                                Scene* scene = Application::Get().GetCurrentScene();
                                const u32 id = DebugMenu::Get().GetSelectedEntity();
                                if(!scene || id == 0xFFFFFFFFu)
                                    return sol::lua_nil;
                                const entt::entity e = (entt::entity)id;
                                if(!scene->GetRegistry().valid(e))
                                    return sol::lua_nil;
                                return sol::make_object(lua, Entity(e, scene));
                            });
            dt.set_function("SetFlyCamera", [](bool v) { DebugMenu::Get().SetFlyCamera(v); });
            dt.set_function("IsFlyCamera", []() { return DebugMenu::Get().IsFlyCamera(); });

            // Docked tools (entity tree, inspector, console) around the game.
            dt.set_function("SetWorkspace", [](bool v) { DebugWorkspace::SetVisible(v); });
            dt.set_function("IsWorkspace", []() { return DebugWorkspace::IsVisible(); });

            // "move" / "rotate" / "scale" / "universal"
            dt.set_function("SetGizmo", [](const std::string& mode)
                            {
                                u32 operation = GizmoOperation_Translate;
                                if(mode == "rotate")         operation = GizmoOperation_Rotate;
                                else if(mode == "scale")     operation = GizmoOperation_Scale;
                                else if(mode == "universal") operation = GizmoOperation_Universal;
                                DebugMenu::Get().SetGizmoOperation(operation);
                            });
            dt.set_function("SetGizmoLocal", [](bool v) { DebugMenu::Get().SetGizmoLocal(v); });
            dt.set_function("SetGizmoSnap", [](bool v) { DebugMenu::Get().SetGizmoSnap(v); });

            // Undo stack shared by the overlay's gizmo and inspector.
            sol::table undo = state.create_named_table("Undo");
            undo.set_function("Undo", []() { Lumos::Undo(); });
            undo.set_function("Redo", []() { Lumos::Redo(); });
            undo.set_function("Clear", []() { Lumos::UndoClear(); });
            undo.set_function("UndoCount", []() { return Lumos::UndoStepCount(); });
            undo.set_function("RedoCount", []() { return Lumos::RedoStepCount(); });

            // Loading overlay. Busy.Pump() draws a frame from inside a long script step.
            sol::table busy = state.create_named_table("Busy");
            busy.set_function("Begin", [](const std::string& label) { Busy::Begin(label.c_str()); });
            busy.set_function("End", []() { Busy::End(); });
            busy.set_function("IsBusy", []() { return Busy::IsBusy(); });
            busy.set_function("SetStep", [](const std::string& step) { Busy::SetStep(step.c_str()); });
            busy.set_function("SetProgress", [](float progress) { Busy::SetProgress(progress); });
            busy.set_function("Pump", []() { Busy::Pump(); });
            dt.set_function("SetFlyMode", [](const std::string& mode)
                            {
                                if(mode == "2d" || mode == "2D")      DebugMenu::Get().SetFlyMode(1);
                                else if(mode == "3d" || mode == "3D") DebugMenu::Get().SetFlyMode(2);
                                else                                  DebugMenu::Get().SetFlyMode(0);
                            });

            dt.set_function("ShowPanel", [panelFromName](const std::string& name, bool visible)
                            {
                                DebugPanel p = panelFromName(name);
                                if(p != DebugPanel_Count)
                                    DebugMenu::Get().SetPanelVisible(p, visible);
                            });

            dt.set_function("IsPanelVisible", [panelFromName](const std::string& name) -> bool
                            {
                                DebugPanel p = panelFromName(name);
                                return p != DebugPanel_Count && DebugMenu::Get().IsPanelVisible(p);
                            });

            // Frame explorer. Passes are 1-based here; 0 = the whole frame.
            dt.set_function("GetPassCount", []() { return (int)Graphics::RenderPass_Count; });
            dt.set_function("GetPassName", [](int pass) -> std::string
                            {
                                if(pass < 1 || pass > (int)Graphics::RenderPass_Count)
                                    return "";
                                return Graphics::GetRenderPassInfo((Graphics::RenderPassID)(pass - 1)).Name;
                            });
            dt.set_function("GetPassState", [](int pass) -> std::string
                            {
                                Graphics::SceneRenderer* renderer = Graphics::SceneRenderer::GetGameRenderer();
                                if(!renderer || pass < 1 || pass > (int)Graphics::RenderPass_Count)
                                    return "off";
                                switch(renderer->GetPassRecord((Graphics::RenderPassID)(pass - 1)).State)
                                {
                                case Graphics::RenderPassState::Ran:    return "ran";
                                case Graphics::RenderPassState::CutOff: return "cut";
                                default:                                return "off";
                                }
                            });
            dt.set_function("SetPassCutoff", sol::overload(
                                                 [](int pass) { DebugMenu::Get().SetPassCutoff(pass - 1); },
                                                 [](const std::string& name)
                                                 {
                                                     for(u32 i = 0; i < Graphics::RenderPass_Count; i++)
                                                     {
                                                         if(name == Graphics::GetRenderPassInfo((Graphics::RenderPassID)i).Name)
                                                         {
                                                             DebugMenu::Get().SetPassCutoff((i32)i);
                                                             return;
                                                         }
                                                     }
                                                 }));
            dt.set_function("GetPassCutoff", []() { return DebugMenu::Get().GetPassCutoff() + 1; });
            dt.set_function("StepPass", [](int direction) { DebugMenu::Get().StepPass(direction); });

            // Physics explorer (3D): advance the simulation one stage / substep at a time.
            dt.set_function("SetPhysicsStepping", [](bool manual)
                            {
                                if(LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>())
                                    p->SetManualStep(manual);
                            });
            dt.set_function("IsPhysicsStepping", []() -> bool
                            {
                                LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                                return p && p->GetManualStep();
                            });
            dt.set_function("StepPhysicsStage", [](sol::optional<int> count)
                            {
                                if(LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>())
                                {
                                    p->SetManualStep(true);
                                    const int stages = count.value_or(1);
                                    p->StepStages(stages > 0 ? (uint32_t)stages : 0);
                                }
                            });
            dt.set_function("StepPhysicsSubstep", []()
                            {
                                if(LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>())
                                {
                                    p->SetManualStep(true);
                                    p->StepSubstep();
                                }
                            });
            // 1-based index of the stage that runs next
            dt.set_function("GetPhysicsStage", []() -> int
                            {
                                LumosPhysicsEngine* p = Application::Get().GetSystem<LumosPhysicsEngine>();
                                return p ? (int)p->GetNextStage() + 1 : 1;
                            });
        }

        {
            auto rt = state["Render"].get_or_create<sol::table>();

            auto getRS = []() -> Scene::SceneRenderSettings* {
                Scene* s = Application::Get().GetCurrentScene();
                return s ? &s->GetSettings().RenderSettings : nullptr;
            };

            rt.set_function("SetBool", [getRS](const std::string& name, bool v) {
                if(name == "LowShader") // cheap forward shading
                {
                    Application::Get().GetQualitySettings().LowForwardShader = v;
                    return;
                }
                if(name == "AutoQuality") // frame-time driven render scale
                {
                    Application::Get().GetQualitySettings().DynamicResolution = v;
                    return;
                }
                auto* rs = getRS(); if(!rs) return;
                if(name == "Shadows")            rs->ShadowsEnabled = v;
                else if(name == "FXAA")          rs->FXAAEnabled = v;
                else if(name == "SMAA")          rs->SMAAEnabled = v;
                else if(name == "Bloom")         { rs->BloomEnabled = v; Application::Get().GetQualitySettings().EnableBloom = v; } // a game's choice beats the quality level
                else if(name == "SSAO")          { rs->SSAOEnabled = v; Application::Get().GetQualitySettings().EnableSSAO = v; }
                else if(name == "SSR")           { rs->SSREnabled = v; Application::Get().GetQualitySettings().EnableSSR = v; }
                else if(name == "Vignette")      rs->VignetteEnabled = v;
                else if(name == "DepthOfField")  rs->DepthOfFieldEnabled = v;
                else if(name == "MotionBlur")    rs->MotionBlurEnabled = v;
                else if(name == "FilmicGrain")   rs->FilmicGrainEnabled = v;
                else if(name == "ChromaticAberration") rs->ChromaticAberationEnabled = v;
                else if(name == "Sharpen")       rs->SharpenEnabled = v;
                else if(name == "Debanding")     rs->DebandingEnabled = v;
                else if(name == "Skybox")        rs->SkyboxRenderEnabled = v;
                else if(name == "Debug")         rs->DebugRenderEnabled = v;
                else if(name == "Wireframe")     rs->Wireframe = v;
                else if(name == "OcclusionCulling") rs->OcclusionCulling = v;
                else if(name == "LightRange")    rs->LightRange = v;
                else if(name == "DepthPrePass")  rs->DepthPrePass = v;
                else if(name == "TAA")           rs->TAAEnabled = v;
                else if(name == "ContactShadows") rs->ContactShadowsEnabled = v;
                else if(name == "VolumetricFog") rs->VolumetricFogEnabled = v;
                else if(name == "GI")            rs->GIEnabled = v;
                else if(name == "AdaptiveExposure") rs->AdaptiveExposure = v;
            });

            rt.set_function("GetBool", [getRS](const std::string& name) -> bool {
                if(name == "AutoQuality")
                    return Application::Get().GetQualitySettings().DynamicResolution;
                auto* rs = getRS(); if(!rs) return false;
                if(name == "Shadows")            return rs->ShadowsEnabled;
                else if(name == "FXAA")          return rs->FXAAEnabled;
                else if(name == "SMAA")          return rs->SMAAEnabled;
                else if(name == "Bloom")         return rs->BloomEnabled;
                else if(name == "SSAO")          return rs->SSAOEnabled;
                else if(name == "SSR")           return rs->SSREnabled;
                else if(name == "Vignette")      return rs->VignetteEnabled;
                else if(name == "DepthOfField")  return rs->DepthOfFieldEnabled;
                else if(name == "MotionBlur")    return rs->MotionBlurEnabled;
                else if(name == "FilmicGrain")   return rs->FilmicGrainEnabled;
                else if(name == "ChromaticAberration") return rs->ChromaticAberationEnabled;
                else if(name == "Sharpen")       return rs->SharpenEnabled;
                else if(name == "Debanding")     return rs->DebandingEnabled;
                else if(name == "Skybox")        return rs->SkyboxRenderEnabled;
                else if(name == "Debug")         return rs->DebugRenderEnabled;
                else if(name == "Wireframe")     return rs->Wireframe;
                else if(name == "OcclusionCulling") return rs->OcclusionCulling;
                else if(name == "LightRange")    return rs->LightRange;
                else if(name == "DepthPrePass")  return rs->DepthPrePass;
                else if(name == "TAA")           return rs->TAAEnabled;
                else if(name == "ContactShadows") return rs->ContactShadowsEnabled;
                else if(name == "VolumetricFog") return rs->VolumetricFogEnabled;
                else if(name == "GI")            return rs->GIEnabled;
                else if(name == "AdaptiveExposure") return rs->AdaptiveExposure;
                return false;
            });

            rt.set_function("SetFloat", [getRS](const std::string& name, float v) {
                // Not a SceneRenderSettings field: the scene renders into targets
                // sized by this and the result is upscaled to the window, so it
                // needs the render targets rebuilt.
                if(name == "Quality") // 0 low .. 2 high, -1 = pick from the GPU
                {
                    Application::Get().SetQualityLevel((int)v);
                    Application::Get().GetQualitySettings().DynamicResolution = v < 0.0f;
                    return;
                }
                if(name == "SSAOQuality") // 1 low (8 samples) .. 3 high (32)
                {
                    const int q = v < 1.0f ? 1 : (v > 3.0f ? 3 : (int)v);
                    Application::Get().GetQualitySettings().SSAOQuality = (SSAOQualitySetting)q;
                    return;
                }
                if(name == "RenderScale")
                {
                    Application& app = Application::Get();
                    app.GetQualitySettings().DynamicResolution = false; // a set scale is a choice
                    app.GetQualitySettings().RendererScale = v < 0.25f ? 0.25f : (v > 2.0f ? 2.0f : v);
                    app.OnSceneViewSizeUpdated(app.GetWindow()->GetWidth(), app.GetWindow()->GetHeight());
                    return;
                }
                // shadow fitting lives on the renderer, not the scene
                // the editor's Game view draws with its own renderer, so set both
                Graphics::SceneRenderer* renderers[2] = { Application::Get().GetSceneRenderer(), Graphics::SceneRenderer::GetGameRenderer() };
                bool wasShadowSetting                 = false;
                for(u32 i = 0; i < 2; i++)
                {
                    if(!renderers[i] || (i == 1 && renderers[1] == renderers[0]))
                        continue;
                    auto& shadows = renderers[i]->GetShadowData();
                    if(name == "ShadowDistance")      { shadows.m_MaxShadowDistance = v > 1.0f ? v : 1.0f; wasShadowSetting = true; }
                    else if(name == "ShadowSplit")    { shadows.m_CascadeSplitLambda = v; wasShadowSetting = true; }
                    else if(name == "ShadowSoftness") { shadows.m_LightSize = v; wasShadowSetting = true; }
                    else if(name == "ShadowBias")     { shadows.m_InitialBias = v; wasShadowSetting = true; }
                }
                if(wasShadowSetting)
                    return;
                if(name == "RenderMode") // forward shading view: 0 lit, 1 albedo .. 8 uv, 9 GI only
                {
                    for(Graphics::SceneRenderer* r : renderers)
                        if(r)
                            r->GetForwardData().m_RenderMode = (uint32_t)v;
                    return;
                }
                auto* rs = getRS(); if(!rs) return;
                if(name == "Exposure")           rs->m_Exposure = v;
                else if(name == "BloomIntensity") rs->m_BloomIntensity = v;
                else if(name == "BloomThreshold") rs->BloomThreshold = v;
                else if(name == "Saturation")    rs->Saturation = v;
                else if(name == "Contrast")      rs->Contrast = v;
                else if(name == "Brightness")    rs->Brightness = v;
                else if(name == "VignetteIntensity") rs->VignetteIntensity = v;
                else if(name == "SSRStrength")   rs->SSRStrength = v;
                else if(name == "SSRMaxRoughness") rs->SSRMaxRoughness = v;
                else if(name == "SSAOStrength")  rs->SSAOStrength = v;
                else if(name == "SSAORadius")    rs->SSAOSampleRadius = v;
                else if(name == "DepthOfFieldDistance") rs->DepthOfFieldDistance = v;
                else if(name == "DepthOfFieldStrength") rs->DepthOfFieldStrength = v;
                else if(name == "MinScreenSize") rs->MinScreenSize = v;
                else if(name == "MaxSpotShadows") rs->MaxSpotShadows = (int)v;
                else if(name == "SpotShadowResolution") rs->SpotShadowMaxResolution = (int)v;
                else if(name == "SpotShadowDistance") rs->SpotShadowDistance = v;
                else if(name == "DebugMode")     rs->DebugMode = (int)v; // forward shader buffer view: 1 albedo .. 6 normal, 8 uv
                else if(name == "FogDensity")    rs->VolumetricFogDensity = v;
                else if(name == "FogAnisotropy") rs->VolumetricFogAnisotropy = v;
                else if(name == "GIIntensity")   rs->GIIntensity = v;
                else if(name == "GIProbeSpacing") rs->GIProbeSpacing = v;
                else if(name == "ContactShadowLength") rs->ContactShadowLength = v;
            });

            // environment probe: the scene from a point lights it (reflections + ambient) in place of the sky
            rt.set_function("CaptureProbe", [](float x, float y, float z, sol::optional<int> size, sol::optional<float> gain, sol::optional<int> index) -> int {
                Graphics::SceneRenderer* r = Application::Get().GetSceneRenderer();
                return r ? r->CaptureEnvironmentProbe(Vec3(x, y, z), (u32)size.value_or(128), gain.value_or(1.0f), index.value_or(-1)) : -1;
            });
            rt.set_function("GetProbeCount", []() -> int {
                Graphics::SceneRenderer* r = Application::Get().GetSceneRenderer();
                return r ? (int)r->GetEnvironmentProbeCount() : 0;
            });
            rt.set_function("ClearProbes", []() {
                if(Graphics::SceneRenderer* r = Application::Get().GetSceneRenderer())
                    r->ClearEnvironmentProbes();
            });
            rt.set_function("IsCapturingProbe", []() -> bool {
                Graphics::SceneRenderer* r = Application::Get().GetSceneRenderer();
                return r && r->IsCapturingProbe();
            });

            // meshes the occlusion grid dropped from the forward queue last frame
            rt.set_function("GetOcclusionCulled", []() -> int {
                Graphics::SceneRenderer* r = Application::Get().GetSceneRenderer();
                return r ? (int)r->GetSceneRendererStats().NumOcclusionCulled : 0;
            });

            // last frame's renderer counters: "Objects", "Shadow", "Instanced", "Batches", "Occluded"
            rt.set_function("GetStat", [](const std::string& name) -> int {
                Graphics::SceneRenderer* r = Application::Get().GetSceneRenderer();
                if(!r)
                    return 0;
                const auto& s = r->GetSceneRendererStats();
                if(name == "Triangles") return (int)Engine::Get().Statistics().TriangleCount;
                if(name == "Draws")     return (int)Engine::Get().Statistics().NumDrawCalls;
                if(name == "Objects")   return (int)s.NumRenderedObjects;
                if(name == "Shadow")    return (int)s.NumShadowObjects;
                if(name == "Instanced") return (int)s.NumInstancedObjects;
                if(name == "Batches")   return (int)s.NumInstanceBatches;
                if(name == "Occluded")  return (int)s.NumOcclusionCulled;
                if(name == "SmallCulled") return (int)s.NumSmallCulled;
                if(name == "LODMeshes") return (int)s.NumLODMeshes;
                return 0;
            });

            rt.set_function("GetFloat", [getRS](const std::string& name) -> float {
                if(name == "RenderScale")
                    return Application::Get().GetQualitySettings().RendererScale;
                if(name == "Quality")
                    return (float)Application::Get().GetQualitySettings().GeneralLevel;
                if(Graphics::SceneRenderer* renderer = Application::Get().GetSceneRenderer())
                {
                    auto& shadows = renderer->GetShadowData();
                    if(name == "ShadowDistance") return shadows.m_MaxShadowDistance;
                    if(name == "ShadowSplit")    return shadows.m_CascadeSplitLambda;
                    if(name == "ShadowSoftness") return shadows.m_LightSize;
                    if(name == "ShadowBias")     return shadows.m_InitialBias;
                }
                auto* rs = getRS(); if(!rs) return 0.0f;
                if(name == "Exposure")           return rs->m_Exposure;
                else if(name == "BloomIntensity") return rs->m_BloomIntensity;
                else if(name == "MinScreenSize") return rs->MinScreenSize;
                else if(name == "MaxSpotShadows") return (float)rs->MaxSpotShadows;
                else if(name == "SpotShadowResolution") return (float)rs->SpotShadowMaxResolution;
                else if(name == "SpotShadowDistance") return rs->SpotShadowDistance;
                else if(name == "BloomThreshold") return rs->BloomThreshold;
                else if(name == "Saturation")    return rs->Saturation;
                else if(name == "Contrast")      return rs->Contrast;
                else if(name == "Brightness")    return rs->Brightness;
                else if(name == "VignetteIntensity") return rs->VignetteIntensity;
                else if(name == "SSRStrength")   return rs->SSRStrength;
                else if(name == "SSAOStrength")  return rs->SSAOStrength;
                else if(name == "SSAORadius")    return rs->SSAOSampleRadius;
                return 0.0f;
            });
        }

        {
            auto ct = state["Cosmo"].get_or_create<sol::table>();
            ct.set_function("SetEnabled", [](bool e)
                            { Graphics::CosmoBackdrop::Get().Enabled = e; });
            ct.set_function("GetEnabled", []() -> bool
                            { return Graphics::CosmoBackdrop::Get().Enabled; });
            ct.set_function("SetGain", [](float g)
                            { Graphics::CosmoBackdrop::Get().Gain = g; });
            // Any key left out keeps the value it had. fade = seconds to blend in the new sky.
            ct.set_function("SetRegion", [](sol::table t)
                            {
                                auto& c = Graphics::CosmoBackdrop::Get();
                                auto& r = c.Sky;
                                r.Seed        = t.get_or("seed", r.Seed);
                                r.Offset      = t.get_or("offset", r.Offset);
                                r.Nebula      = t.get_or("nebula", r.Nebula);
                                r.Band        = t.get_or("band", r.Band);
                                r.Scale       = t.get_or("scale", r.Scale);
                                r.Wander      = t.get_or("wander", r.Wander);
                                r.Tightness   = t.get_or("tightness", r.Tightness);
                                r.Bulge       = t.get_or("bulge", r.Bulge);
                                r.Lanes       = t.get_or("lanes", r.Lanes);
                                r.Density     = t.get_or("density", r.Density);
                                r.Pole        = t.get_or("pole", r.Pole);
                                r.Centre      = t.get_or("centre", r.Centre);
                                r.ColourA     = t.get_or("colA", r.ColourA);
                                r.ColourB     = t.get_or("colB", r.ColourB);
                                r.ColourC     = t.get_or("colC", r.ColourC);
                                r.Tint        = t.get_or("tint", r.Tint);
                                r.FadeSeconds = t.get_or("fade", r.FadeSeconds);
                                c.Version++;
                            });
        }

        state.set_function("DebugLine", &DebugLine);
        state.set_function("DebugPoint", &DebugPoint);
        state.set_function("DebugRect", &DebugRect);
        state.set_function("DebugBox", &DebugBox);
        state.set_function("DebugCircle", &DebugCircle2D);
        state.set_function("DebugText3D", &DebugText3D);

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
#ifdef LUMOS_PLATFORM_IOS
        fileDialog.set_function("ImportFiles", [](const std::string& filter, const std::string& destDir, sol::this_state ts) -> sol::table {
            sol::state_view lua(ts);
            sol::table out = lua.create_table();
            TDArray<std::string> files = FileDialogs::ImportFiles(filter, destDir);
            for(u32 i = 0; i < files.Size(); i++)
                out[i + 1] = files[i];
            return out;
        });
#endif

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
                if(!texture || !Application::Get().GetImGuiManager()) return;
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
                if(!Application::Get().GetImGuiManager()) return;
                auto imguiRenderer = Application::Get().GetImGuiManager()->GetImGuiRenderer();
                bool flip = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
                ImGui::Image(
                    imguiRenderer->AddTexture(texture),
                    ImVec2(w, h),
                    ImVec2(0.0f, flip ? 1.0f : 0.0f),
                    ImVec2(1.0f, flip ? 0.0f : 1.0f));
            }));

        // Write string to file (resolves VFS paths like //Assets/... same as ReadFile)
        state.set_function("WriteFile", [](const std::string& path, const std::string& content) -> bool {
            std::string physicalPath = path;
            if(path.size() >= 2 && path[0] == '/' && path[1] == '/')
            {
                ArenaTemp scratch = ScratchBegin(0, 0);
                String8 resolved = {};
                FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8C((char*)path.c_str()), &resolved);
                if(resolved.size > 0)
                    physicalPath = std::string((const char*)resolved.str, resolved.size);
                ScratchEnd(scratch);
            }
            FILE* f = fopen(physicalPath.c_str(), "w");
            if(!f) return false;
            fwrite(content.c_str(), 1, content.size(), f);
            fclose(f);
            return true;
        });

        // Adds to the end of a file (creates it): logs / histories without rewriting the whole file
        state.set_function("AppendFile", [](const std::string& path, const std::string& content) -> bool {
            std::string physicalPath = path;
            if(path.size() >= 2 && path[0] == '/' && path[1] == '/')
            {
                ArenaTemp scratch = ScratchBegin(0, 0);
                String8 resolved = {};
                FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8C((char*)path.c_str()), &resolved);
                if(resolved.size > 0)
                    physicalPath = std::string((const char*)resolved.str, resolved.size);
                ScratchEnd(scratch);
            }
            FILE* f = fopen(physicalPath.c_str(), "ab");
            if(!f) return false;
            bool ok = fwrite(content.c_str(), 1, content.size(), f) == content.size();
            fclose(f);
            return ok;
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
            "ClearTexture", &Graphics::ShaderPreview::ClearChannelTexture,
            "SetMouse", &Graphics::ShaderPreview::SetMouse,
            "SetFrame", &Graphics::ShaderPreview::SetFrame);
        {
            sol::usertype<Graphics::ShaderPreview> previewType = state["ShaderPreview"];
            // textures from Lua are shared: keep a reference on the existing count (nil clears)
            previewType["SetTexture"] = [](Graphics::ShaderPreview& p, uint32_t channel, sol::object tex)
            {
                if(tex.is<SharedPtr<Graphics::Texture2D>>())
                    p.SetChannelTexture(channel, tex.as<SharedPtr<Graphics::Texture2D>>());
                else if(tex.is<Graphics::Texture2D*>())
                    p.SetChannelTexture(channel, tex.as<Graphics::Texture2D*>());
                else
                    p.ClearChannelTexture(channel);
            };
            previewType["ExportPNG"] = [](Graphics::ShaderPreview& p, const std::string& path, int w, int h, float time, sol::optional<int> ss) -> bool
            { return p.ExportPNG(path, (uint32_t)std::max(1, w), (uint32_t)std::max(1, h), time, (uint32_t)std::max(1, ss.value_or(1))); };
            previewType["GetPrefixLineCount"]  = []() -> int { return Graphics::ShaderPreview::GetPrefixLineCount(); };
            previewType["IsCompilerAvailable"] = []() -> bool { return Graphics::ShaderCompiler::IsAvailable(); };
        }

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
        {
            sol::table icons = lua.create_named_table("Icons");
            for(uint32_t i = 0; i < g_MDIIconCount; i++)
            {
                u8 buf[8];
                u32 n     = Utf8Encode(buf, g_MDIIcons[i].codepoint);
                buf[n]    = 0;
                icons[g_MDIIcons[i].name] = std::string((const char*)buf, n);
            }
        }

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
                     "AnimateAppear", Lumos::WidgetFlags_AnimateAppear,
                     "AlignRight", Lumos::WidgetFlags_AlignRight,
                     "AnimateExit", Lumos::WidgetFlags_AnimateExit);

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
                     "PercentOfViewport", Lumos::SizeKind_PercentOfViewport,
                     "Grow", Lumos::SizeKind_Grow);

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

        lua.new_enum("UITheme",
                     "Light", Lumos::UITheme_Light,
                     "Dark", Lumos::UITheme_Dark,
                     "Blue", Lumos::UITheme_Blue,
                     "Green", Lumos::UITheme_Green,
                     "Purple", Lumos::UITheme_Purple,
                     "HighContrast", Lumos::UITheme_HighContrast);

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
                                                "pressed", &Lumos::UI_Interaction::pressed,
                                                "right_clicked", &Lumos::UI_Interaction::right_clicked,
                                                "double_clicked", &Lumos::UI_Interaction::double_clicked,
                                                "dragging", &Lumos::UI_Interaction::dragging);

        // Functions
        lua["GetUIState"]    = &Lumos::GetUIState;
        lua["GetStringSize"] = &Lumos::GetStringSize;
        lua["InitialiseUI"]  = &Lumos::InitialiseUI;
        lua["ShutDownUI"]    = &Lumos::ShutDownUI;
        lua["UIBeginFrame"]  = &Lumos::UIBeginFrame;
        lua["UIEndFrame"]    = &Lumos::UIEndFrame;
        lua["UIBeginPanel"]  = sol::overload(
            [](const char* str) { return Lumos::UIBeginPanel(str); },
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
        lua["UISetNextFlags"] = &Lumos::UISetNextFlags;
        lua["UILabel"]    = &Lumos::UILabelCStr;
        lua.set_function("UILabelWrapped", [](const char* name, const char* text, float maxWidth) {
            return Lumos::UILabelWrapped(name, Str8C((char*)text), maxWidth);
        });
        lua.set_function("UILabelLink", [](const char* name, const char* text, float maxWidth) {
            return Lumos::UILabelLink(name, Str8C((char*)text), maxWidth);
        });
        lua.set_function("UILabelEllipsis", [](const char* name, const char* text, float maxWidth) {
            return Lumos::UILabelEllipsis(name, Str8C((char*)text), maxWidth);
        });
        lua["UIButton"]   = &Lumos::UIButton;
        lua["UIImage"]    = &Lumos::UIImage;
        lua.set_function("UISeparator", [](sol::optional<float> w) { Lumos::UISeparator(w.value_or(0.0f)); });
        lua.set_function("UISetTheme", [](Lumos::UITheme theme) { Lumos::UISetTheme(theme); });
        lua.set_function("UISetAccentColour", [](const Vec4& colour) { Lumos::UISetAccentColour(colour); });
        lua.set_function("UISetClickOnRelease", [](bool enable) { Lumos::UISetClickOnRelease(enable); });
        lua.set_function("UIPushMonoText", []() { Lumos::UIPushMonoText(); });
        lua.set_function("UIPopMonoText", []() { Lumos::UIPopMonoText(); });
        // on-screen keyboard (touch): show / hide, "code" adds the code key bar, cover = 0..1 of the screen height
        lua.set_function("UIShowKeyboard", [](bool show) { Lumos::UIShowKeyboard(show); });
        lua.set_function("UISetKeyboardStyle", [](const std::string& style) { Lumos::UISetKeyboardStyle(style == "code" ? Lumos::UIKeyboardStyle_Code : Lumos::UIKeyboardStyle_Plain); });
        lua.set_function("UIKeyboardCover", []() { return Lumos::UIKeyboardCover(); });
        lua.set_function("UISetLayeredBatching", [](bool enable) { Lumos::UISetLayeredBatching(enable); });
        lua.set_function("UIClickableRects", [](sol::this_state ts)
                         {
            sol::state_view view(ts);
            sol::table out = view.create_table();
            Vec4 rects[256];
            const u32 n = UIGetClickableRects(rects, 256);
            for(u32 i = 0; i < n; i++)
                out[i + 1] = view.create_table_with("x", rects[i].x, "y", rects[i].y, "w", rects[i].z, "h", rects[i].w);
            return out; });
        lua.set_function("UISetScrollbarFade", [](bool on) { Lumos::UISetScrollbarFade(on); });
        lua.set_function("UISetScrollPhysics", [](bool on) { Lumos::UISetScrollPhysics(on); });
        // Scroll area (engine-held offset; wheel/thumb on desktop, finger-drag pan on touch)
        lua.set_function("UIBeginScrollArea", [](const char* id, float height) { Lumos::UIBeginScrollAreaAuto(id, height); });
        lua.set_function("UIEndScrollArea", []() { Lumos::UIEndScrollArea(); });
        lua.set_function("UIFocusNextTextInput", []() { Lumos::UIFocusNextTextInput(); });
        lua.set_function("UISetSafeAreaConfig", [](sol::optional<float> minMargin,
                                                   sol::optional<float> scaleTop,
                                                   sol::optional<float> scaleBottom,
                                                   sol::optional<float> scaleLeft,
                                                   sol::optional<float> scaleRight) {
            Lumos::UISafeAreaConfig cfg = Lumos::UIGetSafeAreaConfig();
            cfg.MinMargin   = minMargin.value_or(cfg.MinMargin);
            cfg.ScaleTop    = scaleTop.value_or(cfg.ScaleTop);
            cfg.ScaleBottom = scaleBottom.value_or(cfg.ScaleBottom);
            cfg.ScaleLeft   = scaleLeft.value_or(cfg.ScaleLeft);
            cfg.ScaleRight  = scaleRight.value_or(cfg.ScaleRight);
            Lumos::UISetSafeAreaConfig(cfg);
        });
        // Resolved safe-area insets in pixels: Vec4(left, top, right, bottom). The UI root already sits inside them.
        lua.set_function("UIGetSafeAreaInsets", []() {
            const bool isEditor = Lumos::Application::Get().GetAppType() == Lumos::AppType::Editor;
            Lumos::SafeAreaInsets sa = Lumos::UIResolveSafeAreaInsets(Lumos::OS::Get().GetSafeAreaInsets(), isEditor);
            return Lumos::Vec4(sa.left, sa.top, sa.right, sa.bottom);
        });
        lua.set_function("UIIsMouseOverUI", []() { return Lumos::UIIsMouseOverUI(); });
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

        lua.set_function("UISliderRow", [](const char* label, float value, sol::optional<float> min,
                                           sol::optional<float> max, sol::optional<std::string> fmt) -> float
                         {
                             float tmp = value;
                             Lumos::UISliderRow(label, &tmp,
                                                min.value_or(0.0f),
                                                max.value_or(1.0f),
                                                fmt ? fmt->c_str() : "%.2f");
                             return tmp;
                         });

        lua.set_function("UIRect", [](const char* id, float x, float y, float w, float h, const Vec4& colour, sol::optional<float> radius)
                         { Lumos::UIRect(id, x, y, w, h, colour, radius.value_or(0.0f)); });
        lua.set_function("UIRing", [](const char* id, float x, float y, float radius, const Vec4& colour, sol::optional<float> thickness)
                         { Lumos::UIRing(id, x, y, radius, colour, thickness.value_or(1.0f)); });

        lua.set_function("UIColouredBox", sol::overload(
                             [](const char* id, float size, const Vec4& colour)
                             { return Lumos::UIColouredBox(id, size, size, colour, size * 0.5f); },
                             [](const char* id, float w, float h, const Vec4& colour, float radius)
                             { return Lumos::UIColouredBox(id, w, h, colour, radius); }));

        // Circular radar/minimap HUD - see Lumos::UIBeginRadar/UIRadarBlip/UIEndRadar.
        lua["UIBeginRadar"] = &Lumos::UIBeginRadar;
        lua["UIEndRadar"]   = &Lumos::UIEndRadar;
        lua.set_function("UIRadarBlip", [](const char* id, float x, float y, float size,
                                            const Vec4& tint, sol::optional<Graphics::Texture2D*> texture)
                          { return Lumos::UIRadarBlip(id, x, y, size, tint, texture.value_or(nullptr)); });
        lua.set_function("UIRadarRing", [](const char* id, float radius, const Vec4& colour,
                                            sol::optional<float> thickness)
                          { Lumos::UIRadarRing(id, radius, colour, thickness.value_or(1.0f)); });

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

        lua.set_function("UITextInput", [](const char* id) -> std::string
                         {
            static std::unordered_map<std::string, std::string> s_buffers;
            auto& buf = s_buffers[id];
            if(buf.size() < 256)
                buf.resize(256, '\0'); // fixed size => &buf[0] stays valid across frames
            Lumos::UITextInput(id, &buf[0], 256);
            return std::string(buf.c_str()); });
    }

}
