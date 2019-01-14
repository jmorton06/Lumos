#pragma once
#include <LumosEngine.h>

class CubeGame : public Lumos::Scene
{
public:
	CubeGame(const std::string& SceneName);
	virtual ~CubeGame();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;

	void LoadModels();

protected:
};

