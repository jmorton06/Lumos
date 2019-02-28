#include "Scene2D.h"

using namespace Lumos;
using namespace maths;

Scene2D::Scene2D(const std::string& SceneName)
	: Scene(SceneName)
{
}

Scene2D::~Scene2D()
{
}

void Scene2D::OnInit()
{
	Scene::OnInit();

	LumosPhysicsEngine::Instance()->SetDampingFactor(0.998f);
	LumosPhysicsEngine::Instance()->SetIntegrationType(INTEGRATION_RUNGE_KUTTA_4);
	LumosPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 5, std::make_shared<SortAndSweepBroadphase>()));

	SetDebugDrawFlags(DEBUGDRAW_FLAGS_ENTITY_COMPONENTS | DEBUGDRAW_FLAGS_COLLISIONVOLUMES);

	SetDrawObjects(true);
	SetUseShadow(true);

	m_pCamera = new Camera2D(m_ScreenWidth,
		m_ScreenHeight,
		static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight)
		, 6);

	m_SceneBoundingRadius = 20.0f;

	Application::Instance()->GetAudioManager()->SetListener(m_pCamera);

	Application::Instance()->PushOverLay(new ImGuiLayer(true));

	auto renderer2D = new Renderer2D(m_ScreenWidth, m_ScreenHeight);
	Application::Instance()->PushLayer(new Layer2D(renderer2D));
	renderer2D->SetRenderToGBufferTexture(true);

	for (int i = 0; i < 1000; i++)
	{
		std::shared_ptr<Entity> testSprite = std::make_shared<Entity>("Sprite", this);

		std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(maths::Vector2(RandomNumberGenerator32::Rand(-10.0f, 100.0f), RandomNumberGenerator32::Rand(-10.0f, 100.0f)), maths::Vector2(RandomNumberGenerator32::Rand(10.0f, 100.0f), RandomNumberGenerator32::Rand(10.0f, 100.0f)), maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f));
		testSprite->AddComponent(std::make_unique<SpriteComponent>(sprite));
		AddEntity(testSprite);
	}

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

void Scene2D::OnIMGUI()
{
	if (ImGui::Button("<- SceneSelect"))
	{
		Application::Instance()->GetSceneManager()->JumpToScene("SceneSelect");
		return;
	}
}
