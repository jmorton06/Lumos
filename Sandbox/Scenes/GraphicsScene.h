#pragma once
#include <JMEngine.h>

class GraphicsScene : public jm::Scene
{
public:
	GraphicsScene(const std::string& SceneName);
	virtual ~GraphicsScene();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(jm::TimeStep* timeStep) override;
	virtual void OnIMGUI() override;

	void LoadModels();

	int moonID;

protected:
};
