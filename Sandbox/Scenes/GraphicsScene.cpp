#include "GraphicsScene.h"

using namespace Lumos;
using namespace Maths;

GraphicsScene::GraphicsScene(const std::string& SceneName)
	: Scene(SceneName)
{
}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::OnInit()
{
	Scene::OnInit();
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<OctreeBroadphase>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

	LoadModels();

    auto environment = m_EntityManager->Create("Environment");
    environment.AddComponent<Graphics::Environment>("//TextuAssets/cubemap/Arches_E_PineTree", 11, 3072, 4096, 1.0f/ 32.0f, ".tga");
    
    auto lightEntity = m_EntityManager->Create("Light");
    lightEntity.AddComponent<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
    lightEntity.GetOrAddComponent<Maths::Transform>().SetLocalTransform(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
    
    auto cameraEntity = m_EntityManager->Create("Camera");
    cameraEntity.AddComponent<Maths::Transform>(Maths::Vector3(-31.0f, 12.0f, 51.0f));
    cameraEntity.AddComponent<Camera>(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float)m_ScreenWidth / (float)m_ScreenHeight);
    cameraEntity.AddComponent<DefaultCameraController>(DefaultCameraController::ControllerType::EditorCamera);
}

void GraphicsScene::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);
}

void GraphicsScene::OnCleanupScene()
{
	Scene::OnCleanupScene();
}

void GraphicsScene::LoadModels()
{
	//HeightMap
	Entity terrianEntity = m_EntityManager->Create("HeightMap");
    terrianEntity.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(-100.0f, -60.0f,-318.0f)));
    terrianEntity.AddComponent<TextureMatrixComponent>(Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
	Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain());

	auto material = Lumos::CreateRef<Graphics::Material>();
	material->LoadMaterial("checkerboard", "//TextuAssets/checkerboard.tga");

	terrain->SetMaterial(material);
    terrianEntity.AddComponent<Graphics::Model>(terrain, Graphics::PrimitiveType::Terrain);
}
