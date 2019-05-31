#include "Scene2D.h"

using namespace lumos;
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

	B2PhysicsEngine::Instance()->SetPaused(true);

	SetDebugDrawFlags(DEBUGDRAW_FLAGS_ENTITY_COMPONENTS | DEBUGDRAW_FLAGS_COLLISIONVOLUMES);

	m_pCamera = new Camera2D(16, 9 , 1.0f);


	m_SceneBoundingRadius = 20.0f;

	Application::Instance()->GetAudioManager()->SetListener(m_pCamera);

	auto renderer2D = new graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight);
	auto layer2D = new Layer2D(renderer2D);

	m_SceneLayers.emplace_back(layer2D);
	Application::Instance()->PushLayer(layer2D);
	renderer2D->SetRenderToGBufferTexture(true);

	std::vector<std::shared_ptr<graphics::Texture2D>> textures =
	{
		std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile("Test", "/CoreTextures/icon.png")),
		std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile("Test2", "/CoreTextures/noise.png")),
		std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile("Test3", "/CoreTextures/checkerboard.tga")),
		std::shared_ptr<graphics::Texture2D>(graphics::Texture2D::CreateFromFile("Test4", "/CoreTextures/water/waterDUDV.png"))
	};
    for (int i = 0; i < 1000; i++)
    {
        std::shared_ptr<Entity> testSprite = std::make_shared<Entity>("Sprite");

		Vector2 pos(RandomNumberGenerator32::Rand(-5.0f, 10.0f), RandomNumberGenerator32::Rand(-5.0f, 500.0f));
		Vector2 size(RandomNumberGenerator32::Rand(1.0f, 3.0f), RandomNumberGenerator32::Rand(1.0f, 3.0f));
		int textureID = static_cast<int>(RandomNumberGenerator32::Rand(0.0f, 4.0f));
		auto colour = maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);
		std::shared_ptr<graphics::Sprite> sprite = std::make_shared<graphics::Sprite>(textures[textureID],pos,size,colour);
		sprite->SetPosition(size / -2.0f);
    	testSprite->AddComponent<SpriteComponent>(sprite);
		//test->SetIsStatic(true);
		PhysicsObjectParamaters params;
		params.position = Vector3(pos, 1.0f);
		params.scale = Vector3(size / 2.0f, 1.0f);
		params.shape = Shape::Square;
		params.isStatic = false;
		std::shared_ptr<PhysicsObject2D> blockPhysics = std::make_shared<PhysicsObject2D>();
		blockPhysics->Init(params);
		testSprite->AddComponent<Physics2DComponent>(blockPhysics);
        AddEntity(testSprite);
    }

    std::shared_ptr<Entity> testSprite = std::make_shared<Entity>("Sprite");

    std::shared_ptr<graphics::Sprite> sprite = std::make_shared<graphics::Sprite>(maths::Vector2(-1.0f,0.0f), maths::Vector2(1.0f,1.0f), maths::Vector4(0.4f,0.1f,0.6f,1.0f));
    testSprite->AddComponent<SpriteComponent>(sprite);
    AddEntity(testSprite);

	std::shared_ptr<Entity> groundSprite = std::make_shared<Entity>("Sprite");
	std::shared_ptr<graphics::Sprite> ground = std::make_shared<graphics::Sprite>(maths::Vector2(-25.0f, -5.0f), maths::Vector2(50.0f, 10.0f), maths::Vector4(0.4f, 0.1f, 0.6f, 1.0f));
	groundSprite->AddComponent<SpriteComponent>(ground);
	PhysicsObjectParamaters groundParams;
	groundParams.position = Vector3(0.0f, -20.0f, 1.0f);
	groundParams.scale = Vector3(25.0f, 5.0f, 1.0f);
	groundParams.shape = Shape::Square;
	groundParams.isStatic = true;
	std::shared_ptr<PhysicsObject2D> groundPhysics = std::make_shared<PhysicsObject2D>();
	groundPhysics->Init(groundParams);
	groundSprite->AddComponent<Physics2DComponent>(groundPhysics);
	AddEntity(groundSprite);

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
}
