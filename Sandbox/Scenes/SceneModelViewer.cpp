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

	Light* sun = new Light();
	sun->SetDirection(maths::Vector3(26.0f, 22.0f, 48.5f));
	sun->SetPosition(maths::Vector3(26.0f, 22.0f, 48.5f) * 10000.0f);
	sun->SetBrightness(4.0f);
	m_LightSetup->SetDirectionalLight(sun);
}

void SceneModelViewer::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
}

void SceneModelViewer::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		if (m_pCamera)
		{
			delete m_pCamera;
			m_pCamera = nullptr;
		}

		//delete m_EnvironmentMap;
	}

	Scene::OnCleanupScene();
}

void SceneModelViewer::LoadModels()
{
	std::shared_ptr<Entity> TestObject = std::make_shared<Entity>("TestObject", this);
	TestObject->SetBoundingRadius(20000.0f);

	TestObject->AddComponent(std::make_unique<TransformComponent>(Matrix4()));//(Matrix4::Translation(maths::Vector3(0.0, 0.0, 0.0f)) * Matrix4::RotationX(-90.0f) * Matrix4::RotationZ(90.0f))));
	std::shared_ptr<Model> model = std::make_shared<Model>("/Meshes/Spyro/ArtisansHub.obj");//DamagedHelmet/glTF/DamagedHelmet.gltf");//Cube/Cube.gltf");//Scene/scene.gltf");// *AssetsManager::DefaultModels()->GetAsset("Cube"));
	TestObject->AddComponent(std::make_unique<ModelComponent>(model));

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
