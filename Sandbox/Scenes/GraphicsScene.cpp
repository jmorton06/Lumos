#include "GraphicsScene.h"

using namespace Lumos;
using namespace Maths;

GraphicsScene::GraphicsScene(const std::string& SceneName) : Scene(SceneName), m_Terrain(entt::null) {}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::OnInit()
{
	Scene::OnInit();
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

	LoadModels();

	m_SceneBoundingRadius = 200.0f;

    auto environment = GetRegistry().create();
    GetRegistry().emplace<Graphics::Environment>(environment, "/Textures/cubemap/Arches_E_PineTree", 11, 3072, 4096, ".tga");
    GetRegistry().emplace<NameComponent>(environment, "Environment");

	auto lightEntity = GetRegistry().create();
	GetRegistry().emplace<Graphics::Light>(lightEntity, Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
	GetRegistry().emplace<Maths::Transform>(lightEntity, Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	GetRegistry().emplace<NameComponent>(lightEntity, "Light");

	auto cameraEntity = GetRegistry().create();
	auto& camera = GetRegistry().emplace<Camera>(cameraEntity, 45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
	GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");
	camera.SetCameraController(CreateRef<EditorCameraController>());

	auto audioSystem = Application::Get().GetSystem<AudioManager>();
	if (audioSystem)
		Application::Get().GetSystem<AudioManager>()->SetListener(&camera);

    bool editor = false;

    #ifdef LUMOS_EDITOR
    editor = true;
    #endif
	auto deferredRenderer = new Graphics::ForwardRenderer(m_ScreenWidth, m_ScreenHeight, editor, true);
	auto skyboxRenderer = new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight);

    //Can't render to array texture on iPhoneX or older
#ifndef LUMOS_PLATFORM_IOS
    auto shadowRenderer = new Graphics::ShadowRenderer();
	shadowRenderer->SetLightEntity(lightEntity);
    auto shadowLayer = new Layer3D(shadowRenderer);
    Application::Get().GetRenderManager()->SetShadowRenderer(shadowRenderer);
    PushLayer(shadowLayer);
#endif

	auto deferredLayer = new Layer3D(deferredRenderer);
	auto skyBoxLayer = new Layer3D(skyboxRenderer);
	PushLayer(deferredLayer);
	PushLayer(skyBoxLayer);
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
	m_Terrain = GetRegistry().create(); // EntityManager::Get().CreateEntity("heightmap");
	GetRegistry().emplace<Maths::Transform>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f)));
	GetRegistry().emplace<TextureMatrixComponent>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
	GetRegistry().emplace<NameComponent>(m_Terrain, "HeightMap");
    Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain());

	auto material = Lumos::CreateRef<Material>();
	material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

	GetRegistry().emplace<MaterialComponent>(m_Terrain, material);
	GetRegistry().emplace<MeshComponent>(m_Terrain, terrain);

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
		GetRegistry().destroy(m_Terrain);

		m_Terrain = GetRegistry().create();
		GetRegistry().emplace<Maths::Transform>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f)));
		GetRegistry().emplace<TextureMatrixComponent>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
		GetRegistry().emplace<NameComponent>(m_Terrain, "HeightMap");
		Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain(width, height, lowside, lowscale, xRand, yRand, zRand, texRandX, texRandZ));

		auto material = Lumos::CreateRef<Material>();
		material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

		GetRegistry().emplace<MaterialComponent>(m_Terrain, material);
		GetRegistry().emplace<MeshComponent>(m_Terrain, terrain);
    }
    
    ImGui::End();
}
