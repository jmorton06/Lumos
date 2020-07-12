#include "SceneModelViewer.h"

using namespace Lumos;
using namespace Maths;

SceneModelViewer::SceneModelViewer(const std::string& SceneName)
	: Scene(SceneName)
{
}

SceneModelViewer::~SceneModelViewer()
{
}

void SceneModelViewer::OnInit()
{
	Scene::OnInit();

	LoadModels();

	auto audioSystem = Application::Get().GetSystem<AudioManager>();

	auto environment = GetRegistry().create();
	GetRegistry().emplace<Graphics::Environment>(environment, "/Textures/cubemap/Arches_E_PineTree", 11, 3072, 4096, ".tga");
	GetRegistry().emplace<NameComponent>(environment, "Environment");

	auto lightEntity = GetRegistry().create();
	GetRegistry().emplace<Graphics::Light>(lightEntity, Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
	GetRegistry().emplace<Maths::Transform>(lightEntity, Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	GetRegistry().emplace<NameComponent>(lightEntity, "Directional Light");

	auto cameraEntity = GetRegistry().create();
	Camera& camera = GetRegistry().emplace<Camera>(cameraEntity, -20.0f, 330.0f, Maths::Vector3(-2.5f, 1.3f, 3.8f), 45.0f, 0.1f, 1000.0f, (float)m_ScreenWidth / (float)m_ScreenHeight);
	camera.SetCameraController(CreateRef<EditorCameraController>());
	GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");

	//Temp
	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

	auto deferredRenderer = new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
	auto skyboxRenderer = new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight);

	auto deferredLayer = new Layer3D(deferredRenderer, "Deferred");
	auto skyBoxLayer = new Layer3D(skyboxRenderer, "Skybox");
	PushLayer(deferredLayer);
	PushLayer(skyBoxLayer);

#ifndef LUMOS_PLATFORM_IOS
	auto shadowRenderer = new Graphics::ShadowRenderer();
	auto shadowLayer = new Layer3D(shadowRenderer);
	Application::Get().GetRenderManager()->SetShadowRenderer(shadowRenderer);
	PushLayer(shadowLayer);
#endif

	m_SceneBoundingRadius = 20.0f;
}

void SceneModelViewer::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);
}

void SceneModelViewer::OnCleanupScene()
{
	Scene::OnCleanupScene();
}

void SceneModelViewer::LoadModels()
{
	std::vector<std::string> ExampleModelPaths{
		"/Meshes/DamagedHelmet/glTF/DamagedHelmet.gltf",
		"/Meshes/Scene/scene.gltf",
		"/Meshes/Spyro/ArtisansHub.obj",
		"/Meshes/Cube/Cube.gltf",
		"/Meshes/KhronosExamples/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
		"/Meshes/KhronosExamples/EnvironmentTest/glTF/EnvironmentTest.gltf",
		"/Meshes/Sponza/sponza.gltf",
		"/Meshes/capsule.glb"};

	auto TestObject = ModelLoader::LoadModel(ExampleModelPaths[0], GetRegistry());

	if(!GetRegistry().has<Maths::Transform>(TestObject))
		GetRegistry().emplace<Maths::Transform>(TestObject, Maths::Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
}

void SceneModelViewer::OnImGui()
{
}
