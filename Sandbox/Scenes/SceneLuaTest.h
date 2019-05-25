#pragma once
#include "LumosEngine.h"

class SceneLuaTest : public lumos::Scene
{
public:
	explicit SceneLuaTest(const String& SceneName);
	virtual ~SceneLuaTest();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(lumos::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;
};
