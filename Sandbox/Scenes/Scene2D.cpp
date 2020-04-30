#include "Scene2D.h"

using namespace Lumos;
using namespace Maths;

#define MAX_HEIGHT 10.0f
#define SPEED 3.0f

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
#ifdef LUMOS_PLATFORM_IOS
		OS::Instance()->Vibrate();
#endif
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

	m_pCamera = new Camera2D(static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight), MAX_HEIGHT);

	LoadLuaScene(ROOT_DIR"/Sandbox/res/scripts/FlappyBirdTest.lua");

	auto cameraEntity = m_Registry.create();
	m_Registry.emplace<CameraComponent>(cameraEntity, m_pCamera);
	m_Registry.emplace<NameComponent>(cameraEntity, "Camera");

	m_SceneBoundingRadius = 20.0f;

	auto audioSystem = Application::Instance()->GetSystem<AudioManager>();
	if (audioSystem)
		Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

	Application::Instance()->PushLayer(new Layer2D(new Graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight, editor, true, false)));
	
	{
		s_player.entity = m_Registry.create();
		s_player.scene = this;
		auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);
		m_Registry.emplace<Graphics::Sprite>(s_player.entity, Maths::Vector2(-1.0f / 2.0f, -1.0f / 2.0f), Maths::Vector2(1.0f, 1.0f), colour);

		PhysicsObjectParamaters params;
		params.position = Vector3({ 1.0f, 1.0f }, 1.0f);
		params.scale = Vector3(1.0f / 2.0f, 1.0f / 2.0f, 1.0f);
		params.shape = Shape::Circle;
		params.isStatic = false;
		auto blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
		blockPhysics->Init(params);
		blockPhysics->GetB2Body()->SetLinearVelocity({SPEED, 0.0f});
		blockPhysics->GetB2Body()->SetUserData(&s_player);

		m_Registry.emplace<Physics2DComponent>(s_player.entity, blockPhysics);
		m_Registry.emplace<Maths::Transform>(s_player.entity, Maths::Vector3(1.0f, 1.0f,0.0f));
	}

	Application::Instance()->GetSystem<B2PhysicsEngine>()->GetB2World()->SetContactListener(&myContactListenerInstance);

	m_Pillars.resize(10);
	for (int i = 0; i < 10; i +=2)
		CreatePillar(i, (i + 3) * 10.0f);
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

		auto pos = m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition();

		if (pos.y > MAX_HEIGHT || pos.y < -MAX_HEIGHT)
			m_GameState = GameOver;

		pos.y = 0.0f;
		m_pCamera->SetPosition(pos);

		m_Score = (uint32_t)((m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition().x - 5.0f) / 10.0f);

		if (m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition().x > m_PillarTarget)
		{
			if (m_Registry.valid(m_Pillars[m_PillarIndex]))
			{
				m_Registry.destroy(m_Pillars[m_PillarIndex]);
				m_Registry.destroy(m_Pillars[m_PillarIndex + 1]);
			}

 			CreatePillar(m_PillarIndex, m_FurthestPillarPosX * 2.0f + 20.0f);
			m_PillarIndex = m_PillarIndex += 2;
			m_PillarIndex = m_PillarIndex % m_Pillars.size();
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
		ImGui::Text("Score : %i", m_Score);
		if (ImGui::Button("Reset"))
		{
			Reset();
		}

		ImGui::End();
	}
	else if(m_GameState == Running)
	{
		ImGui::Begin("Running");
		ImGui::Text("Score : %i", m_Score);
		auto pos = m_Registry.get<Maths::Transform>(s_player.entity).GetWorldPosition();
		ImGui::InputFloat3("Pos", &pos.x);
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

	const float scale = 1.0f;

	for (int i = 0; i < 1; i++)
	{
		const auto& testSprite = m_Registry.create();
        
		Vector2 pos(RandomNumberGenerator32::Rand(-5.0f * scale, 10.0f * scale), RandomNumberGenerator32::Rand(-5.0f * scale, 500.0f * scale));
		Vector2 size(RandomNumberGenerator32::Rand(1.0f / 10.0f * scale, 3.0f* scale), RandomNumberGenerator32::Rand(1.0f * scale , 3.0f * scale));
		int textureID = static_cast<int>(RandomNumberGenerator32::Rand(0.0f, 4.0f));
		auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);

		PhysicsObjectParamaters params;
		params.position = Vector3(pos, 1.0f);
		params.scale = Vector3(size / 2.0f, 1.0f);
		params.shape = Shape::Square;
		params.isStatic = false;
        auto blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
		blockPhysics->Init(params);

        m_Registry.emplace<Graphics::Sprite>(testSprite, textures[textureID], size / -2.0f, size, colour);
        m_Registry.emplace<Physics2DComponent>(testSprite, blockPhysics);
        m_Registry.emplace<Maths::Transform>(testSprite);
	}

	entt::entity groundSprite = m_Registry.create();
	PhysicsObjectParamaters groundParams;
	groundParams.position = Vector3(0.0f, -15.0f , 1.0f);
	groundParams.scale = Vector3(25.0f, 5.0f, 1.0f);
	groundParams.shape = Shape::Square;
	groundParams.isStatic = true;
	Lumos::Ref<PhysicsObject2D> groundPhysics = Lumos::CreateRef<PhysicsObject2D>();
	groundPhysics->Init(groundParams);
    
	//Set Position of sprite to -Scale to align with box2d collider
    m_Registry.emplace<Graphics::Sprite>(groundSprite, Maths::Vector2(-25.0f, -5.0f), Maths::Vector2(50.0f, 10.0f), Maths::Vector4(0.4f, 0.1f, 0.6f, 1.0f));
    m_Registry.emplace<Physics2DComponent>(groundSprite, groundPhysics);
    m_Registry.emplace<Maths::Transform>(groundSprite, Vector3(0.0f, -15.0f, 1.0f));
}

