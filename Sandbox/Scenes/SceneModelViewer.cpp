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

	m_pCamera = new MayaCamera(-20.0f, -40.0f, Maths::Vector3(-1.0f, 1.0f, 2.0f), 45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);

	Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

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

	auto sun = Lumos::CreateRef<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 2.0f);

	auto lightEntity = EntityManager::Instance()->CreateEntity("Directional Light");
	lightEntity->AddComponent<LightComponent>(sun);
	lightEntity->AddComponent<TransformComponent>(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)));
	lightEntity->GetTransformComponent()->GetTransform().SetLocalOrientation(Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::Zero()));
	lightEntity->GetTransformComponent()->GetTransform().ApplyTransform();
	AddEntity(lightEntity);

    auto shadowTexture = std::unique_ptr<Graphics::TextureDepthArray>(Graphics::TextureDepthArray::Create(2048, 2048, 4));
    auto shadowRenderer = new Graphics::ShadowRenderer();
    auto deferredRenderer = new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
    auto skyboxRenderer = new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap);
	shadowRenderer->SetLight(sun);

    deferredRenderer->SetRenderToGBufferTexture(true);
    skyboxRenderer->SetRenderToGBufferTexture(true);
    
    auto shadowLayer = new Layer3D(shadowRenderer, "Shadow");
    auto deferredLayer = new Layer3D(deferredRenderer, "Deferred");
    auto skyBoxLayer = new Layer3D(skyboxRenderer, "Skybox");
    Application::Instance()->PushLayer(shadowLayer);
    Application::Instance()->PushLayer(deferredLayer);
    Application::Instance()->PushLayer(skyBoxLayer);
    
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
        "/Meshes/sponza.glb",
        "/Meshes/capsule.glb"
	};

	auto TestObject = ModelLoader::LoadModel(ExampleModelPaths[0]);
	TestObject->GetOrAddComponent<TransformComponent>(Maths::Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));
	AddEntity(TestObject);

}

void SceneModelViewer::OnIMGUI()
{
}
