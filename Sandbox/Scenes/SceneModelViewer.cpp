#include "SceneModelViewer.h"

using namespace Lumos;
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

	m_EnvironmentMap = TextureCube::CreateFromVCross(environmentFiles, 11);

	auto sun = std::make_shared<Light>();
	sun->SetDirection(maths::Vector3(26.0f, 22.0f, 48.5f));
	sun->SetPosition(maths::Vector3(26.0f, 22.0f, 48.5f) * 10000.0f);
	sun->SetBrightness(4.0f);
	m_LightSetup->SetDirectionalLight(sun);

    auto shadowTexture = std::unique_ptr<TextureDepthArray>(TextureDepthArray::Create(2048, 2048, 4));
    auto shadowRenderer = new ShadowRenderer();
    auto deferredRenderer = new DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
    auto skyboxRenderer = new SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap);
    deferredRenderer->SetRenderTarget(Application::Instance()->GetRenderManager()->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0]);
    skyboxRenderer->SetRenderTarget(Application::Instance()->GetRenderManager()->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0]);
    
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
	std::shared_ptr<Entity> TestObject = ModelLoader::LoadModel("/Meshes/Scene/scene.gltf");
	//DamagedHelmet/glTF/DamagedHelmet.gltf");//Cube/Cube.gltf");//Scene/scene.gltf");///Meshes/Spyro/ArtisansHub.obj
	TestObject->SetBoundingRadius(20000.0f);
	AddEntity(TestObject);

}

void SceneModelViewer::OnIMGUI()
{
	ImGui::Begin(m_SceneName.c_str());
 	if(ImGui::Button("<- Back"))
	{
		Application::Instance()->GetSceneManager()->JumpToScene("SceneSelect");
		ImGui::End();
		return;
	}

    ImGui::End();
}