void Scene2D::Reset()
{
	m_GameState = Running;
	auto phys = m_Registry.get<Physics2DComponent>(s_player.entity).GetPhysicsObject();
	phys->SetPosition({ 0.0f, 0.0f });
	phys->SetLinearVelocity({ SPEED, 0.0f });
	phys->SetOrientation(0.0f);
	phys->SetAngularVelocity(0.0f);

	m_FurthestPillarPosX = 0.0f;
	m_PillarTarget = 35.0f;
    m_PillarIndex = 0;

	m_Registry.get<Maths::Transform>(s_player.entity).SetLocalPosition({ 0.0f,0.0f,0.0f });
	m_Registry.get<Maths::Transform>(s_player.entity).UpdateMatrices();

	for (int i = 0; i < 10; i += 2)
	{
		if (m_Registry.valid(m_Pillars[i]))
		{
			m_Registry.destroy(m_Pillars[i]);
			m_Registry.destroy(m_Pillars[i + 1]);
		}

		CreatePillar(i, (i + 3) * 10.0f);
	}
	
}

void Scene2D::CreatePillar(int index, float offset)
{
	auto& pillarTop = m_Pillars[index];
	auto& pillarBottom = m_Pillars[index + 1];
	pillarTop = m_Registry.create();
	pillarBottom = m_Registry.create();

	auto colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), RandomNumberGenerator32::Rand(0.0f, 1.0f), 1.0f);

	const float gapSize = 4.0f;
	float centre = RandomNumberGenerator32::Rand(-6.0f, 6.0f);
	
	//Top Pillar
	float topY = MAX_HEIGHT;
	float bottomY = centre + (gapSize / 2.0f);

	auto pos = Maths::Vector3(offset / 2.0f, ((topY - bottomY )/ 2.0f) + bottomY, 0.0f);
	auto scale = Maths::Vector3(1.0f, (topY - bottomY) / 2.0f, 1.0f);

	m_Registry.emplace<Graphics::Sprite>(pillarTop, -scale.xy(), scale.xy() * 2.0f, colour);

	PhysicsObjectParamaters params;
	params.position = pos;
	params.scale = scale;
	params.shape = Shape::Square;
	params.isStatic = true;
	auto blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
	blockPhysics->Init(params);
	blockPhysics->GetB2Body()->SetLinearVelocity({ 1.0f, 0.0f });

	m_Registry.emplace<Physics2DComponent>(pillarTop, blockPhysics);
	m_Registry.emplace<Maths::Transform>(pillarTop, pos);

	//Bottom Pillar
	topY = centre - (gapSize / 2.0f);
	bottomY = -MAX_HEIGHT;

	pos = Maths::Vector3(offset / 2.0f, ((topY - bottomY) / 2.0f) + bottomY, 0.0f);
	scale = Maths::Vector3(1.0f, (topY - bottomY) / 2.0f, 1.0f);

	m_Registry.emplace<Graphics::Sprite>(pillarBottom, -scale.xy(), scale.xy() * 2.0f, colour);

	params.position = pos;
	params.scale = scale;
	params.shape = Shape::Square;
	params.isStatic = true;
	blockPhysics = Lumos::CreateRef<PhysicsObject2D>();
	blockPhysics->Init(params);
	blockPhysics->GetB2Body()->SetLinearVelocity({ 1.0f, 0.0f });

	m_Registry.emplace<Physics2DComponent>(pillarBottom, blockPhysics);
	m_Registry.emplace<Maths::Transform>(pillarBottom, pos);

	if (pos.x > m_FurthestPillarPosX)
		m_FurthestPillarPosX = pos.x;
}
