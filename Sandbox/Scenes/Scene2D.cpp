#include "Scene2D.h"

using namespace Lumos;
using namespace Maths;

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

	Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetPaused(true);

	m_pCamera = new Camera2D(16, 9, 0.4f);
    m_pCamera->SetPosition({-16.0f,5.0f,0.0f});

	auto cameraEntity = EntityManager::Instance()->CreateEntity("Camera");
	cameraEntity->AddComponent<CameraComponent>(m_pCamera);
	AddEntity(cameraEntity);

	m_SceneBoundingRadius = 20.0f;

	Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	LoadSprites();

	Application::Instance()->PushLayer(new Layer2D(new Graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight, true)));
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

void Scene2D::OnImGui()
{
}

void Scene2D::LoadSprites()
{
	std::vector<Lumos::Ref<Graphics::Texture2D>> textures =
	{
		Lumos::Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("Test", "/CoreTextures/icon.png")),
		Lumos::Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("Test2", "/CoreTextures/noise.png")),
		Lumos::Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("Test3", "/CoreTextures/checkerboard.tga")),
		Lumos::Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("Test4", "/CoreTextures/water/waterDUDV.png"))
	};

	for (int i = 0; i < 1000; i++)
	{
		auto testSprite = EntityManager::Instance()->CreateEntity("Sprite" + StringFormat::ToString(i));

		Vector2 pos(RandomNumberGenerator32::Rand(-5.0f, 10.0f), RandomNumberGenerator32::Rand(-5.0f, 500.0f));
		Vector2 size(RandomNumberGenerator32::Rand(1.0f, 3.0f), RandomNumberGenerator32::Rand(1.0f, 3.0f));
		int textureID = static_cast<int>(RandomNumberGenerator32::Rand(0.0f, 4.0f));
		auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);
		Lumos::Ref<Graphics::Sprite> sprite = Lumos::CreateRef<Graphics::Sprite>(textures[textureID], pos, size, colour);
		sprite.get()->SetPosition(size / -2.0f);

		//test->SetIsStatic(true);
		PhysicsObjectParamaters params;
		params.position = Vector3(pos, 1.0f);
		params.scale = Vector3(size / 2.0f, 1.0f);
		params.shape = Shape::Square;
		params.isStatic = false;
		Lumos::Ref<PhysicsObject2D> blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
		blockPhysics->Init(params);

		testSprite->AddComponent<SpriteComponent>(sprite);
		testSprite->AddComponent<Physics2DComponent>(blockPhysics);
		testSprite->AddComponent<TransformComponent>();
		AddEntity(testSprite);
	}

	auto testSprite = EntityManager::Instance()->CreateEntity("SpriteTest");

	Lumos::Ref<Graphics::Sprite> sprite = Lumos::CreateRef<Graphics::Sprite>(Maths::Vector2(-1.0f, 0.0f), Maths::Vector2(1.0f, 1.0f), Maths::Vector4(0.4f, 0.1f, 0.6f, 1.0f));
	testSprite->AddComponent<SpriteComponent>(sprite);
	testSprite->AddComponent<TransformComponent>();

	AddEntity(testSprite);

	auto groundSprite = EntityManager::Instance()->CreateEntity("Ground");
	Lumos::Ref<Graphics::Sprite> ground = Lumos::CreateRef<Graphics::Sprite>(Maths::Vector2(-25.0f, -5.0f), Maths::Vector2(50.0f, 10.0f), Maths::Vector4(0.4f, 0.1f, 0.6f, 1.0f));
	groundSprite->AddComponent<SpriteComponent>(ground);
	PhysicsObjectParamaters groundParams;
	groundParams.position = Vector3(0.0f, -20.0f, 1.0f);
	groundParams.scale = Vector3(25.0f, 5.0f, 1.0f);
	groundParams.shape = Shape::Square;
	groundParams.isStatic = true;
	Lumos::Ref<PhysicsObject2D> groundPhysics = Lumos::CreateRef<PhysicsObject2D>();
	groundPhysics->Init(groundParams);
	groundSprite->AddComponent<Physics2DComponent>(groundPhysics);
	groundSprite->AddComponent<TransformComponent>();

	AddEntity(groundSprite);
}
