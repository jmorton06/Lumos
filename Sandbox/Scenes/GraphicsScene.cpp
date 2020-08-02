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
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

	LoadModels();

    auto environment = m_EntityManager->Create("Environment");
    environment.AddComponent<Graphics::Environment>("/Textures/cubemap/Arches_E_PineTree", 11, 3072, 4096, ".tga");
    
    auto lightEntity = m_EntityManager->Create("Light");
    lightEntity.AddComponent<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
    lightEntity.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
    
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
	m_Terrain = m_EntityManager->Create("HeightMap");
    m_Terrain.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(-100.0f, -60.0f,-318.0f)));
    m_Terrain.AddComponent<TextureMatrixComponent>(Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
	Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain());

	auto material = Lumos::CreateRef<Material>();
	material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

    m_Terrain.AddComponent<MaterialComponent>(material);
    m_Terrain.AddComponent<MeshComponent>(terrain);
}

int width = 500;
int height = 500;
int lowside = 50;
int lowscale = 10;
float xRand = 1.0f;
float yRand = 150.0f;
float zRand = 1.0f;
float texRandX = 1.0f / 16.0f;
float texRandZ = 1.0f / 16.0f;

void GraphicsScene::OnImGui()
{
	ImGui::Begin("Terrain");

	ImGui::SliderInt("Width", &width, 1, 5000);
	ImGui::SliderInt("Height", &height, 1, 5000);
	ImGui::SliderInt("lowside", &lowside, 1, 300);
	ImGui::SliderInt("lowscale", &lowscale, 1, 300);

	ImGui::SliderFloat("xRand", &xRand, 0.0f, 300.0f);
	ImGui::SliderFloat("yRand", &yRand, 0.0f, 300.0f);
	ImGui::SliderFloat("zRand", &zRand, 0.0f, 300.0f);

	ImGui::InputFloat("texRandX", &texRandX);
	ImGui::InputFloat("texRandZ", &texRandZ);

	if(ImGui::Button("Rebuild Terrain"))
	{
		m_Terrain.Destroy();
    
        m_Terrain = m_EntityManager->Create("HeightMap");
        m_Terrain.AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(1.0f)));
        m_Terrain.AddComponent<TextureMatrixComponent>(Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
        Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain(width, height, lowside, lowscale, xRand, yRand, zRand, texRandX, texRandZ));
        
        auto material = Lumos::CreateRef<Material>();
        material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");
        
        m_Terrain.AddComponent<MaterialComponent>(material);
        m_Terrain.AddComponent<MeshComponent>(terrain);
	}

	ImGui::End();
}
