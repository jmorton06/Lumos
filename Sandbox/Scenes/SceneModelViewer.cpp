#include "SceneModelViewer.h"

using namespace lumos;
using namespace maths;

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

	m_pCamera = new ThirdPersonCamera(-20.0f, -40.0f, maths::Vector3(-10.0f, 10.0f, 20.0f), 45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);

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

	m_EnvironmentMap = graphics::TextureCube::CreateFromVCross(environmentFiles, 11);

	auto sun = std::make_shared<graphics::Light>(maths::Vector3(26.0f, 22.0f, 48.5f), maths::Vector4(1.0f), 2.0f);

	auto lightEntity = std::make_shared<Entity>("Directional Light");
	lightEntity->AddComponent(std::make_unique<LightComponent>(sun));
	lightEntity->AddComponent(std::make_unique<TransformComponent>(Matrix4::Translation(maths::Vector3(26.0f, 22.0f, 48.5f))));
	AddEntity(lightEntity);

    auto shadowTexture = std::unique_ptr<graphics::TextureDepthArray>(graphics::TextureDepthArray::Create(2048, 2048, 4));
    auto shadowRenderer = new graphics::ShadowRenderer();
    auto deferredRenderer = new graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
    auto skyboxRenderer = new graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap);
	shadowRenderer->SetLight(sun);

    deferredRenderer->SetRenderToGBufferTexture(true);
    skyboxRenderer->SetRenderToGBufferTexture(true);
    
    auto shadowLayer = new Layer3D(shadowRenderer, "Shadow");
    auto deferredLayer = new Layer3D(deferredRenderer, "Deferred");
    auto skyBoxLayer = new Layer3D(skyboxRenderer, "Skybox");
    Application::Instance()->PushLayer(shadowLayer);
    Application::Instance()->PushLayer(deferredLayer);
    Application::Instance()->PushLayer(skyBoxLayer);
    
    m_SceneLayers.emplace_back(shadowLayer);
    m_SceneLayers.emplace_back(deferredLayer);
    m_SceneLayers.emplace_back(skyBoxLayer);
    //Application::Instance()->PushOverLay(new ImGuiLayer(true));
    
    Application::Instance()->GetRenderManager()->SetShadowRenderer(shadowRenderer);
    Application::Instance()->GetRenderManager()->SetSkyBoxTexture(m_EnvironmentMap);
}

void SceneModelViewer::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
}

void SceneModelViewer::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		SAFE_DELETE(m_pCamera)
        SAFE_DELETE(m_EnvironmentMap);
	}

	Scene::OnCleanupScene();
}

void SceneModelViewer::LoadModels()
{
	std::vector<String> ExampleModelPaths
	{
		"/Meshes/DamagedHelmet/glTF/DamagedHelmet.gltf",
		"/Meshes/Scene/scene.gltf",
		"/Meshes/Spyro/ArtisansHub.obj",
		"/Meshes/Cube/Cube.gltf",
		"/Meshes/KhronosExamples/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
		"/Meshes/KhronosExamples/EnvironmentTest/glTF/EnvironmentTest.gltf",
        "/Meshes/KhronosExamples/Sponza/glTF/Sponza.gltf"
	};

	std::shared_ptr<Entity> TestObject = ModelLoader::LoadModel(ExampleModelPaths[0]);
	TestObject->SetBoundingRadius(1000.0f);
	TestObject->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4::Scale(maths::Vector3(10.0f, 10.0f, 10.0f))));
	AddEntity(TestObject);

}

void SceneModelViewer::OnIMGUI()
{
}
