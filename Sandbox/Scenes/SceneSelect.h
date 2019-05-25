#pragma once
#include <LumosEngine.h>

class SceneSelect : public lumos::Scene
{
public:
	explicit SceneSelect(const String& SceneName);
	virtual ~SceneSelect();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	//virtual void OnUpdate(lumos::TimeStep* timeStep) override;
	//virtual void Render2D() override;
	//virtual void Controls() override;
	virtual void OnIMGUI() override;

private:
    std::vector<String> m_SceneNames;
};
