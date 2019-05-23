#pragma once
#include <LumosEngine.h>

class GraphicsScene : public lumos::Scene
{
public:
	GraphicsScene(const std::string& SceneName);
	virtual ~GraphicsScene();

	virtual void OnInit() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdate(lumos::TimeStep* timeStep) override;
	virtual void OnIMGUI() override;

	void LoadModels();
private:
	std::unique_ptr<lumos::graphics::TextureDepthArray> m_ShadowTexture;
};
