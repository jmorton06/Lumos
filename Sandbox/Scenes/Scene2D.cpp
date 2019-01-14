#include "Scene2D.h"

using namespace Lumos;
using namespace maths;

Player2D::Player2D(std::shared_ptr<Texture2D> texture, const Vector2& position, const Vector2& scale, const Vector4& colour, float colourMix,Scene* scene)
	: Entity(scene)
{
	std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(texture, position, scale, colour, colourMix);
	AddComponent(std::make_unique<SpriteComponent>(sprite));
}

Player2D::~Player2D() = default;

void Player2D::Update(float dt) const
{
	const float speed = 3.0f * dt;

	Physics2DComponent* physicsComponent = GetComponent<Physics2DComponent>();

	if (physicsComponent)
	{
		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_LEFT))
		{
			physicsComponent->m_PhysicsObject->SetLinearVelocity(Vector2(-1000.0f * speed, 0.0f));
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_RIGHT))
		{
			physicsComponent->m_PhysicsObject->SetLinearVelocity(Vector2(1000.0f * speed, 0.0f));
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_UP))
		{
			physicsComponent->m_PhysicsObject->SetLinearVelocity(Vector2(0.0f, 100.0f * speed));
		}
	}
}

Block::Block(std::shared_ptr<Texture2D> texture, const Vector2& position, const Vector2& scale, const Vector4& colour, float colourMix, Scene* scene) : Entity(scene)
{
	std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(texture, position, scale, colour, colourMix);
	AddComponent(std::make_unique<SpriteComponent>(sprite));
}

Block::Block(Scene* scene) : Entity(scene)
{
}

Block::~Block() = default;

void Block::Update(float dt) const
{

}

int Block::GetHealth() const
{
    return m_Health;
}

Ball::Ball(std::shared_ptr<Texture2D> texture, const Vector2& position, const Vector2& scale, const Vector4& colour, float colourMix, Scene* scene) : Entity(scene)
{
	std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(texture, position, scale, colour, colourMix);
	AddComponent(std::make_unique<SpriteComponent>(sprite));
}

Ball::~Ball() = default;

void Ball::Update(float dt) const
{

}

Scene2D::Scene2D(const String& SceneName) : Scene(SceneName)
{
}


Scene2D::~Scene2D()
{
}


void Scene2D::OnInit()
{
	Scene::OnInit();

	m_pCamera = new Camera2D(m_ScreenWidth,
                             m_ScreenHeight, 
							 (float) m_ScreenWidth / (float) m_ScreenHeight
                            , 32);

	SetUseShadow(false);

	SoundSystem::Instance()->SetListener(m_pCamera);

	AddBreakoutLevel(m_pCamera->GetScale());
}

void Scene2D::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
	m_Player->Update(timeStep->GetSeconds());
}

void Scene2D::Render2D()
{
	std::stringstream fpsSS;
	fpsSS << "FPS: ";
	fpsSS << Engine::Instance()->GetFPS();

	RenderString(fpsSS.str(), Vector2(0.9f, 0.9f), 0.6f, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Scene2D::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		if (m_pCamera)
		{
			delete m_pCamera;
			m_pCamera = nullptr;
		}
	}

	Scene::OnCleanupScene();
}

