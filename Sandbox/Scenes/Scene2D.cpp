#include "Scene2D.h"

using namespace Lumos;
using namespace Maths;

#define MAX_HEIGHT 10.0f
#define MIN_HEIGHT -10.0f

Scene2D::Scene2D(const String& SceneName)
	: Scene(SceneName)
{
}

Scene2D::~Scene2D()
{
}

struct player
{
	entt::entity entity;
	Scene2D* scene;
	void Reset()
	{
		Lumos::Debug::Log::Info("Collision"); 
		scene->m_GameState = Scene2D::GameState::GameOver;
	}
};

static player s_player;

class MyContactListener : public b2ContactListener
{
	void BeginContact(b2Contact* contact) {

		void* bodyUserData = contact->GetFixtureA()->GetBody()->GetUserData();
		if (bodyUserData)
			static_cast<player*>(bodyUserData)->Reset();

		bodyUserData = contact->GetFixtureB()->GetBody()->GetUserData();
		if (bodyUserData)
			static_cast<player*>(bodyUserData)->Reset();

	}

	void EndContact(b2Contact* contact) {


	}
};

static MyContactListener myContactListenerInstance;

void Scene2D::OnInit()
{
	Scene::OnInit();

	Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetPaused(true);

	m_pCamera = new Camera2D(static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight), 2.0f);

	auto cameraEntity = m_Registry.create();
	m_Registry.assign<CameraComponent>(cameraEntity, m_pCamera);
	m_Registry.assign<NameComponent>(cameraEntity, "Camera");

	m_SceneBoundingRadius = 20.0f;

	auto audioSystem = Application::Instance()->GetSystem<AudioManager>();
	if (audioSystem)
		Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	LoadSprites();

	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

	Application::Instance()->PushLayer(new Layer2D(new Graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight, editor)));

	s_player.entity = m_Registry.create();
	s_player.scene = this;
	auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);
	m_Registry.assign<Graphics::Sprite>(s_player.entity, Maths::Vector2(-1.0f / 2.0f, -1.0f / 2.0f), Maths::Vector2(1.0f, 1.0f), colour);

	PhysicsObjectParamaters params;
	params.position = Vector3({ 1.0f, 1.0f }, 1.0f);
	params.scale = Vector3(1.0f / 2.0f, 1.0f / 2.0f, 1.0f);
	params.shape = Shape::Square;
	params.isStatic = false;
	auto blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
	blockPhysics->Init(params);
	blockPhysics->GetB2Body()->SetLinearVelocity({1.0f, 0.0f});
	blockPhysics->GetB2Body()->SetUserData(&s_player);

	m_Registry.assign<Physics2DComponent>(s_player.entity, blockPhysics);
	m_Registry.assign<Maths::Transform>(s_player.entity, Maths::Vector3(1.0f, 1.0f,0.0f));

	Application::Instance()->GetSystem<B2PhysicsEngine>()->GetB2World()->SetContactListener(&myContactListenerInstance);

	m_Pillars.resize(10);
	for (int i = 0; i < 10; i+=2)
		CreatePillar(i, i * 10.0f);
}



