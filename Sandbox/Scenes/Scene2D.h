#pragma once
#include <LumosEngine.h>

class Scene2D : public lumos::Scene
{
public:
	explicit Scene2D(const String& SceneName);
	virtual ~Scene2D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(lumos::TimeStep* timeStep) override;
	virtual void OnIMGUI() override;
};
