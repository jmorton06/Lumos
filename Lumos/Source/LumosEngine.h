#pragma once

//App
#include "Core/Core.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Core/LMLog.h"

//Physics
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/DistanceConstraint.h"
#include "Physics/LumosPhysicsEngine/SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/WeldConstraint.h"
#include "Physics/LumosPhysicsEngine/Broadphase.h"
#include "Physics/LumosPhysicsEngine/OctreeBroadphase.h"
#include "Physics/LumosPhysicsEngine/BruteForceBroadphase.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/RigidBody.h"
#include "Physics/B2PhysicsEngine/RigidBody2D.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"

//Graphics
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/IMGUIRenderer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Sprite.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Terrain.h"
#include "Graphics/Light.h"
#include "Graphics/Environment.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/AnimatedSprite.h"

//Entity
#include "Scene/Component/Components.h"
#include "Scene/EntityManager.h"
#include "Scene/EntityFactory.h"
//Cameras
#include "Graphics/Camera/ThirdPersonCamera.h"
#include "Graphics/Camera/FPSCamera.h"
#include "Graphics/Camera/Camera2D.h"
#include "Graphics/Camera/Camera.h"
//Maths
#include "Maths/Maths.h"
#include "Maths/Transform.h"

//Audio
#include "Audio/AudioManager.h"
#include "Audio/Sound.h"
#include "Audio/SoundNode.h"

//System
#include "Core/VFS.h"
#include "Core/OS/FileSystem.h"
#include "Core/StringUtilities.h"
#include "Core/CoreSystem.h"
#include "Core/LMLog.h"
#include "Core/OS/Input.h"
#include "Core/OS/OS.h"

//Scripting
#include "Scripting/Lua/LuaScriptComponent.h"
#include "Scripting/Lua/LuaManager.h"

//Utilities
#include "Utilities/LoadImage.h"
#include "Utilities/Timer.h"
#include "Maths/Random.h"
#include "Utilities/TimeStep.h"

#include "ImGui/ImGuiManager.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Graphics/Renderers/DeferredRenderer.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/Renderers/ShadowRenderer.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Graphics/Renderers/SkyboxRenderer.h"
#include "Graphics/Renderers/RenderGraph.h"

//External
#include <imgui/imgui.h>
