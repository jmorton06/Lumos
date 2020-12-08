#pragma once
#include <LumosEngine.h>

class GraphicsScene : public Lumos::Scene
{
public:
	GraphicsScene(const std::string& SceneName);
	virtual ~GraphicsScene();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(const Lumos::TimeStep& timeStep) override;

	void LoadModels();
    
private:
};
