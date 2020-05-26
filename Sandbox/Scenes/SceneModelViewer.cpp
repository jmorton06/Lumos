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

	m_pCamera = new Camera(-20.0f, 330.0f, Maths::Vector3(-2.5f, 1.3f, 3.8f), 45.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
    m_pCamera->SetCameraController(CreateRef<EditorCameraController>(m_pCamera));

	auto audioSystem = Application::Instance()->GetSystem<AudioManager>();
	if (audioSystem)
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
    
    String environmentIrrFiles[7] =
    {
         "/Textures/cubemap/CubeMapIrr_0.tga",
         "/Textures/cubemap/CubeMapIrr_1.tga",
         "/Textures/cubemap/CubeMapIrr_2.tga",
         "/Textures/cubemap/CubeMapIrr_3.tga",
         "/Textures/cubemap/CubeMapIrr_4.tga",
         "/Textures/cubemap/CubeMapIrr_5.tga",
         "/Textures/cubemap/CubeMapIrr_6.tga",
    };

    auto environmentMap = Graphics::TextureCube::CreateFromVCross(environmentFiles, 11);
    auto irradianceMap = Graphics::TextureCube::CreateFromVCross(environmentIrrFiles, 7);

    auto environment = m_Registry.create();
    m_Registry.emplace<Graphics::Environment>(environment, environmentMap, irradianceMap);
    m_Registry.emplace<NameComponent>(environment, "Environment");

    
    auto lightEntity = m_Registry.create();
    m_Registry.emplace<Graphics::Light>(lightEntity, Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
    m_Registry.emplace<Maths::Transform>(lightEntity,Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	m_Registry.emplace<NameComponent>(lightEntity, "Directional Light");

    auto cameraEntity = m_Registry.create();
    m_Registry.emplace<CameraComponent>(cameraEntity, m_pCamera);
	m_Registry.emplace<NameComponent>(cameraEntity, "Camera");

	//Temp
	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

    auto deferredRenderer = new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
    auto skyboxRenderer = new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight);

    deferredRenderer->SetRenderToGBufferTexture(editor);
    skyboxRenderer->SetRenderToGBufferTexture(editor);
    
    auto deferredLayer = new Layer3D(deferredRenderer, "Deferred");
    auto skyBoxLayer = new Layer3D(skyboxRenderer, "Skybox");
    Application::Instance()->PushLayer(deferredLayer);
    Application::Instance()->PushLayer(skyBoxLayer);
        
#ifndef LUMOS_PLATFORM_IOS
    auto shadowRenderer = new Graphics::ShadowRenderer();
    shadowRenderer->SetLightEntity(lightEntity);
    auto shadowLayer = new Layer3D(shadowRenderer);
    Application::Instance()->GetRenderManager()->SetShadowRenderer(shadowRenderer);
    Application::Instance()->PushLayer(shadowLayer);
#endif
    
    m_SceneBoundingRadius = 20.0f;
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
        "/Meshes/Sponza/sponza.gltf",
        "/Meshes/capsule.glb"
	};

	auto TestObject = ModelLoader::LoadModel(ExampleModelPaths[0], m_Registry);
	m_Registry.get_or_emplace<Maths::Transform>(TestObject, Maths::Matrix4::Scale(Maths::Vector3(1.0f, 1.0f, 1.0f)));

}

void SceneModelViewer::OnImGui()
{
}
