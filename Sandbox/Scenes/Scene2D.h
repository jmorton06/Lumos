#pragma once
#include <LumosEngine.h>

class Scene2D : public Lumos::Scene
{
public:
	explicit Scene2D(const String& SceneName);
	virtual ~Scene2D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(Lumos::TimeStep* timeStep) override;
};
