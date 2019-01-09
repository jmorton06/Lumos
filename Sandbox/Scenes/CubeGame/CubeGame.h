#pragma once
#include <JMEngine.h>

class CubeGame : public jm::Scene
{
public:
	CubeGame(const std::string& SceneName);
	virtual ~CubeGame();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(jm::TimeStep* timeStep) override;

	void LoadModels();

protected:
};

