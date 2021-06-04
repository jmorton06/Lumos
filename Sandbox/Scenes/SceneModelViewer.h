#pragma once

#include <LumosEngine.h>

class SceneModelViewer : public Lumos::Scene
{
public:
    explicit SceneModelViewer(const std::string& SceneName);
    virtual ~SceneModelViewer();

    virtual void OnInit() override;
    virtual void OnCleanupScene() override;
    virtual void OnUpdate(const Lumos::TimeStep& timeStep) override;
    virtual void OnImGui() override;
    void LoadModels();
};
