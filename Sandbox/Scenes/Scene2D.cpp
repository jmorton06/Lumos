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

	m_pCamera = new Camera2D(16, 9 , 0.5f);

	m_SceneBoundingRadius = 20.0f;

	Application::Instance()->GetAudioManager()->SetListener(m_pCamera);

	Application::Instance()->PushOverLay(new ImGuiLayer(true));

	auto renderer2D = new Renderer2D(m_ScreenWidth, m_ScreenHeight);
	Application::Instance()->PushLayer(new Layer2D(renderer2D));
	renderer2D->SetRenderToGBufferTexture(true);

	std::vector<std::shared_ptr<Texture2D>> textures = 
	{
		std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("Test", "/CoreTextures/icon.png")),
		std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("Test2", "/CoreTextures/noise.png")),
		std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("Test3", "/CoreTextures/checkerboard.tga")),
		std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("Test4", "/CoreTextures/water/waterDUDV.png")),
		//std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("Test5", "/CoreTextures/water/waterNoise.png"))
	};
    for (int i = 0; i < 5; i++)
    {
        std::shared_ptr<Entity> testSprite = std::make_shared<Entity>("Sprite", this);

		Vector2 pos(RandomNumberGenerator32::Rand(-5.0f, 5.0f), RandomNumberGenerator32::Rand(-5.0f, 5.0f));
		Vector2 size(RandomNumberGenerator32::Rand(1.0f, 2.0f), RandomNumberGenerator32::Rand(1.0f, 2.0f));

		std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(textures[(int)RandomNumberGenerator32::Rand(0.0f, 1.0f)],pos,size, maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f));
		testSprite->AddComponent(std::make_unique<SpriteComponent>(sprite));
		//test->SetIsStatic(true);
		PhysicsObjectParamaters params;
		params.position = Vector3(pos, 1.0f);
		params.scale = Vector3(size / 2.0f, 1.0f);
		params.shape = Shape::Square;
		params.isStatic = false;
		std::shared_ptr<PhysicsObject2D> blockPhysics = std::make_shared<PhysicsObject2D>();
		blockPhysics->Init(params);
		testSprite->AddComponent(std::make_unique<Physics2DComponent>(blockPhysics));
        AddEntity(testSprite);
    }

    std::shared_ptr<Entity> testSprite = std::make_shared<Entity>("Sprite", this);

    std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(maths::Vector2(-1.0f,0.0f), maths::Vector2(1.0f,1.0f), maths::Vector4(0.4f,0.1f,0.6f,1.0f));
    testSprite->AddComponent(std::make_unique<SpriteComponent>(sprite));
    AddEntity(testSprite);

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
