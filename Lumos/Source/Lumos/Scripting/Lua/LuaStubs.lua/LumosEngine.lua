-- AUTO-GENERATED: /Users/jmorton/Dev/Lumos-Dev/Lumos/Source/Lumos/Scripting/Lua/LuaStubs.lua/LumosEngine.lua
---@module Globals

pairs is a function
ElasticOut is a function
loadfile is a function
UIToggle is a function
next is a function
rawset is a function
AnimateToTarget is a function
ElasticIn is a function
rawequal is a function
Rand is a function
require is a function
UIBeginFrame is a function
SwitchSceneByName is a function
UIEndFrame is a function
UIEndPanel is a function
tonumber is a function
AddLightCubeEntity is a function
GetUIState is a function
GetStringSize is a function
sol.🔩 is a function
GetAppInstance is a function
rawlen is a function
UIButton is a function
UIPushStyle is a function
pcall is a function
ElasticInOut is a function
ExponentialIn is a function
ipairs is a function
UISlider is a function
UIImage is a function
UILabel is a function
UIBeginPanel is a function
AddPyramidEntity is a function
ShutDownUI is a function
InitialiseUI is a function
LoadMesh is a function
SineInOut is a function
ExitApp is a function
select is a function
xpcall is a function
SetB2DGravity is a function
UIPopStyle is a function
SetCallback is a function
LoadTextureWithParams is a function
SetPhysicsDebugFlags is a function
GetEntityByName is a function
setmetatable is a function
LoadTexture is a function
type is a function
SineOut is a function
assert is a function
ExponentialInOut is a function
UILayoutRoot is a function
load is a function
collectgarbage is a function
ExponentialOut is a function
rawget is a function
dofile is a function
AddSphereEntity is a function
SwitchScene is a function
print is a function
SineIn is a function
error is a function
tostring is a function
getmetatable is a function
SwitchSceneByIndex is a function
-- Module-level fields
---@field pairs fun(... )
---@field ElasticOut fun(... )
---@field loadfile fun(... )
---@field sol.Lumos::Scene.♻ any
---@field sol.Lumos::RigidBody2D.♻ any
---@field UIToggle fun(... )
---@field next fun(... )
---@field sol.Lumos::Maths::Matrix4.♻ any
---@field rawset fun(... )
---@field sol.Lumos::Maths::Vector2.♻ any
---@field AnimateToTarget fun(... )
---@field ElasticIn fun(... )
---@field sol.Lumos::UI_Interaction.♻ any
---@field sol.Lumos::Maths::Vector3.♻ any
---@field rawequal fun(... )
---@field Rand fun(... )
---@field require fun(... )
---@field sol.entt::basic_registry<>.♻ any
---@field UIBeginFrame fun(... )
---@field sol.Lumos::Graphics::Light.♻ any
---@field sol.Lumos::RigidBody3D.♻ any
---@field sol.Lumos::Graphics::Mesh.♻ any
---@field SwitchSceneByName fun(... )
---@field UIEndFrame fun(... )
---@field sol.Lumos::UI_Widget.♻ any
---@field UIEndPanel fun(... )
---@field tonumber fun(... )
---@field sol.Lumos::Maths::Transform.♻ any
---@field sol.☢☢ any
---@field sol.Lumos::Maths::Vector4.♻ any
---@field AddLightCubeEntity fun(... )
---@field sol.Lumos::Maths::Quaternion.♻ any
---@field GetUIState fun(... )
---@field GetStringSize fun(... )
---@field sol.ImVec2.♻ any
---@field sol.🔩 fun(... )
---@field GetAppInstance fun(... )
---@field sol.Lumos::LuaScriptComponent.♻ any
---@field sol.Lumos::RigidBody3DProperties.♻ any
---@field sol.Lumos::EntityManager.♻ any
---@field _VERSION string
---@field rawlen fun(... )
---@field UIButton fun(... )
---@field UIPushStyle fun(... )
---@field pcall fun(... )
---@field ElasticInOut fun(... )
---@field ExponentialIn fun(... )
---@field ipairs fun(... )
---@field UISlider fun(... )
---@field UIImage fun(... )
---@field UILabel fun(... )
---@field UIBeginPanel fun(... )
---@field AddPyramidEntity fun(... )
---@field ShutDownUI fun(... )
---@field InitialiseUI fun(... )
---@field LoadMesh fun(... )
---@field sol.Lumos::Maths::Matrix3.♻ any
---@field sol.Lumos::UI_Size.♻ any
---@field SineInOut fun(... )
---@field ExitApp fun(... )
---@field select fun(... )
---@field xpcall fun(... )
---@field SetB2DGravity fun(... )
---@field UIPopStyle fun(... )
---@field sol.Lumos::Graphics::Model.♻ any
---@field SetCallback fun(... )
---@field sol.Lumos::Entity.♻ any
---@field sol.ImGuiIO.♻ any
---@field sol.Lumos::RigidBodyParameters.♻ any
---@field sol.Lumos::TextComponent.♻ any
---@field sol.Lumos::Graphics::Texture2D.♻ any
---@field LoadTextureWithParams fun(... )
---@field sol.Lumos::RigidBody2DComponent.♻ any
---@field sol.Lumos::Graphics::Sprite.♻ any
---@field SetPhysicsDebugFlags fun(... )
---@field GetEntityByName fun(... )
---@field setmetatable fun(... )
---@field LoadTexture fun(... )
---@field sol.Lumos::Application.♻ any
---@field type fun(... )
---@field SineOut fun(... )
---@field sol.Lumos::RigidBody3DComponent.♻ any
---@field assert fun(... )
---@field ExponentialInOut fun(... )
---@field sol.Lumos::Camera.♻ any
---@field UILayoutRoot fun(... )
---@field load fun(... )
---@field collectgarbage fun(... )
---@field ExponentialOut fun(... )
---@field sol.Lumos::Graphics::Material.♻ any
---@field sol.Lumos::NameComponent.♻ any
---@field rawget fun(... )
---@field dofile fun(... )
---@field AddSphereEntity fun(... )
---@field SwitchScene fun(... )
---@field print fun(... )
---@field SineIn fun(... )
---@field sol.Lumos::ParticleEmitter.♻ any
---@field error fun(... )
---@field sol.Lumos::Graphics::AnimatedSprite.♻ any
---@field tostring fun(... )
---@field getmetatable fun(... )
---@field SwitchSceneByIndex fun(... )