void Scene2D::AddBreakoutLevel(int cameraScale)
{
    //Level
	const TextureLoadOptions options(false, true);

	std::shared_ptr<Texture2D> blockTex  = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("block", "/Textures/block.png", TextureParameters(TextureWrap::CLAMP), options));
	std::shared_ptr<Texture2D> solidTex  = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("solidblockTex", "/Textures/block_solid.png", TextureParameters(TextureWrap::CLAMP), options));
	std::shared_ptr<Texture2D> spriteTex = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("playerTex", "/Textures/playerTex.png", TextureParameters(TextureWrap::CLAMP), options));

	m_Player = std::make_shared<Player2D>(spriteTex, Vector2(20.0f, 0.5f), Vector2(6.0f, 0.7f), Vector4(1.0f, 0.0f, 1.0f, 1.0f), 0.0f,this);
	std::shared_ptr<PhysicsObject2D> playerPhysics = std::make_shared<PhysicsObject2D>();
	PhysicsObjectParamaters params;
	params.position = maths::Vector3(Vector2(20.0f, 0.5f), 1.0f);
	params.scale = maths::Vector3(Vector2(6.0f, 0.7f), 1.0f);
	params.shape = Shape::Square;
	playerPhysics->Init(params);
	m_Player->AddComponent(std::make_unique<Physics2DComponent>(playerPhysics));
	AddEntity(m_Player);


	std::vector<std::vector<int>> tileData = {	{ 1, 1, 1, 1, 1, 1 },
    											{ 2, 2, 0, 0, 2, 2 },
    											{ 3, 3, 4, 4, 3, 3 } };

    const uint height = static_cast<uint>(tileData.size());
    const uint width  = static_cast<uint>(tileData[0].size());

    const float unit_width  = ((float) m_ScreenWidth / m_pCamera->GetScale()) / width;
	const float unit_height = ((float) m_ScreenHeight / m_pCamera->GetScale()) / (2.0f * height);

    for (uint y = 0; y < height; ++y)
    {
    	for (uint x = 0; x < width; ++x)
    	{
    		if (tileData[y][x] == 1) // Solid
    		{
				Vector2 pos(unit_width * x + unit_width / 2.0f, unit_height * y + 10.0f + unit_height );
    			Vector2 size(unit_width, unit_height );
				std::shared_ptr<Block> test = std::make_shared<Block>(blockTex, pos, size / 2.0f, Vector4(0.8f, 0.8f, 0.7f,1.0f), 0.8f,this);
				//test->SetIsStatic(true);
				PhysicsObjectParamaters params;
				params.position = Vector3(pos, 1.0f);
				params.scale	= Vector3(size / 2.0f, 1.0f);
				params.shape	= Shape::Square;
				params.isStatic = true;
				std::shared_ptr<PhysicsObject2D> blockPhysics = std::make_shared<PhysicsObject2D>();
				blockPhysics->Init(params);
				test->AddComponent(std::make_unique<Physics2DComponent>(blockPhysics));
				AddEntity(test);
    		}
    		else if (tileData[y][x] > 1)
    		{
    			Vector4 colour = Vector4(1.0f); // original: white
    			if (tileData[y][x] == 2)
    				colour = Vector4(0.2f, 0.6f, 1.0f, 1.0f);
    			else if (tileData[y][x] == 3)
    				colour = Vector4(0.0f, 0.7f, 0.0f, 1.0f);
    			else if (tileData[y][x] == 4)
    				colour = Vector4(0.8f, 0.8f, 0.4f, 1.0f);
    			else if (tileData[y][x] == 5)
    				colour = Vector4(1.0f, 0.5f, 0.0f, 1.0f);

				Vector2 pos(unit_width * x + unit_width / 2.0f, unit_height * y + 10.0f + unit_height);
    			Vector2 size(unit_width, unit_height);
				std::shared_ptr<Block> test = std::make_shared<Block>(solidTex, pos, size / 2.0f, colour, 0.8f,this);
				PhysicsObjectParamaters params;
				params.position = Vector3(pos, 1.0f);
				params.scale	= Vector3(size / 2.0f, 1.0f);
				params.shape	= Shape::Square;
				params.isStatic = true;
				std::shared_ptr<PhysicsObject2D> blockPhysics = std::make_shared<PhysicsObject2D>();
				blockPhysics->Init(params);
				test->AddComponent(std::make_unique<Physics2DComponent>(blockPhysics));
				AddEntity(test);
    		}
    	}
    }

	std::shared_ptr<Texture2D> circleTex = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile("circleTex", "/Textures/circle.png", TextureParameters(TextureWrap::CLAMP), options));

	std::shared_ptr<Ball> ball = std::make_shared<Ball>(circleTex, Vector2(20.0f, 2.0f), Vector2(1.0f, 1.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,this);
	PhysicsObjectParamaters circleParams;
	params.position = Vector3(Vector2(20.0f, 2.0f), 1.0f);
	params.scale = Vector3(Vector2(1.0f, 1.0f), 1.0f);
	params.shape = Shape::Circle;
	params.isStatic = false;
	std::shared_ptr<PhysicsObject2D> ballPhysics = std::make_shared<PhysicsObject2D>();
	ballPhysics->Init(params);
	ball->AddComponent(std::make_unique<Physics2DComponent>(ballPhysics));
	AddEntity(ball);


	std::shared_ptr<Block> ground = std::make_shared<Block>(this);
	PhysicsObjectParamaters groundParams;
	groundParams.position = Vector3(0.0f, -10.0f, 1.0f);
	groundParams.scale = Vector3(500.0f, 10.0f, 1.0f);
	groundParams.shape = Shape::Square;
	groundParams.isStatic = true;
	std::shared_ptr<PhysicsObject2D> groundPhysics = std::make_shared<PhysicsObject2D>();
	groundPhysics->Init(groundParams);
	ground->AddComponent(std::make_unique<Physics2DComponent>(groundPhysics));
	AddEntity(ground);

	std::shared_ptr<Block> left = std::make_shared<Block>(this);
	PhysicsObjectParamaters leftParams;
	leftParams.position = Vector3(-1.0f, m_ScreenHeight / (2.0f * m_pCamera->GetScale()), 1.0f);
	leftParams.scale = Vector3(1.0f, m_ScreenHeight / (2.0f * m_pCamera->GetScale()), 1.0f);
	leftParams.shape = Shape::Square;
	leftParams.isStatic = true;
	std::shared_ptr<PhysicsObject2D> leftWallPhysics = std::make_shared<PhysicsObject2D>();
	leftWallPhysics->Init(leftParams);
	left->AddComponent(std::make_unique<Physics2DComponent>(leftWallPhysics));
	AddEntity(left);

	std::shared_ptr<Block> right = std::make_shared<Block>(this);
	PhysicsObjectParamaters rightParams;
	rightParams.position = Vector3(m_ScreenWidth / (m_pCamera->GetScale()) + 1.0f, m_ScreenHeight / (2.0f * m_pCamera->GetScale()), 1.0f);
	rightParams.scale = Vector3(1.0f, m_ScreenHeight / (2.0f * m_pCamera->GetScale()), 1.0f);
	rightParams.shape = Shape::Square;
	rightParams.isStatic = true;
	std::shared_ptr<PhysicsObject2D> rightWallPhysics = std::make_shared<PhysicsObject2D>();
	rightWallPhysics->Init(rightParams);
	right->AddComponent(std::make_unique<Physics2DComponent>(rightWallPhysics));
	AddEntity(right);
}

void Scene2D::OnIMGUI()
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
