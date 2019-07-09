#include "GraphicsScene.h"

using namespace Lumos;
using namespace Maths;

GraphicsScene::GraphicsScene(const std::string& SceneName) : Scene(SceneName) {}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::OnInit()
{
	Scene::OnInit();
	LumosPhysicsEngine::Instance()->SetDampingFactor(0.998f);
	LumosPhysicsEngine::Instance()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
	LumosPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 3, std::make_shared<BruteForceBroadphase>()));

	LoadModels();

	m_SceneBoundingRadius = 200.0f;

	m_pCamera = new ThirdPersonCamera(45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
	m_pCamera->SetYaw(-40.0f);
	m_pCamera->SetPitch(-20.0f);
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

	auto sun = std::make_shared<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 2.0f);

	auto lightEntity = EntityManager::Instance()->CreateEntity("Directional Light");
	lightEntity->AddComponent<LightComponent>(sun);
	lightEntity->AddComponent<TransformComponent>(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)));
	AddEntity(lightEntity);

	Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	m_ShadowTexture = std::unique_ptr<Graphics::TextureDepthArray>(Graphics::TextureDepthArray::Create(4096, 4096, 4));
	auto shadowRenderer = new Graphics::ShadowRenderer();
	auto deferredRenderer = new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
	auto skyboxRenderer = new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap);
	shadowRenderer->SetLight(sun);

	deferredRenderer->SetRenderToGBufferTexture(true);
	skyboxRenderer->SetRenderToGBufferTexture(true);

	auto shadowLayer = new Layer3D(shadowRenderer);
	auto deferredLayer = new Layer3D(deferredRenderer);
	auto skyBoxLayer = new Layer3D(skyboxRenderer);
	Application::Instance()->PushLayer(shadowLayer);
	Application::Instance()->PushLayer(deferredLayer);
	Application::Instance()->PushLayer(skyBoxLayer);

	m_SceneLayers.emplace_back(shadowLayer);
	m_SceneLayers.emplace_back(deferredLayer);
	m_SceneLayers.emplace_back(skyBoxLayer);

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
		m_ShadowTexture.reset();
		SAFE_DELETE(m_pCamera)
        SAFE_DELETE(m_EnvironmentMap);
	}

	Scene::OnCleanupScene();
}

void GraphicsScene::LoadModels()
{
	Terrain* terrainMesh = new Terrain();

	//HeightMap
	auto heightmap = EntityManager::Instance()->CreateEntity("heightmap");
	heightmap->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(1.0f)));
	heightmap->AddComponent<TextureMatrixComponent>(Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
	std::shared_ptr<Graphics::Mesh> terrain = std::make_shared<Graphics::Mesh>(*terrainMesh);
	auto material = std::make_shared<Material>();

	material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

	heightmap->AddComponent<MaterialComponent>(material);
    heightmap->SetBoundingRadius(800.0f);

	//terrain->SetMaterial(std::make_shared<Material>(*m_MaterialManager->GetAsset("Stone").get()));
	//terrain->SetMaterialFlag(Material::RenderFlags::WIREFRAME);
	heightmap->AddComponent<MeshComponent>(terrain);

	AddEntity(heightmap);

	delete terrainMesh;
}

void GraphicsScene::OnIMGUI()
{
}