void Scene2D::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);

	if (m_GameState == Running)
	{
		auto phys = m_Registry.get<Physics2DComponent>(s_player.entity);

		Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0);

		auto m_CameraSpeed = 1.0f;
		auto m_Velocity = Maths::Vector3(0.0f, 0.0f);

		if (Input::GetInput()->GetKeyPressed(LUMOS_KEY_SPACE))
		{
			m_Velocity += up * m_CameraSpeed * 400.0f;
		}

		phys.GetPhysicsObject()->GetB2Body()->ApplyForce({ m_Velocity.x, m_Velocity.y }, phys.GetPhysicsObject()->GetB2Body()->GetPosition(), true);

		m_pCamera->SetPosition(m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition());

		m_Score = (uint32_t)(m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition().x + 10.0f) / 10.0f;

		if (m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition().x > m_PillarTarget)
		{
			if (m_Registry.valid(m_Pillars[m_PillarIndex]))
			{
				m_Registry.destroy(m_Pillars[m_PillarIndex]);
				m_Registry.destroy(m_Pillars[m_PillarIndex + 1]);
			}

			CreatePillar(m_PillarIndex, m_PillarTarget + 20.0f);
			m_PillarIndex = m_PillarIndex += 2 % m_Pillars.size();
			m_PillarTarget += 10.0f;
		}
	}

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
	if (m_GameState == GameOver)
	{
		ImGui::Begin("GameOver");
		ImGui::TextUnformatted("GameOver");
		if (ImGui::Button("Reset"))
		{
			Reset();
		}

		ImGui::End();
	}
	
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

	//for (int i = 0; i < 1; i++)
	//{
	//	const auto& testSprite = m_Registry.create();
 //       
	//	Vector2 pos(RandomNumberGenerator32::Rand(-5.0f * scale, 10.0f * scale), RandomNumberGenerator32::Rand(-5.0f * scale, 500.0f * scale));
	//	Vector2 size(RandomNumberGenerator32::Rand(1.0f / 10.0f * scale, 3.0f* scale), RandomNumberGenerator32::Rand(1.0f * scale , 3.0f * scale));
	//	int textureID = static_cast<int>(RandomNumberGenerator32::Rand(0.0f, 4.0f));
	//	auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);

	//	PhysicsObjectParamaters params;
	//	params.position = Vector3(pos, 1.0f);
	//	params.scale = Vector3(size / 2.0f, 1.0f);
	//	params.shape = Shape::Square;
	//	params.isStatic = false;
 //       auto blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
	//	blockPhysics->Init(params);

 //       m_Registry.assign<Graphics::Sprite>(testSprite, textures[textureID], size / -2.0f, size, colour);
 //       m_Registry.assign<Physics2DComponent>(testSprite, blockPhysics);
 //       m_Registry.assign<Maths::Transform>(testSprite);
	//}

	entt::entity groundSprite = m_Registry.create();
	PhysicsObjectParamaters groundParams;
	groundParams.position = Vector3(0.0f, -15.0f , 1.0f);
	groundParams.scale = Vector3(25.0f, 5.0f, 1.0f);
	groundParams.shape = Shape::Square;
	groundParams.isStatic = true;
	Lumos::Ref<PhysicsObject2D> groundPhysics = Lumos::CreateRef<PhysicsObject2D>();
	groundPhysics->Init(groundParams);
    
	//Set Position of sprite to -Scale to align with box2d collider
    m_Registry.assign<Graphics::Sprite>(groundSprite, Maths::Vector2(-25.0f, -5.0f), Maths::Vector2(50.0f, 10.0f), Maths::Vector4(0.4f, 0.1f, 0.6f, 1.0f));
    m_Registry.assign<Physics2DComponent>(groundSprite, groundPhysics);
    m_Registry.assign<Maths::Transform>(groundSprite, Maths::Matrix4());
}

void Scene2D::Reset()
{
	m_GameState = Running;
	m_Registry.get<Physics2DComponent>(s_player.entity).GetPhysicsObject()->SetPosition({ 0.0f, 0.0f });
	m_Registry.get<Physics2DComponent>(s_player.entity).GetPhysicsObject()->SetLinearVelocity({ 1.0f, 0.0f });
}

void Scene2D::CreatePillar(int index, float offset)
{
	auto& pillarTop = m_Pillars[index];
	auto& pillarBottom = m_Pillars[index + 1];
	pillarTop = m_Registry.create();
	pillarBottom = m_Registry.create();

	float gapSize = RandomNumberGenerator32::Rand(2.0f, 4.0f);
	float gapPos = RandomNumberGenerator32::Rand(-3.0f, 3.0f);
	float bottomScaleY = gapPos - gapSize / 2.0f -MIN_HEIGHT;

	auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);
	auto pos = Maths::Vector3(offset / 4.0f, MIN_HEIGHT - bottomScaleY/2.0f ,0.0f );
	auto scale = Maths::Vector3(1.0f, bottomScaleY, 1.0f);

	float center = RandomNumberGenerator32::Rand(0.5f, 1.0f) * 35.0f - 17.5f;
	float gap = 2.0f + RandomNumberGenerator32::Rand(0.0f, 1.0f);// *5.0f;

	auto ty = MAX_HEIGHT - ((MAX_HEIGHT - center) * 0.2f) + gap * 0.5f;
	auto by = MIN_HEIGHT - ((MIN_HEIGHT - center) * 0.2f) - gap * 0.5f;

	scale.y = by - MIN_HEIGHT;
	pos.y = ty;// (by - MIN_HEIGHT) / 2.0f;

	m_Registry.assign<Graphics::Sprite>(pillarTop, scale.xy() / 2.0f, scale.xy(), colour);

	PhysicsObjectParamaters params;
	params.position = pos;
	params.scale = scale / 2.0f;
	params.shape = Shape::Square;
	params.isStatic = true;
	auto blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
	blockPhysics->Init(params);
	blockPhysics->GetB2Body()->SetLinearVelocity({ 1.0f, 0.0f });

	m_Registry.assign<Physics2DComponent>(pillarTop, blockPhysics);
	m_Registry.assign<Maths::Transform>(pillarTop, pos);


	scale.y = 20.0f;// by - MIN_HEIGHT;
	pos.y = by;// (by - MIN_HEIGHT) / 2.0f;

	m_Registry.assign<Graphics::Sprite>(pillarBottom, (scale.xy() / 2.0f), scale.xy(), colour);

	params.position = pos;
	params.scale = scale / 2.0f;
	params.shape = Shape::Square;
	params.isStatic = true;
	blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
	blockPhysics->Init(params);
	blockPhysics->GetB2Body()->SetLinearVelocity({ 1.0f, 0.0f });

	m_Registry.assign<Physics2DComponent>(pillarBottom, blockPhysics);
	m_Registry.assign<Maths::Transform>(pillarBottom, pos);
}
