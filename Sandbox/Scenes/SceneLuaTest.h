#pragma once
#include <LumosEngine.h>

class SceneLuaTest : public Lumos::Scene
{
public:
	explicit SceneLuaTest(const String& SceneName);
	virtual ~SceneLuaTest();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;
};