-- Classes / types
---@class LumosEngine.base
local base = {}
---@field new fun(...):LumosEngine.base
---@field pairs fun(self:LumosEngine.base, ...)
---@field ElasticOut fun(self:LumosEngine.base, ...)
---@field loadfile fun(self:LumosEngine.base, ...)
---@field sol.Lumos::Scene.♻ any
---@field base any
---@field sol.Lumos::RigidBody2D.♻ any
---@field UIToggle fun(self:LumosEngine.base, ...)
---@field next fun(self:LumosEngine.base, ...)
---@field Quaternion any
---@field TextureFilter any
---@field Log any
---@field sol.Lumos::Maths::Matrix4.♻ any
---@field rawset fun(self:LumosEngine.base, ...)
---@field package any
---@field Matrix4 any
---@field sol.Lumos::Maths::Vector2.♻ any
---@field AnimateToTarget fun(self:LumosEngine.base, ...)
---@field ElasticIn fun(self:LumosEngine.base, ...)
---@field Key any
---@field sol.Lumos::UI_Interaction.♻ any
---@field AnimatedSprite_view any
---@field sol.Lumos::Maths::Vector3.♻ any
---@field Transform_view any
---@field Matrix3 any
---@field rawequal fun(self:LumosEngine.base, ...)
---@field Texture2D any
---@field Rand fun(self:LumosEngine.base, ...)
---@field require fun(self:LumosEngine.base, ...)
---@field AnimatedSprite any
---@field ParticleEmitter_view any
---@field sol.entt::basic_registry<>.♻ any
---@field ParticleEmitter any
---@field UIBeginFrame fun(self:LumosEngine.base, ...)
---@field sol.Lumos::Graphics::Light.♻ any
---@field sol.Lumos::RigidBody3D.♻ any
---@field sol.Lumos::Graphics::Mesh.♻ any
---@field SwitchSceneByName fun(self:LumosEngine.base, ...)
---@field UIEndFrame fun(self:LumosEngine.base, ...)
---@field Sprite any
---@field sol.Lumos::UI_Widget.♻ any
---@field RigidBodyParameters any
---@field Application any
---@field TextComponent any
---@field Shape any
---@field Material any
---@field UIEndPanel fun(self:LumosEngine.base, ...)
---@field tonumber fun(self:LumosEngine.base, ...)
---@field sol.Lumos::Maths::Transform.♻ any
---@field sol.☢☢ any
---@field sol.Lumos::Maths::Vector4.♻ any
---@field AddLightCubeEntity fun(self:LumosEngine.base, ...)
---@field sol.Lumos::Maths::Quaternion.♻ any
---@field GetUIState fun(self:LumosEngine.base, ...)
---@field GetStringSize fun(self:LumosEngine.base, ...)
---@field RigidBody2DComponent_view any
---@field NameComponent any
---@field sol.ImVec2.♻ any
---@field sol.🔩 fun(self:LumosEngine.base, ...)
---@field GetAppInstance fun(self:LumosEngine.base, ...)
---@field RigidBody3D any
---@field sol.Lumos::LuaScriptComponent.♻ any
---@field Entity any
---@field sol.Lumos::RigidBody3DProperties.♻ any
---@field sol.Lumos::EntityManager.♻ any
---@field _VERSION any
---@field rawlen fun(self:LumosEngine.base, ...)
---@field UIButton fun(self:LumosEngine.base, ...)
---@field Light any
---@field tracy any
---@field UIPushStyle fun(self:LumosEngine.base, ...)
---@field pcall fun(self:LumosEngine.base, ...)
---@field debug any
---@field ElasticInOut fun(self:LumosEngine.base, ...)
---@field ExponentialIn fun(self:LumosEngine.base, ...)
---@field TextComponent_view any
---@field ipairs fun(self:LumosEngine.base, ...)
---@field UISlider fun(self:LumosEngine.base, ...)
---@field UIImage fun(self:LumosEngine.base, ...)
---@field UILabel fun(self:LumosEngine.base, ...)
---@field Model_view any
---@field Vector2 any
---@field UITextAlignment any
---@field UIBeginPanel fun(self:LumosEngine.base, ...)
---@field AddPyramidEntity fun(self:LumosEngine.base, ...)
---@field ShutDownUI fun(self:LumosEngine.base, ...)
---@field InitialiseUI fun(self:LumosEngine.base, ...)
---@field enttRegistry any
---@field LoadMesh fun(self:LumosEngine.base, ...)
---@field UI_Interaction any
---@field sol.Lumos::Maths::Matrix3.♻ any
---@field UI_Widget any
---@field UI_Size any
---@field sol.Lumos::UI_Size.♻ any
---@field StyleVar any
---@field SineInOut fun(self:LumosEngine.base, ...)
---@field LuaScriptComponent any
---@field ExitApp fun(self:LumosEngine.base, ...)
---@field select fun(self:LumosEngine.base, ...)
---@field xpcall fun(self:LumosEngine.base, ...)
---@field SetB2DGravity fun(self:LumosEngine.base, ...)
---@field SizeKind any
---@field UIPopStyle fun(self:LumosEngine.base, ...)
---@field WidgetFlags any
---@field sol.Lumos::Graphics::Model.♻ any
---@field SetCallback fun(self:LumosEngine.base, ...)
---@field sol.Lumos::Entity.♻ any
---@field Scene any
---@field sol.ImGuiIO.♻ any
---@field RigidBody2D any
---@field Camera_view any
---@field RigidBodyParameters3D any
---@field sol.Lumos::RigidBodyParameters.♻ any
---@field sol.Lumos::TextComponent.♻ any
---@field sol.Lumos::Graphics::Texture2D.♻ any
---@field LoadTextureWithParams fun(self:LumosEngine.base, ...)
---@field sol.Lumos::RigidBody2DComponent.♻ any
---@field EntityManager any
---@field sol.Lumos::Graphics::Sprite.♻ any
---@field TextureWrap any
---@field ParticleAlignedType any
---@field UIAxis any
---@field SoundComponent_view any
---@field _G any
---@field SetPhysicsDebugFlags fun(self:LumosEngine.base, ...)
---@field RigidBody2DComponent any
---@field GetEntityByName fun(self:LumosEngine.base, ...)
---@field setmetatable fun(self:LumosEngine.base, ...)
---@field ParticleBlendType any
---@field LoadTexture fun(self:LumosEngine.base, ...)
---@field RigidBody3DComponent_view any
---@field sol.Lumos::Application.♻ any
---@field MouseButton any
---@field type fun(self:LumosEngine.base, ...)
---@field Light_view any
---@field SineOut fun(self:LumosEngine.base, ...)
---@field sol.Lumos::RigidBody3DComponent.♻ any
---@field Sprite_view any
---@field Transform any
---@field Camera any
---@field assert fun(self:LumosEngine.base, ...)
---@field ExponentialInOut fun(self:LumosEngine.base, ...)
---@field sol.Lumos::Camera.♻ any
---@field UILayoutRoot fun(self:LumosEngine.base, ...)
---@field table any
---@field load fun(self:LumosEngine.base, ...)
---@field collectgarbage fun(self:LumosEngine.base, ...)
---@field ExponentialOut fun(self:LumosEngine.base, ...)
---@field RenderFlags any
---@field sol.Lumos::Graphics::Material.♻ any
---@field math any
---@field PrimitiveType any
---@field NameComponent_view any
---@field sol.Lumos::NameComponent.♻ any
---@field Model any
---@field rawget fun(self:LumosEngine.base, ...)
---@field LuaScriptComponent_view any
---@field dofile fun(self:LumosEngine.base, ...)
---@field AddSphereEntity fun(self:LumosEngine.base, ...)
---@field SwitchScene fun(self:LumosEngine.base, ...)
---@field print fun(self:LumosEngine.base, ...)
---@field Vector3 any
---@field SineIn fun(self:LumosEngine.base, ...)
---@field sol.Lumos::ParticleEmitter.♻ any
---@field RigidBody3DComponent any
---@field error fun(self:LumosEngine.base, ...)
---@field Vector4 any
---@field sol.Lumos::Graphics::AnimatedSprite.♻ any
---@field tostring fun(self:LumosEngine.base, ...)
---@field string any
---@field PhysicsDebugFlags any
---@field getmetatable fun(self:LumosEngine.base, ...)
---@field os any
---@field gui any
---@field SwitchSceneByIndex fun(self:LumosEngine.base, ...)
---@field Input any
---@field Mesh any

