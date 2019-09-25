#pragma once
#include <LumosEngine.h>

class Scene3D : public Lumos::Scene
{
public:
	Scene3D(const String& SceneName);
	virtual ~Scene3D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnImGui() override;
	void LoadModels();
};
