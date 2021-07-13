#include <LumosEngine.h>
#include <Lumos/Core/EntryPoint.h>

using namespace Lumos;

class Runtime : public Application
{
public:
    explicit Runtime()
        : Application()
    {
        Application::Get().GetWindow()->SetWindowTitle("Runtime");
    }

    ~Runtime()
    {
    }

    void OnEvent(Event& e) override
    {
        Application::OnEvent(e);
    }

    void Init() override
    {
        Application::Init();
        Application::SetEditorState(EditorState::Play);
        Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Runtime::OnEvent));
    }

    void OnImGui() override
    {
        ImGui::Begin("Metrics");
        ImGui::Text("FPS : %.2f", (float)Lumos::Engine::Get().Statistics().FramesPerSecond);
        ImGui::End();
        Application::OnImGui();
    }
};

Lumos::Application* Lumos::CreateApplication()
{
    return new Runtime();
}
