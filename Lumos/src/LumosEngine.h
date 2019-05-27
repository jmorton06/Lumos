#pragma once

//App
#include "App/Engine.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "App/Scene.h"

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
#include "Physics/LumosPhysicsEngine/Octree.h"
#include "Physics/LumosPhysicsEngine/BruteForceBroadphase.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/PhysicsObject.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"

//Graphics
#include "Graphics/API/Textures/TextureCube.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/Textures/TextureDepth.h"
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "Graphics/Mesh.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Graphics/Material.h"
#include "Graphics/Particle.h"
#include "Graphics/ParticleEmitter.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Water.h"
#include "Graphics/Sprite.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Terrain.h"

#include "Utilities/AssetsManager.h"

//Entity
#include "Entity/Entity.h"
#include "Entity/Component/Components.h"

//Cameras
#include "Graphics/Camera/ThirdPersonCamera.h"
#include "Graphics/Camera/FPSCamera.h"
#include "Graphics/Camera/MayaCamera.h"
#include "Graphics/Camera/Camera2D.h"

//Managers
#include "Graphics/LightSetUp.h"
#include "Graphics/ParticleManager.h"

//Maths
#include "Maths/Maths.h"

//Audio
#include "Audio/AudioManager.h"
//#include "Audio/OggSound.h"
#include "Audio/Sound.h"
#include "Audio/SoundNode.h"
//#include "Audio/SoundSystem.h"

//System
#include "System/VFS.h"
#include "System/FileSystem.h"
#include "System/String.h"
#include "System/System.h"
#include "System/LMLog.h"

//Scripting
#include "Scripting/LuaScript.h"

//Utilities
#include "Utilities/LoadImage.h"
#include "Utilities/Timer.h"
#include "Utilities/RandomNumberGenerator.h"
#include "Utilities/TimeStep.h"
#include "Utilities/CommonUtils.h"

//Layers
#include "Graphics/Layers/ImGuiLayer.h"
#include "Graphics/Layers/Layer3D.h"
#include "Graphics/Layers/Layer2D.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Graphics/Renderers/DeferredRenderer.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/Renderers/ShadowRenderer.h"
#include "Graphics/RenderManager.h"

//External
#include <imgui/imgui.h>
