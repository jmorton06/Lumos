#include "GraphicsScene.h"

using namespace Lumos;
using namespace maths;

GraphicsScene::GraphicsScene(const std::string& SceneName) : Scene(SceneName) {}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::OnInit()
{
	Scene::OnInit();
	JMPhysicsEngine::Instance()->SetDampingFactor(0.998f);
	JMPhysicsEngine::Instance()->SetIntegrationType(IntegrationType::INTEGRATION_RUNGE_KUTTA_4);
	JMPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 3, std::make_shared<BruteForceBroadphase>()));

	LoadModels();

	m_SceneBoundingRadius = 200.0f;

	m_pCamera = new ThirdPersonCamera(45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
	m_pCamera->SetYaw(-40.0f);
	m_pCamera->SetPitch(-20.0f);
	m_pCamera->SetPosition(maths::Vector3(120.0f, 70.0f, 260.0f));

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

	m_EnvironmentMap = TextureCube::CreateFromVCross(environmentFiles, 11);

	auto sun = std::make_shared<Light>();
	sun->SetDirection(maths::Vector3(26.0f, 22.0f, 48.5f));
	sun->SetPosition(maths::Vector3(26.0f, 22.0f, 48.5f) * 10000.0f);
	m_LightSetup->SetDirectionalLight(sun);

	SoundSystem::Instance()->SetListener(m_pCamera);
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
	//std::shared_ptr<Entity> water = std::make_shared<Entity>("Water",this);

	//water->AddComponent(std::make_unique<TransformComponent>(Matrix4::Translation(maths::Vector3(250.0f, 10.0f, 250.0f)) *
	//	Matrix4::Scale(maths::Vector3(250.0f, 1.0f, 250.0f)) * Matrix4::Rotation(-90.0f, maths::Vector3(1.0f, 0.0f, 0.0f))));

	//std::shared_ptr<Model> waterModel = std::make_shared<Model>(Water(maths::Vector3(20.0f, 2.0f, 20.0f), maths::Vector3(20.0f, 2.0f, 20.0f)));
	//water->AddComponent(std::make_unique<TextureMatrixComponent>(Matrix4::Scale(maths::Vector3(10.0f, 10.0f, 10.0f))));
	//water->AddComponent(std::make_unique<ModelComponent>(waterModel));
	//water->SetBoundingRadius(200.0f);
	//AddEntity(water);

	//m_MaterialManager->AddAsset("Stone", std::make_shared<Material>(AssetsManager::s_DefualtPBRMaterial->GetShader()));//, "stone", ".png"));

	Terrain* terrainMesh = new Terrain();

	//HeightMap
	std::shared_ptr<Entity> heightmap = std::make_shared<Entity>("heightmap",this);
	heightmap->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(1.0f))));
	heightmap->AddComponent(std::make_unique<TextureMatrixComponent>(Matrix4::Scale(maths::Vector3(1.0f, 1.0f, 1.0f))));
	heightmap->SetBoundingRadius(2000.0f);
	std::shared_ptr<Model> terrain = std::make_shared<Model>(*terrainMesh);
	auto material = std::make_shared<Material>();

	material->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");
	terrain->SetMaterial(material);

	//terrain->SetMaterial(std::make_shared<Material>(*m_MaterialManager->GetAsset("Stone").get()));
	//terrain->SetMaterialFlag(Material::RenderFlags::WIREFRAME);
	heightmap->AddComponent(std::make_unique<ModelComponent>(terrain));

	AddEntity(heightmap);

	delete terrainMesh;
}

void GraphicsScene::OnIMGUI()
{
	ImGui::Begin(m_SceneName.c_str());
 	if(ImGui::Button("<- Back"))
	{
		Application::Instance()->GetSceneManager()->JumpToScene("SceneSelect");
		ImGui::End();
		return;
	}

	auto lightDirection = m_LightSetup->GetDirectionalLight()->GetDirection();

	ImVec4 test = ImVec4(lightDirection.GetX(),lightDirection.GetY(), lightDirection.GetZ(), 1.0f);

	ImGui::Text("Light");
	ImGui::DragFloat4("Direction", &test.x);

	lightDirection = maths::Vector3(test.x,test.y,test.z);
	m_LightSetup->GetDirectionalLight()->SetDirection(lightDirection);

    ImGui::End();
}
