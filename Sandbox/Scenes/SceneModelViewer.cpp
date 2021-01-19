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

    auto environment = m_EntityManager->Create("Environment");
    environment.AddComponent<Graphics::Environment>("//TextuAssets/cubemap/Arches_E_PineTree", 11, 3072, 4096, ".tga");
    
    auto lightEntity = m_EntityManager->Create("Light");
    lightEntity.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
    lightEntity.AddComponent<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
    
    auto cameraEntity = m_EntityManager->Create("Camera");
    cameraEntity.AddComponent<Maths::Transform>(Maths::Vector3(-31.0f, 12.0f, 51.0f));
    cameraEntity.AddComponent<Camera>(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float)m_ScreenWidth / (float)m_ScreenHeight);
    cameraEntity.AddComponent<DefaultCameraController>(DefaultCameraController::ControllerType::EditorCamera);
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
		"//Meshes/DamagedHelmet/glTF/DamagedHelmet.gltf",
		"//Meshes/Scene/scene.gltf",
		"//Meshes/Spyro/ArtisansHub.obj",
		"//Meshes/Cube/Cube.gltf",
		"//Meshes/KhronosExamples/MetalRoughSpheAssets/glTF/MetalRoughSpheres.gltf",
		"//Meshes/KhronosExamples/EnvironmentTest/glTF/EnvironmentTest.gltf",
		"//Meshes/Sponza/sponza.gltf",
		"//Meshes/capsule.glb"};

	Entity modelEntity = m_EntityManager->Create();
	modelEntity.AddComponent<Graphics::Model>(ExampleModelPaths[0]);
    modelEntity.AddOrReplaceComponent<Maths::Transform>(Maths::Matrix4::Scale(Maths::Vector3(20.0f, 20.0f, 20.0f)));
}

void SceneModelViewer::OnImGui()
{
}
