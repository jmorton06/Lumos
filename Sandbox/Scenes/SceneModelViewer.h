#pragma once

#include <LumosEngine.h>

class SceneModelViewer : public Lumos::Scene
{
public:
	explicit SceneModelViewer(const String& SceneName);
	virtual ~SceneModelViewer();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
	virtual void OnIMGUI() override;
	void LoadModels();
};
