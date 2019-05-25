#pragma once

#include <LumosEngine.h>

class SceneModelViewer : public lumos::Scene
{
public:
	explicit SceneModelViewer(const String& SceneName);
	virtual ~SceneModelViewer();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(lumos::TimeStep* timeStep) override;
	virtual void OnIMGUI() override;
	void LoadModels();
};
