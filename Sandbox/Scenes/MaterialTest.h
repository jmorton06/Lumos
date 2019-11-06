#pragma once
#include <LumosEngine.h>

class MaterialTest : public Lumos::Scene
{
public:
	MaterialTest(const String& SceneName);
	virtual ~MaterialTest();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnImGui() override;
	void LoadModels();
};