---@class LumosEngine.Quaternion
local Quaternion = {}
---@field new fun(...):LumosEngine.Quaternion
---@field __name any

---@class LumosEngine.TextureFilter
local TextureFilter = {}
---@field new fun(...):LumosEngine.TextureFilter
---@field None any
---@field Nearest any
---@field Linear any

---@class LumosEngine.Log
local Log = {}
---@field new fun(...):LumosEngine.Log
---@field Info fun(self:LumosEngine.Log, ...)
---@field Error fun(self:LumosEngine.Log, ...)
---@field Warn fun(self:LumosEngine.Log, ...)
---@field Trace fun(self:LumosEngine.Log, ...)
---@field FATAL fun(self:LumosEngine.Log, ...)

---@class LumosEngine.package
local package = {}
---@field new fun(...):LumosEngine.package
---@field loadlib fun(self:LumosEngine.package, ...)
---@field cpath any
---@field config any
---@field loaded any
---@field path any
---@field searchers any
---@field preload any
---@field searchpath fun(self:LumosEngine.package, ...)

---@class LumosEngine.Matrix4
local Matrix4 = {}
---@field new fun(...):LumosEngine.Matrix4
---@field __name any

---@class LumosEngine.Key
local Key = {}
---@field new fun(...):LumosEngine.Key
---@field F11 any
---@field F6 any
---@field Keypad4 any
---@field Menu any
---@field PageDown any
---@field Keypad9 any
---@field F10 any
---@field Keypad0 any
---@field Backspace any
---@field Tab any
---@field Subtract any
---@field PrintScreen any
---@field F5 any
---@field PERIOD any
---@field RightShift any
---@field X any
---@field W any
---@field Z any
---@field Y any
---@field T any
---@field S any
---@field V any
---@field U any
---@field LeftShift any
---@field Multiply any
---@field Comma any
---@field KP_EQUAL any
---@field NumLock any
---@field LEFT_SUPER any
---@field Left any
---@field G any
---@field J any
---@field I any
---@field D any
---@field C any
---@field BACKSLASH any
---@field SEMICOLON any
---@field P any
---@field O any
---@field Pasue any
---@field Q any
---@field L any
---@field K any
---@field N any
---@field RIGHT_ALT any
---@field Add any
---@field Insert any
---@field EQUAL any
---@field Divide any
---@field Keypad7 any
---@field Keypad8 any
---@field Decimal any
---@field Keypad3 any
---@field SCROLL_LOCK any
---@field Keypad6 any
---@field B any
---@field PageUp any
---@field Home any
---@field Keypad5 any
---@field Escape any
---@field RIGHT_BRACKET any
---@field Delete any
---@field SLASH any
---@field F1 any
---@field Down any
---@field Keypad1 any
---@field F3 any
---@field F8 any
---@field F9 any
---@field LEFT_ALT any
---@field F12 any
---@field F7 any
---@field F4 any
---@field F2 any
---@field Keypad2 any
---@field H any
---@field MINUS any
---@field F any
---@field E any
---@field RightControl any
---@field R any
---@field M any
---@field Space any
---@field Enter any
---@field APOSTROPHE any
---@field Right any
---@field End any
---@field LeftControl any
---@field A any
---@field Up any
---@field RIGHT_SUPER any
---@field LEFT_BRACKET any
---@field CAPS_LOCK any

---@class LumosEngine.AnimatedSprite_view
local AnimatedSprite_view = {}
---@field new fun(...):LumosEngine.AnimatedSprite_view
---@field __name any

---@class LumosEngine.Transform_view
local Transform_view = {}
---@field new fun(...):LumosEngine.Transform_view
---@field __name any

---@class LumosEngine.Matrix3
local Matrix3 = {}
---@field new fun(...):LumosEngine.Matrix3
---@field __name any

---@class LumosEngine.Texture2D
local Texture2D = {}
---@field new fun(...):LumosEngine.Texture2D
---@field __name any

---@class LumosEngine.AnimatedSprite
local AnimatedSprite = {}
---@field new fun(...):LumosEngine.AnimatedSprite
---@field __name any

---@class LumosEngine.ParticleEmitter_view
local ParticleEmitter_view = {}
---@field new fun(...):LumosEngine.ParticleEmitter_view
---@field __name any

---@class LumosEngine.ParticleEmitter
local ParticleEmitter = {}
---@field new fun(...):LumosEngine.ParticleEmitter
---@field __name any

---@class LumosEngine.Sprite
local Sprite = {}
---@field new fun(...):LumosEngine.Sprite
---@field __name any

---@class LumosEngine.RigidBodyParameters
local RigidBodyParameters = {}
---@field new fun(...):LumosEngine.RigidBodyParameters
---@field __name any

---@class LumosEngine.Application
local Application = {}
---@field new fun(...):LumosEngine.Application
---@field __name any

---@class LumosEngine.TextComponent
local TextComponent = {}
---@field new fun(...):LumosEngine.TextComponent
---@field __name any

---@class LumosEngine.Shape
local Shape = {}
---@field new fun(...):LumosEngine.Shape
---@field Circle any
---@field Square any
---@field Custom any

---@class LumosEngine.Material
local Material = {}
---@field new fun(...):LumosEngine.Material
---@field __name any

---@class LumosEngine.RigidBody2DComponent_view
local RigidBody2DComponent_view = {}
---@field new fun(...):LumosEngine.RigidBody2DComponent_view
---@field __name any

---@class LumosEngine.NameComponent
local NameComponent = {}
---@field new fun(...):LumosEngine.NameComponent
---@field __name any

---@class LumosEngine.RigidBody3D
local RigidBody3D = {}
---@field new fun(...):LumosEngine.RigidBody3D
---@field __name any

---@class LumosEngine.Entity
local Entity = {}
---@field new fun(...):LumosEngine.Entity
---@field __name any

