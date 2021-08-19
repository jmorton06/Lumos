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
        std::vector<std::string> projectLocations = { "ExampleProject/Example.lmprj", "/Users/jmorton/dev/Lumos/ExampleProject/Example.lmprj", "../ExampleProject/Example.lmprj", OS::Instance()->GetExecutablePath() + "/ExampleProject/Example.lmprj", OS::Instance()->GetExecutablePath() + "/../ExampleProject/Example.lmprj", OS::Instance()->GetExecutablePath() + "/../../ExampleProject/Example.lmprj" };
        bool fileFound = false;
        std::string filePath;
        for(auto& path : projectLocations)
        {
            if(FileSystem::FileExists(path))
            {
                LUMOS_LOG_INFO("Loaded Project {0}", path);
                m_ProjectRoot = StringUtilities::GetFileLocation(path);
                m_ProjectName = "Example";
                break;
            }
        }

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
