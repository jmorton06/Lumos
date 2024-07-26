#include <Lumos/Core/Application.h>
#include <Lumos/Core/EntryPoint.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/Font.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Maths/Vector4.h>
#include <Lumos/Maths/Matrix4.h>
#include <Lumos/Maths/MathsUtilities.h>

#include <imgui/imgui.h>

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

    void OnUpdate(const TimeStep& dt) override
    {
        using namespace Lumos;
        Application::OnUpdate(dt);
    }

    void OnEvent(Event& e) override
    {
        Application::OnEvent(e);

        // if(Input::Get().GetKeyPressed(Lumos::InputCode::Key::Escape))
        //   Application::SetAppState(AppState::Closing);
    }

    void Init() override
    {
        std::vector<std::string> projectLocations = {
            OS::Instance()->GetExecutablePath() + "../../../../../ExampleProject/",
            "/Users/jmorton/dev/Lumos/ExampleProject/Example.lmproj",
            "ExampleProject/Example.lmproj",
            "../ExampleProject/Example.lmproj",
            OS::Instance()->GetExecutablePath() + "/ExampleProject/Example.lmproj",
            OS::Instance()->GetExecutablePath() + "/../ExampleProject/Example.lmproj",
            OS::Instance()->GetExecutablePath() + "/../../ExampleProject/Example.lmproj"
        };

        bool fileFound = false;
        std::string filePath;
        for(auto& path : projectLocations)
        {
            if(FileSystem::FileExists(path))
            {
                LINFO("Loaded Project %s", path.c_str());
                m_ProjectSettings.m_ProjectRoot = StringUtilities::GetFileLocation(path);
                m_ProjectSettings.m_ProjectName = "Example";
                break;
            }
        }

        Application::Init();
        Application::SetEditorState(EditorState::Play);
        Application::Get().GetWindow()->SetWindowTitle("Runtime");
        Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Runtime::OnEvent));

        Vec4 testVec4 = { 3.0f, 0.0f, 0.0f, 1.0f };
        Mat4 testMat4 = Mat4::Translation(testVec4.ToVector3());
    }

    void OnImGui() override
    {
        ImGui::Begin("Metrics");
        ImGui::Text("FPS : %.2f", (float)Lumos::Engine::Get().Statistics().FramesPerSecond);
        ImGui::Text("Scene : %s", Application::Get().GetSceneManager()->GetCurrentScene()->GetSceneName().c_str());

        i32 sceneIndex = Application::Get().GetSceneManager()->GetCurrentSceneIndex();
        if(ImGui::InputInt("Scene Index", &sceneIndex))
            Application::Get().GetSceneManager()->SwitchScene(sceneIndex);
        ImGui::End();
        Application::OnImGui();
    }
};

Lumos::Application* Lumos::CreateApplication()
{
    return new ::Runtime();
}
