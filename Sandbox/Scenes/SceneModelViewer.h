#pragma once

#include <JMEngine.h>

class SceneModelViewer : public jm::Scene
{
public:
	explicit SceneModelViewer(const String& SceneName);
	virtual ~SceneModelViewer();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(jm::TimeStep* timeStep) override;
	virtual void OnIMGUI() override;
	void LoadModels();
};