---@class LumosEngine.Light
local Light = {}
---@field new fun(...):LumosEngine.Light
---@field __name any

---@class LumosEngine.tracy
local tracy = {}
---@field new fun(...):LumosEngine.tracy
---@field Message fun(self:LumosEngine.tracy, ...)
---@field ZoneBeginS fun(self:LumosEngine.tracy, ...)
---@field ZoneName fun(self:LumosEngine.tracy, ...)
---@field ZoneEnd fun(self:LumosEngine.tracy, ...)
---@field ZoneText fun(self:LumosEngine.tracy, ...)
---@field ZoneBeginN fun(self:LumosEngine.tracy, ...)
---@field ZoneBeginNS fun(self:LumosEngine.tracy, ...)
---@field ZoneBegin fun(self:LumosEngine.tracy, ...)

---@class LumosEngine.debug
local debug = {}
---@field new fun(...):LumosEngine.debug
---@field traceback fun(self:LumosEngine.debug, ...)
---@field setupvalue fun(self:LumosEngine.debug, ...)
---@field setmetatable fun(self:LumosEngine.debug, ...)
---@field sethook fun(self:LumosEngine.debug, ...)
---@field setlocal fun(self:LumosEngine.debug, ...)
---@field getmetatable fun(self:LumosEngine.debug, ...)
---@field setuservalue fun(self:LumosEngine.debug, ...)
---@field gethook fun(self:LumosEngine.debug, ...)
---@field getlocal fun(self:LumosEngine.debug, ...)
---@field getupvalue fun(self:LumosEngine.debug, ...)
---@field debug fun(self:LumosEngine.debug, ...)
---@field getinfo fun(self:LumosEngine.debug, ...)
---@field getuservalue fun(self:LumosEngine.debug, ...)
---@field upvaluejoin fun(self:LumosEngine.debug, ...)
---@field getregistry fun(self:LumosEngine.debug, ...)
---@field upvalueid fun(self:LumosEngine.debug, ...)

---@class LumosEngine.TextComponent_view
local TextComponent_view = {}
---@field new fun(...):LumosEngine.TextComponent_view
---@field __name any

---@class LumosEngine.Model_view
local Model_view = {}
---@field new fun(...):LumosEngine.Model_view
---@field __name any

---@class LumosEngine.Vector2
local Vector2 = {}
---@field new fun(...):LumosEngine.Vector2
---@field __name any

---@class LumosEngine.UITextAlignment
local UITextAlignment = {}
---@field new fun(...):LumosEngine.UITextAlignment

---@class LumosEngine.enttRegistry
local enttRegistry = {}
---@field new fun(...):LumosEngine.enttRegistry
---@field __name any

---@class LumosEngine.UI_Interaction
local UI_Interaction = {}
---@field new fun(...):LumosEngine.UI_Interaction
---@field __name any

---@class LumosEngine.UI_Widget
local UI_Widget = {}
---@field new fun(...):LumosEngine.UI_Widget
---@field __name any

---@class LumosEngine.UI_Size
local UI_Size = {}
---@field new fun(...):LumosEngine.UI_Size
---@field __name any

---@class LumosEngine.StyleVar
local StyleVar = {}
---@field new fun(...):LumosEngine.StyleVar

---@class LumosEngine.LuaScriptComponent
local LuaScriptComponent = {}
---@field new fun(...):LumosEngine.LuaScriptComponent
---@field __name any

---@class LumosEngine.SizeKind
local SizeKind = {}
---@field new fun(...):LumosEngine.SizeKind

---@class LumosEngine.WidgetFlags
local WidgetFlags = {}
---@field new fun(...):LumosEngine.WidgetFlags

---@class LumosEngine.Scene
local Scene = {}
---@field new fun(...):LumosEngine.Scene
---@field __name any

---@class LumosEngine.RigidBody2D
local RigidBody2D = {}
---@field new fun(...):LumosEngine.RigidBody2D
---@field __name any

---@class LumosEngine.Camera_view
local Camera_view = {}
---@field new fun(...):LumosEngine.Camera_view
---@field __name any

---@class LumosEngine.RigidBodyParameters3D
local RigidBodyParameters3D = {}
---@field new fun(...):LumosEngine.RigidBodyParameters3D
---@field __name any

---@class LumosEngine.EntityManager
local EntityManager = {}
---@field new fun(...):LumosEngine.EntityManager
---@field __name any

---@class LumosEngine.TextureWrap
local TextureWrap = {}
---@field new fun(...):LumosEngine.TextureWrap
---@field ClampToEdge any
---@field Repeat any
---@field ClampToBorder any
---@field None any
---@field Clamp any
---@field MirroredRepeat any

---@class LumosEngine.ParticleAlignedType
local ParticleAlignedType = {}
---@field new fun(...):LumosEngine.ParticleAlignedType
---@field Aligned3D any
---@field Aligned2D any
---@field None any

---@class LumosEngine.UIAxis
local UIAxis = {}
---@field new fun(...):LumosEngine.UIAxis

---@class LumosEngine.SoundComponent_view
local SoundComponent_view = {}
---@field new fun(...):LumosEngine.SoundComponent_view
---@field __name any

