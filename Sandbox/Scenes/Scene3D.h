#pragma once
#include <LumosEngine.h>

class Scene3D : public lumos::Scene
{
public:
	explicit Scene3D(const String& SceneName);
	virtual ~Scene3D();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(lumos::TimeStep* timeStep) override;
	virtual void Render2D() override;
	virtual void OnIMGUI() override;
	void LoadModels();

private:
	std::unique_ptr<lumos::graphics::TextureDepthArray> m_ShadowTexture;
};
