#pragma once
#include <JMEngine.h>

class SceneLuaTest : public jm::Scene
{
public:
	explicit SceneLuaTest(const String& SceneName);
	virtual ~SceneLuaTest();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(jm::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;
};