---@class LumosEngine._G
local _G = {}
---@field new fun(...):LumosEngine._G
---@field pairs fun(self:LumosEngine._G, ...)
---@field ElasticOut fun(self:LumosEngine._G, ...)
---@field loadfile fun(self:LumosEngine._G, ...)
---@field sol.Lumos::Scene.♻ any
---@field base any
---@field sol.Lumos::RigidBody2D.♻ any
---@field UIToggle fun(self:LumosEngine._G, ...)
---@field next fun(self:LumosEngine._G, ...)
---@field Quaternion any
---@field TextureFilter any
---@field Log any
---@field sol.Lumos::Maths::Matrix4.♻ any
---@field rawset fun(self:LumosEngine._G, ...)
---@field package any
---@field Matrix4 any
---@field sol.Lumos::Maths::Vector2.♻ any
---@field AnimateToTarget fun(self:LumosEngine._G, ...)
---@field ElasticIn fun(self:LumosEngine._G, ...)
---@field Key any
---@field sol.Lumos::UI_Interaction.♻ any
---@field AnimatedSprite_view any
---@field sol.Lumos::Maths::Vector3.♻ any
---@field Transform_view any
---@field Matrix3 any
---@field rawequal fun(self:LumosEngine._G, ...)
---@field Texture2D any
---@field Rand fun(self:LumosEngine._G, ...)
---@field require fun(self:LumosEngine._G, ...)
---@field AnimatedSprite any
---@field ParticleEmitter_view any
---@field sol.entt::basic_registry<>.♻ any
---@field ParticleEmitter any
---@field UIBeginFrame fun(self:LumosEngine._G, ...)
---@field sol.Lumos::Graphics::Light.♻ any
---@field sol.Lumos::RigidBody3D.♻ any
---@field sol.Lumos::Graphics::Mesh.♻ any
---@field SwitchSceneByName fun(self:LumosEngine._G, ...)
---@field UIEndFrame fun(self:LumosEngine._G, ...)
---@field Sprite any
---@field sol.Lumos::UI_Widget.♻ any
---@field RigidBodyParameters any
---@field Application any
---@field TextComponent any
---@field Shape any
---@field Material any
---@field UIEndPanel fun(self:LumosEngine._G, ...)
---@field tonumber fun(self:LumosEngine._G, ...)
---@field sol.Lumos::Maths::Transform.♻ any
---@field sol.☢☢ any
---@field sol.Lumos::Maths::Vector4.♻ any
---@field AddLightCubeEntity fun(self:LumosEngine._G, ...)
---@field sol.Lumos::Maths::Quaternion.♻ any
---@field GetUIState fun(self:LumosEngine._G, ...)
---@field GetStringSize fun(self:LumosEngine._G, ...)
---@field RigidBody2DComponent_view any
---@field NameComponent any
---@field sol.ImVec2.♻ any
---@field sol.🔩 fun(self:LumosEngine._G, ...)
---@field GetAppInstance fun(self:LumosEngine._G, ...)
---@field RigidBody3D any
---@field sol.Lumos::LuaScriptComponent.♻ any
---@field Entity any
---@field sol.Lumos::RigidBody3DProperties.♻ any
---@field sol.Lumos::EntityManager.♻ any
---@field _VERSION any
---@field rawlen fun(self:LumosEngine._G, ...)
---@field UIButton fun(self:LumosEngine._G, ...)
---@field Light any
---@field tracy any
---@field UIPushStyle fun(self:LumosEngine._G, ...)
---@field pcall fun(self:LumosEngine._G, ...)
---@field debug any
---@field ElasticInOut fun(self:LumosEngine._G, ...)
---@field ExponentialIn fun(self:LumosEngine._G, ...)
---@field TextComponent_view any
---@field ipairs fun(self:LumosEngine._G, ...)
---@field UISlider fun(self:LumosEngine._G, ...)
---@field UIImage fun(self:LumosEngine._G, ...)
---@field UILabel fun(self:LumosEngine._G, ...)
---@field Model_view any
---@field Vector2 any
---@field UITextAlignment any
---@field UIBeginPanel fun(self:LumosEngine._G, ...)
---@field AddPyramidEntity fun(self:LumosEngine._G, ...)
---@field ShutDownUI fun(self:LumosEngine._G, ...)
---@field InitialiseUI fun(self:LumosEngine._G, ...)
---@field enttRegistry any
---@field LoadMesh fun(self:LumosEngine._G, ...)
---@field UI_Interaction any
---@field sol.Lumos::Maths::Matrix3.♻ any
---@field UI_Widget any
---@field UI_Size any
---@field sol.Lumos::UI_Size.♻ any
---@field StyleVar any
---@field SineInOut fun(self:LumosEngine._G, ...)
---@field LuaScriptComponent any
---@field ExitApp fun(self:LumosEngine._G, ...)
---@field select fun(self:LumosEngine._G, ...)
---@field xpcall fun(self:LumosEngine._G, ...)
---@field SetB2DGravity fun(self:LumosEngine._G, ...)
---@field SizeKind any
---@field UIPopStyle fun(self:LumosEngine._G, ...)
---@field WidgetFlags any
---@field sol.Lumos::Graphics::Model.♻ any
---@field SetCallback fun(self:LumosEngine._G, ...)
---@field sol.Lumos::Entity.♻ any
---@field Scene any
---@field sol.ImGuiIO.♻ any
---@field RigidBody2D any
---@field Camera_view any
---@field RigidBodyParameters3D any
---@field sol.Lumos::RigidBodyParameters.♻ any
---@field sol.Lumos::TextComponent.♻ any
---@field sol.Lumos::Graphics::Texture2D.♻ any
---@field LoadTextureWithParams fun(self:LumosEngine._G, ...)
---@field sol.Lumos::RigidBody2DComponent.♻ any
---@field EntityManager any
---@field sol.Lumos::Graphics::Sprite.♻ any
---@field TextureWrap any
---@field ParticleAlignedType any
---@field UIAxis any
---@field SoundComponent_view any
---@field _G any
---@field SetPhysicsDebugFlags fun(self:LumosEngine._G, ...)
---@field RigidBody2DComponent any
---@field GetEntityByName fun(self:LumosEngine._G, ...)
---@field setmetatable fun(self:LumosEngine._G, ...)
---@field ParticleBlendType any
---@field LoadTexture fun(self:LumosEngine._G, ...)
---@field RigidBody3DComponent_view any
---@field sol.Lumos::Application.♻ any
---@field MouseButton any
---@field type fun(self:LumosEngine._G, ...)
---@field Light_view any
---@field SineOut fun(self:LumosEngine._G, ...)
---@field sol.Lumos::RigidBody3DComponent.♻ any
---@field Sprite_view any
---@field Transform any
---@field Camera any
---@field assert fun(self:LumosEngine._G, ...)
---@field ExponentialInOut fun(self:LumosEngine._G, ...)
---@field sol.Lumos::Camera.♻ any
---@field UILayoutRoot fun(self:LumosEngine._G, ...)
---@field table any
---@field load fun(self:LumosEngine._G, ...)
---@field collectgarbage fun(self:LumosEngine._G, ...)
---@field ExponentialOut fun(self:LumosEngine._G, ...)
---@field RenderFlags any
---@field sol.Lumos::Graphics::Material.♻ any
---@field math any
---@field PrimitiveType any
---@field NameComponent_view any
---@field sol.Lumos::NameComponent.♻ any
---@field Model any
---@field rawget fun(self:LumosEngine._G, ...)
---@field LuaScriptComponent_view any
---@field dofile fun(self:LumosEngine._G, ...)
---@field AddSphereEntity fun(self:LumosEngine._G, ...)
---@field SwitchScene fun(self:LumosEngine._G, ...)
---@field print fun(self:LumosEngine._G, ...)
---@field Vector3 any
---@field SineIn fun(self:LumosEngine._G, ...)
---@field sol.Lumos::ParticleEmitter.♻ any
---@field RigidBody3DComponent any
---@field error fun(self:LumosEngine._G, ...)
---@field Vector4 any
---@field sol.Lumos::Graphics::AnimatedSprite.♻ any
---@field tostring fun(self:LumosEngine._G, ...)
---@field string any
---@field PhysicsDebugFlags any
---@field getmetatable fun(self:LumosEngine._G, ...)
---@field os any
---@field gui any
---@field SwitchSceneByIndex fun(self:LumosEngine._G, ...)
---@field Input any
---@field Mesh any

---@class LumosEngine.RigidBody2DComponent
local RigidBody2DComponent = {}
---@field new fun(...):LumosEngine.RigidBody2DComponent
---@field __name any

---@class LumosEngine.ParticleBlendType
local ParticleBlendType = {}
---@field new fun(...):LumosEngine.ParticleBlendType
---@field Alpha any
---@field Additive any
---@field Off any

