#pragma once
#include <LumosEngine.h>

class Scene2D : public Lumos::Scene
{
public:
	explicit Scene2D(const std::string& SceneName);
	virtual ~Scene2D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(const Lumos::TimeStep& timeStep) override;
};
