#pragma once
#include <LumosEngine.h>

class Player2D : public Lumos::Entity
{
public:
	Player2D(std::shared_ptr<Lumos::Texture2D> texture, const Lumos::maths::Vector2& position, const Lumos::maths::Vector2& scale, const Lumos::maths::Vector4& colour, float colourMix, Lumos::Scene* scene);
	~Player2D();

	void Update(float dt) const;

};

class Block : public Lumos::Entity
{
public:
	Block(std::shared_ptr<Lumos::Texture2D> texture, const Lumos::maths::Vector2& position, const Lumos::maths::Vector2& scale, const Lumos::maths::Vector4& colour, float colourMix, Lumos::Scene* scene);
	Block(Lumos::Scene* scene);
	~Block();

	void Update(float dt) const;


	int m_Health = 1;

	int GetHealth() const;
};

class Ball : public Lumos::Entity
{
public:
	Ball(std::shared_ptr<Lumos::Texture2D> texture, const Lumos::maths::Vector2& position, const Lumos::maths::Vector2& scale, const Lumos::maths::Vector4& colour, float colourMix, Lumos::Scene* scene);
    ~Ball();

    void Update(float dt) const;
};

class Scene2D : public Lumos::Scene
{
public:
	explicit Scene2D(const String& SceneName);
	virtual ~Scene2D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;

	void AddBreakoutLevel(int cameraScale);

	std::shared_ptr<Player2D> m_Player;
	std::vector<Lumos::Sprite*> m_Blocks;
	Lumos::Sprite* m_Ball;
};