---@class LumosEngine.RigidBody3DComponent_view
local RigidBody3DComponent_view = {}
---@field new fun(...):LumosEngine.RigidBody3DComponent_view
---@field __name any

---@class LumosEngine.MouseButton
local MouseButton = {}
---@field new fun(...):LumosEngine.MouseButton
---@field Right any
---@field Middle any
---@field Left any

---@class LumosEngine.Light_view
local Light_view = {}
---@field new fun(...):LumosEngine.Light_view
---@field __name any

---@class LumosEngine.Sprite_view
local Sprite_view = {}
---@field new fun(...):LumosEngine.Sprite_view
---@field __name any

---@class LumosEngine.Transform
local Transform = {}
---@field new fun(...):LumosEngine.Transform
---@field __name any

---@class LumosEngine.Camera
local Camera = {}
---@field new fun(...):LumosEngine.Camera
---@field __name any

---@class LumosEngine.table
local table = {}
---@field new fun(...):LumosEngine.table
---@field insert fun(self:LumosEngine.table, ...)
---@field move fun(self:LumosEngine.table, ...)
---@field concat fun(self:LumosEngine.table, ...)
---@field sort fun(self:LumosEngine.table, ...)
---@field remove fun(self:LumosEngine.table, ...)
---@field pack fun(self:LumosEngine.table, ...)
---@field unpack fun(self:LumosEngine.table, ...)

---@class LumosEngine.RenderFlags
local RenderFlags = {}
---@field new fun(...):LumosEngine.RenderFlags
---@field DEPTHTEST any
---@field WIREFRAME any
---@field DEFERREDRENDER any
---@field ALPHABLEND any
---@field TWOSIDED any
---@field FORWARDRENDER any
---@field NONE any
---@field NOSHADOW any

---@class LumosEngine.math
local math = {}
---@field new fun(...):LumosEngine.math
---@field frexp fun(self:LumosEngine.math, ...)
---@field huge any
---@field cos fun(self:LumosEngine.math, ...)
---@field maxinteger any
---@field mininteger any
---@field type fun(self:LumosEngine.math, ...)
---@field abs fun(self:LumosEngine.math, ...)
---@field sin fun(self:LumosEngine.math, ...)
---@field tointeger fun(self:LumosEngine.math, ...)
---@field ldexp fun(self:LumosEngine.math, ...)
---@field tan fun(self:LumosEngine.math, ...)
---@field fmod fun(self:LumosEngine.math, ...)
---@field floor fun(self:LumosEngine.math, ...)
---@field rad fun(self:LumosEngine.math, ...)
---@field ceil fun(self:LumosEngine.math, ...)
---@field acos fun(self:LumosEngine.math, ...)
---@field pi any
---@field max fun(self:LumosEngine.math, ...)
---@field randomseed fun(self:LumosEngine.math, ...)
---@field atan fun(self:LumosEngine.math, ...)
---@field log10 fun(self:LumosEngine.math, ...)
---@field tanh fun(self:LumosEngine.math, ...)
---@field sinh fun(self:LumosEngine.math, ...)
---@field cosh fun(self:LumosEngine.math, ...)
---@field atan2 fun(self:LumosEngine.math, ...)
---@field sqrt fun(self:LumosEngine.math, ...)
---@field random fun(self:LumosEngine.math, ...)
---@field modf fun(self:LumosEngine.math, ...)
---@field log fun(self:LumosEngine.math, ...)
---@field min fun(self:LumosEngine.math, ...)
---@field exp fun(self:LumosEngine.math, ...)
---@field pow fun(self:LumosEngine.math, ...)
---@field deg fun(self:LumosEngine.math, ...)
---@field asin fun(self:LumosEngine.math, ...)
---@field ult fun(self:LumosEngine.math, ...)

---@class LumosEngine.PrimitiveType
local PrimitiveType = {}
---@field new fun(...):LumosEngine.PrimitiveType
---@field Pyramid any
---@field Terrain any
---@field Cylinder any
---@field Cube any
---@field Plane any
---@field Quad any
---@field Sphere any
---@field Capsule any

---@class LumosEngine.NameComponent_view
local NameComponent_view = {}
---@field new fun(...):LumosEngine.NameComponent_view
---@field __name any

---@class LumosEngine.Model
local Model = {}
---@field new fun(...):LumosEngine.Model
---@field __name any

---@class LumosEngine.LuaScriptComponent_view
local LuaScriptComponent_view = {}
---@field new fun(...):LumosEngine.LuaScriptComponent_view
---@field __name any

---@class LumosEngine.Vector3
local Vector3 = {}
---@field new fun(...):LumosEngine.Vector3
---@field __name any

---@class LumosEngine.RigidBody3DComponent
local RigidBody3DComponent = {}
---@field new fun(...):LumosEngine.RigidBody3DComponent
---@field __name any

---@class LumosEngine.Vector4
local Vector4 = {}
---@field new fun(...):LumosEngine.Vector4
---@field __name any

---@class LumosEngine.string
local string = {}
---@field new fun(...):LumosEngine.string
---@field find fun(self:LumosEngine.string, ...)
---@field rep fun(self:LumosEngine.string, ...)
---@field pack fun(self:LumosEngine.string, ...)
---@field reverse fun(self:LumosEngine.string, ...)
---@field gmatch fun(self:LumosEngine.string, ...)
---@field char fun(self:LumosEngine.string, ...)
---@field unpack fun(self:LumosEngine.string, ...)
---@field dump fun(self:LumosEngine.string, ...)
---@field upper fun(self:LumosEngine.string, ...)
---@field match fun(self:LumosEngine.string, ...)
---@field gsub fun(self:LumosEngine.string, ...)
---@field sub fun(self:LumosEngine.string, ...)
---@field byte fun(self:LumosEngine.string, ...)
---@field lower fun(self:LumosEngine.string, ...)
---@field len fun(self:LumosEngine.string, ...)
---@field format fun(self:LumosEngine.string, ...)
---@field packsize fun(self:LumosEngine.string, ...)

---@class LumosEngine.PhysicsDebugFlags
local PhysicsDebugFlags = {}
---@field new fun(...):LumosEngine.PhysicsDebugFlags
---@field LINEARFORCE any
---@field COLLISIONVOLUMES any
---@field COLLISIONNORMALS any
---@field MANIFOLD any
---@field CONSTRAINT any
---@field BROADPHASE_PAIRS any
---@field LINEARVELOCITY any
---@field AABB any
---@field BOUNDING_RADIUS any
---@field BROADPHASE any

