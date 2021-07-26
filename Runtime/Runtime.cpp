#include <LumosEngine.h>
#include <Lumos/Core/EntryPoint.h>

using namespace Lumos;

class Runtime : public Application
{
    friend class Application;

public:
    explicit Runtime()
        : Application()
    {
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
        m_ProjectRoot = ROOT_DIR "/ExampleProject/";
        m_ProjectName = "Example";

        Application::Init();
        Application::SetEditorState(EditorState::Play);
        Application::Get().GetWindow()->SetWindowTitle("Runtime");
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
    return new ::Runtime();
}
