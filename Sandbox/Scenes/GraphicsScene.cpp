#include "GraphicsScene.h"

using namespace Lumos;
using namespace Maths;

GraphicsScene::GraphicsScene(const std::string& SceneName) : Scene(SceneName), m_Terrain(entt::null) {}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::OnInit()
{
	Scene::OnInit();
	Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
	Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
	Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

	LoadModels();

	m_SceneBoundingRadius = 200.0f;

	m_pCamera = new ThirdPersonCamera(45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
	m_pCamera->SetPosition(Maths::Vector3(120.0f, 70.0f, 260.0f));

	String environmentFiles[11] =
	{
		"/Textures/cubemap/CubeMap0.tga",
		"/Textures/cubemap/CubeMap1.tga",
		"/Textures/cubemap/CubeMap2.tga",
		"/Textures/cubemap/CubeMap3.tga",
		"/Textures/cubemap/CubeMap4.tga",
		"/Textures/cubemap/CubeMap5.tga",
		"/Textures/cubemap/CubeMap6.tga",
		"/Textures/cubemap/CubeMap7.tga",
		"/Textures/cubemap/CubeMap8.tga",
		"/Textures/cubemap/CubeMap9.tga",
		"/Textures/cubemap/CubeMap10.tga"
	};

	m_EnvironmentMap = Graphics::TextureCube::CreateFromVCross(environmentFiles, 11);

	auto lightEntity = m_Registry.create();
	m_Registry.assign<Graphics::Light>(lightEntity, Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
	m_Registry.assign<Maths::Transform>(lightEntity, Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	m_Registry.assign<NameComponent>(lightEntity, "Light");

	auto cameraEntity = m_Registry.create();
	m_Registry.assign<CameraComponent>(cameraEntity, m_pCamera);
	m_Registry.assign<NameComponent>(cameraEntity, "Camera");
	Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	auto audioSystem = Application::Instance()->GetSystem<AudioManager>();
	if (audioSystem)
		Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	auto shadowRenderer = new Graphics::ShadowRenderer();
	auto deferredRenderer = new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
	auto skyboxRenderer = new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap);
	shadowRenderer->SetLightEntity(lightEntity);

	deferredRenderer->SetRenderToGBufferTexture(true);
	skyboxRenderer->SetRenderToGBufferTexture(true);

	auto shadowLayer = new Layer3D(shadowRenderer);
	auto deferredLayer = new Layer3D(deferredRenderer);
	auto skyBoxLayer = new Layer3D(skyboxRenderer);
	Application::Instance()->PushLayer(shadowLayer);
	Application::Instance()->PushLayer(deferredLayer);
	Application::Instance()->PushLayer(skyBoxLayer);

	Application::Instance()->GetRenderManager()->SetShadowRenderer(shadowRenderer);
	Application::Instance()->GetRenderManager()->SetSkyBoxTexture(m_EnvironmentMap);
}

void GraphicsScene::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
}

void GraphicsScene::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		SAFE_DELETE(m_pCamera)
        SAFE_DELETE(m_EnvironmentMap);
	}

	Scene::OnCleanupScene();
}

void GraphicsScene::LoadModels()
{
	//HeightMap
	m_Terrain = m_Registry.create(); // EntityManager::Instance()->CreateEntity("heightmap");
	m_Registry.assign<Maths::Transform>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f)));
	m_Registry.assign<TextureMatrixComponent>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
	m_Registry.assign<NameComponent>(m_Terrain, "HeightMap");
    Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain());

	auto material = Lumos::CreateRef<Material>();
	material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

	m_Registry.assign<MaterialComponent>(m_Terrain, material);
	m_Registry.assign<MeshComponent>(m_Terrain, terrain);

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
		m_Registry.destroy(m_Terrain);

		m_Terrain = m_Registry.create();
		m_Registry.assign<Maths::Transform>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f)));
		m_Registry.assign<TextureMatrixComponent>(m_Terrain, Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
		m_Registry.assign<NameComponent>(m_Terrain, "HeightMap");
		Lumos::Ref<Graphics::Mesh> terrain = Lumos::Ref<Graphics::Mesh>(new Terrain(width, height, lowside, lowscale, xRand, yRand, zRand, texRandX, texRandZ));

		auto material = Lumos::CreateRef<Material>();
		material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

		m_Registry.assign<MaterialComponent>(m_Terrain, material);
		m_Registry.assign<MeshComponent>(m_Terrain, terrain);
    }
    
    ImGui::End();
}
