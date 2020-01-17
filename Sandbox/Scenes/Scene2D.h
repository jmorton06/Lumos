#pragma once
#include <LumosEngine.h>

class Scene2D : public Lumos::Scene
{
public:
	explicit Scene2D(const String& SceneName);
	virtual ~Scene2D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
	virtual void OnImGui() override;

	void LoadSprites();

	void Reset();
	void CreatePillar(int index, float offset);

	std::vector<entt::entity> m_Pillars;

	enum GameState 
	{
		Running,
		GameOver
	};

	GameState m_GameState = Running;

	u32 m_Score = 0;
	float m_PillarTarget = 30.0f;
	int m_PillarIndex = 0;
};
