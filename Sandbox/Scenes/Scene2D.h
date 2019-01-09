#pragma once
#include <JMEngine.h>

class Player2D : public jm::Entity
{
public:
	Player2D(std::shared_ptr<jm::Texture2D> texture, const jm::maths::Vector2& position, const jm::maths::Vector2& scale, const jm::maths::Vector4& colour, float colourMix, jm::Scene* scene);
	~Player2D();

	void Update(float dt) const;

};

class Block : public jm::Entity
{
public:
	Block(std::shared_ptr<jm::Texture2D> texture, const jm::maths::Vector2& position, const jm::maths::Vector2& scale, const jm::maths::Vector4& colour, float colourMix, jm::Scene* scene);
	Block(jm::Scene* scene);
	~Block();

	void Update(float dt) const;


	int m_Health = 1;

	int GetHealth() const;
};

class Ball : public jm::Entity
{
public:
	Ball(std::shared_ptr<jm::Texture2D> texture, const jm::maths::Vector2& position, const jm::maths::Vector2& scale, const jm::maths::Vector4& colour, float colourMix, jm::Scene* scene);
    ~Ball();

    void Update(float dt) const;
};

class Scene2D : public jm::Scene
{
public:
	explicit Scene2D(const String& SceneName);
	virtual ~Scene2D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(jm::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;

	void AddBreakoutLevel(int cameraScale);

	std::shared_ptr<Player2D> m_Player;
	std::vector<jm::Sprite*> m_Blocks;
	jm::Sprite* m_Ball;
};