---@class LumosEngine.os
local os = {}
---@field new fun(...):LumosEngine.os
---@field setlocale fun(self:LumosEngine.os, ...)
---@field exit fun(self:LumosEngine.os, ...)
---@field execute fun(self:LumosEngine.os, ...)
---@field date fun(self:LumosEngine.os, ...)
---@field clock fun(self:LumosEngine.os, ...)
---@field difftime fun(self:LumosEngine.os, ...)
---@field remove fun(self:LumosEngine.os, ...)
---@field tmpname fun(self:LumosEngine.os, ...)
---@field time fun(self:LumosEngine.os, ...)
---@field rename fun(self:LumosEngine.os, ...)
---@field getenv fun(self:LumosEngine.os, ...)

---@class LumosEngine.gui
local gui = {}
---@field new fun(...):LumosEngine.gui
---@field getID fun(self:LumosEngine.gui, ...)
---@field endCombo fun(self:LumosEngine.gui, ...)
---@field ConfigFlags any
---@field popFont fun(self:LumosEngine.gui, ...)
---@field ComboFlags any
---@field setWindowCollapsed fun(self:LumosEngine.gui, ...)
---@field dummy fun(self:LumosEngine.gui, ...)
---@field getFontSize fun(self:LumosEngine.gui, ...)
---@field ItemFlags any
---@field NavInput any
---@field textWrapped fun(self:LumosEngine.gui, ...)
---@field getWindowPos fun(self:LumosEngine.gui, ...)
---@field beginGroup fun(self:LumosEngine.gui, ...)
---@field popItemWidth fun(self:LumosEngine.gui, ...)
---@field getContentRegionAvailWidth fun(self:LumosEngine.gui, ...)
---@field getDrawData fun(self:LumosEngine.gui, ...)
---@field getCursorStartPos fun(self:LumosEngine.gui, ...)
---@field setWindowPos fun(self:LumosEngine.gui, ...)
---@field setWindowFontScale fun(self:LumosEngine.gui, ...)
---@field showAboutWindow fun(self:LumosEngine.gui, ...)
---@field beginPopupContextWindow fun(self:LumosEngine.gui, ...)
---@field Key any
---@field getScrollMaxY fun(self:LumosEngine.gui, ...)
---@field getScrollX fun(self:LumosEngine.gui, ...)
---@field isPopupOpen fun(self:LumosEngine.gui, ...)
---@field endWindow fun(self:LumosEngine.gui, ...)
---@field getCursorPosY fun(self:LumosEngine.gui, ...)
---@field getCursorPos fun(self:LumosEngine.gui, ...)
---@field colourEdit4 fun(self:LumosEngine.gui, ...)
---@field treePop fun(self:LumosEngine.gui, ...)
---@field DataType any
---@field treeNodeEx fun(self:LumosEngine.gui, ...)
---@field endTooltip fun(self:LumosEngine.gui, ...)
---@field getStyle fun(self:LumosEngine.gui, ...)
---@field setScrollHereY fun(self:LumosEngine.gui, ...)
---@field setCursorPosY fun(self:LumosEngine.gui, ...)
---@field spacing fun(self:LumosEngine.gui, ...)
---@field getScrollMaxX fun(self:LumosEngine.gui, ...)
---@field BackendFlags any
---@field progressBar fun(self:LumosEngine.gui, ...)
---@field popStyleVar fun(self:LumosEngine.gui, ...)
---@field newFrame fun(self:LumosEngine.gui, ...)
---@field setNextWindowContentSize fun(self:LumosEngine.gui, ...)
---@field isWindowAppearing fun(self:LumosEngine.gui, ...)
---@field getFont fun(self:LumosEngine.gui, ...)
---@field endFrame fun(self:LumosEngine.gui, ...)
---@field showStyleEditor fun(self:LumosEngine.gui, ...)
---@field bullet fun(self:LumosEngine.gui, ...)
---@field getIO fun(self:LumosEngine.gui, ...)
---@field beginPopup fun(self:LumosEngine.gui, ...)
---@field calcItemWidth fun(self:LumosEngine.gui, ...)
---@field setNextWindowBgAlpha fun(self:LumosEngine.gui, ...)
---@field beginWindow fun(self:LumosEngine.gui, ...)
---@field setScrollFromPosY fun(self:LumosEngine.gui, ...)
---@field setNextWindowSize fun(self:LumosEngine.gui, ...)
---@field Cond any
---@field getCursorScreenPos fun(self:LumosEngine.gui, ...)
---@field setColumnOffset fun(self:LumosEngine.gui, ...)
---@field showMetricsWindow fun(self:LumosEngine.gui, ...)
---@field setNextWindowCollapsed fun(self:LumosEngine.gui, ...)
---@field checkbox fun(self:LumosEngine.gui, ...)
---@field setNextWindowFocus fun(self:LumosEngine.gui, ...)
---@field getWindowHeight fun(self:LumosEngine.gui, ...)
---@field isWindowHovered fun(self:LumosEngine.gui, ...)
---@field beginChild fun(self:LumosEngine.gui, ...)
---@field getWindowWidth fun(self:LumosEngine.gui, ...)
---@field pushButtonRepeat fun(self:LumosEngine.gui, ...)
---@field inputInt fun(self:LumosEngine.gui, ...)
---@field GetColumnOffset fun(self:LumosEngine.gui, ...)
---@field beginMenuBar fun(self:LumosEngine.gui, ...)
---@field setNextWindowPos fun(self:LumosEngine.gui, ...)
---@field beginDragDropTarget fun(self:LumosEngine.gui, ...)
---@field getFrameHeightWithSpacing fun(self:LumosEngine.gui, ...)
---@field test fun(self:LumosEngine.gui, ...)
---@field setColumnWidth fun(self:LumosEngine.gui, ...)
---@field TreeNodeFlags any
---@field setDragDropPayload fun(self:LumosEngine.gui, ...)
---@field columns fun(self:LumosEngine.gui, ...)
---@field separator fun(self:LumosEngine.gui, ...)
---@field popStyleColor fun(self:LumosEngine.gui, ...)
---@field setTabItemClosed fun(self:LumosEngine.gui, ...)
---@field endTabItem fun(self:LumosEngine.gui, ...)
---@field beginCombo fun(self:LumosEngine.gui, ...)
---@field InputTextFlags any
---@field endTabBar fun(self:LumosEngine.gui, ...)
---@field getTextLineHeight fun(self:LumosEngine.gui, ...)
---@field beginTabBar fun(self:LumosEngine.gui, ...)
---@field collapsingHeaderClosable fun(self:LumosEngine.gui, ...)
---@field treeNode fun(self:LumosEngine.gui, ...)
---@field getScrollY fun(self:LumosEngine.gui, ...)
---@field setWindowFocus fun(self:LumosEngine.gui, ...)
---@field GetColumnWidth fun(self:LumosEngine.gui, ...)
---@field WindowFlags any
---@field GetColumnIndex fun(self:LumosEngine.gui, ...)
---@field styleColorsLight fun(self:LumosEngine.gui, ...)
---@field nextColumn fun(self:LumosEngine.gui, ...)
---@field beginDragDropSource fun(self:LumosEngine.gui, ...)
---@field closeCurrentPopup fun(self:LumosEngine.gui, ...)
---@field openPopupOnItemClick fun(self:LumosEngine.gui, ...)
---@field endPopup fun(self:LumosEngine.gui, ...)
---@field beginPopupModal fun(self:LumosEngine.gui, ...)
---@field beginTabItem fun(self:LumosEngine.gui, ...)
---@field setCursorPosX fun(self:LumosEngine.gui, ...)
---@field textColored fun(self:LumosEngine.gui, ...)
---@field beginWindowClosable fun(self:LumosEngine.gui, ...)
---@field beginPopupContextItem fun(self:LumosEngine.gui, ...)
---@field setTooltip fun(self:LumosEngine.gui, ...)
---@field isWindowFocused fun(self:LumosEngine.gui, ...)
---@field value fun(self:LumosEngine.gui, ...)
---@field alignTextToFramePadding fun(self:LumosEngine.gui, ...)
---@field beginTooltip fun(self:LumosEngine.gui, ...)
---@field newLine fun(self:LumosEngine.gui, ...)
---@field menuItem fun(self:LumosEngine.gui, ...)
---@field endMenu fun(self:LumosEngine.gui, ...)
---@field popButtonRepeat fun(self:LumosEngine.gui, ...)
---@field indent fun(self:LumosEngine.gui, ...)
---@field radioButton fun(self:LumosEngine.gui, ...)
---@field pushStyleColor fun(self:LumosEngine.gui, ...)
---@field setCursorScreenPos fun(self:LumosEngine.gui, ...)
---@field popTextWrapPos fun(self:LumosEngine.gui, ...)
---@field endMainMenuBar fun(self:LumosEngine.gui, ...)
---@field FontAtlasFlags any
---@field getStyleColorVec4 fun(self:LumosEngine.gui, ...)
---@field selectable fun(self:LumosEngine.gui, ...)
---@field isWindowCollapsed fun(self:LumosEngine.gui, ...)
---@field getTreeNodeToLabelSpacing fun(self:LumosEngine.gui, ...)
---@field treePush fun(self:LumosEngine.gui, ...)
---@field arrowButton fun(self:LumosEngine.gui, ...)
---@field colourButton fun(self:LumosEngine.gui, ...)
---@field inputFloat fun(self:LumosEngine.gui, ...)
---@field getFontTexUvWhitePixel fun(self:LumosEngine.gui, ...)
---@field colourPicker3 fun(self:LumosEngine.gui, ...)
---@field styleColorsDark fun(self:LumosEngine.gui, ...)
---@field colourEdit3 fun(self:LumosEngine.gui, ...)
---@field setColorEditOptions fun(self:LumosEngine.gui, ...)
---@field getCursorPosX fun(self:LumosEngine.gui, ...)
---@field smallButton fun(self:LumosEngine.gui, ...)
---@field styleColorsClassic fun(self:LumosEngine.gui, ...)
---@field beginPopupContextVoid fun(self:LumosEngine.gui, ...)
---@field TabItemFlags any
---@field FocusedFlags any
---@field bulletText fun(self:LumosEngine.gui, ...)
---@field labelText fun(self:LumosEngine.gui, ...)
---@field Col any
---@field pushStyleVar fun(self:LumosEngine.gui, ...)
---@field pushID fun(self:LumosEngine.gui, ...)
---@field getContentRegionMax fun(self:LumosEngine.gui, ...)
---@field text fun(self:LumosEngine.gui, ...)
---@field getWindowContentRegionMax fun(self:LumosEngine.gui, ...)
---@field TabBarFlags any
---@field beginMenu fun(self:LumosEngine.gui, ...)
---@field StyleVar any
---@field endDragDropSource fun(self:LumosEngine.gui, ...)
---@field getFrameHeight fun(self:LumosEngine.gui, ...)
---@field getWindowContentRegionMin fun(self:LumosEngine.gui, ...)
---@field showFontSelector fun(self:LumosEngine.gui, ...)
---@field DrawListFlags any
---@field render fun(self:LumosEngine.gui, ...)
---@field MouseCursor any
---@field getTextLineHeightWithSpacing fun(self:LumosEngine.gui, ...)
---@field setCursorPos fun(self:LumosEngine.gui, ...)
---@field ImVec2 any
---@field pushFont fun(self:LumosEngine.gui, ...)
---@field getVersion fun(self:LumosEngine.gui, ...)
---@field Dir any
---@field endMenuBar fun(self:LumosEngine.gui, ...)
---@field acceptDragDropPayload fun(self:LumosEngine.gui, ...)
---@field getWindowSize fun(self:LumosEngine.gui, ...)
---@field SelectableFlags any
---@field invisibleButton fun(self:LumosEngine.gui, ...)
---@field HoveredFlags any
---@field endGroup fun(self:LumosEngine.gui, ...)
---@field button fun(self:LumosEngine.gui, ...)
---@field unindent fun(self:LumosEngine.gui, ...)
---@field popID fun(self:LumosEngine.gui, ...)
---@field beginMainMenuBar fun(self:LumosEngine.gui, ...)
---@field getContentRegionAvail fun(self:LumosEngine.gui, ...)
---@field pushTextWrapPos fun(self:LumosEngine.gui, ...)
---@field DragDropFlags any
---@field pushItemWidth fun(self:LumosEngine.gui, ...)
---@field DrawCornerFlags any
---@field textDisabled fun(self:LumosEngine.gui, ...)
---@field sameLine fun(self:LumosEngine.gui, ...)
---@field showDemoWindow fun(self:LumosEngine.gui, ...)
---@field IO any
---@field showUserGuid fun(self:LumosEngine.gui, ...)
---@field setWindowSize fun(self:LumosEngine.gui, ...)
---@field endChild fun(self:LumosEngine.gui, ...)
---@field GetColumnsCount fun(self:LumosEngine.gui, ...)
---@field collapsingHeader fun(self:LumosEngine.gui, ...)
---@field ColorEditFlags any

---@class LumosEngine.Input
local Input = {}
---@field new fun(...):LumosEngine.Input
---@field IsControllerButtonPressed fun(self:LumosEngine.Input, ...)
---@field GetMouseClicked fun(self:LumosEngine.Input, ...)
---@field GetControllerName fun(self:LumosEngine.Input, ...)
---@field GetMouseHeld fun(self:LumosEngine.Input, ...)
---@field GetMousePosition fun(self:LumosEngine.Input, ...)
---@field GetKeyHeld fun(self:LumosEngine.Input, ...)
---@field GetControllerHat fun(self:LumosEngine.Input, ...)
---@field GetScrollOffset fun(self:LumosEngine.Input, ...)
---@field GetControllerAxis fun(self:LumosEngine.Input, ...)
---@field GetKeyPressed fun(self:LumosEngine.Input, ...)

---@class LumosEngine.Mesh
local Mesh = {}
---@field new fun(...):LumosEngine.Mesh
---@field __name any

local LumosEngine = {}
return LumosEngine
