#include "Scene2D.h"

using namespace Lumos;
using namespace Maths;

Scene2D::Scene2D(const String& SceneName)
	: Scene(SceneName)
{
}

Scene2D::~Scene2D()
{
}

void Scene2D::OnInit()
{
	Scene::OnInit();

	Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetPaused(true);

	m_pCamera = new Camera(static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight), 10.0f);
    m_pCamera->SetCameraController(CreateRef<CameraController2D>(m_pCamera));
    m_pCamera->SetIsOrthographic(true);
    
	LoadLuaScene(ROOT_DIR"/Sandbox/res/scripts/FlappyBirdTest.lua");

	auto cameraEntity = m_Registry.create();
	m_Registry.emplace<CameraComponent>(cameraEntity, m_pCamera);
	m_Registry.emplace<NameComponent>(cameraEntity, "Camera");

	auto audioSystem = Application::Instance()->GetSystem<AudioManager>();
	if (audioSystem)
		Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

	Application::Instance()->PushLayer(new Layer2D(new Graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight, editor, true, false)));
}

void Scene2D::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
}

void Scene2D::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		SAFE_DELETE(m_pCamera)
		SAFE_DELETE(m_EnvironmentMap);
	}

	Scene::OnCleanupScene();
}
