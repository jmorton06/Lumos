#pragma once
#include <JMEngine.h>

class Scene3D : public jm::Scene
{
public:
	explicit Scene3D(const String& SceneName);
	virtual ~Scene3D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(jm::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;
	void LoadModels();
};
