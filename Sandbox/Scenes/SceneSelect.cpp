#include "SceneSelect.h"

using namespace Lumos;
using namespace maths;

SceneSelect::SceneSelect(const std::string& SceneName)
		: Scene(SceneName)
{
}

SceneSelect::~SceneSelect()
{
}

void SceneSelect::OnInit()
{
    Scene::OnInit();
    m_SceneNames = Application::Instance()->GetSceneManager()->GetSceneNames();
    m_pCamera = new ThirdPersonCamera(-20.0f, -40.0f, maths::Vector3(-3.0f, 10.0f, 15.0f), 60.0f, 0.1f, 10000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
    
    //Application::Instance()->PushOverLay(new ImGuiLayer(true));
}

void SceneSelect::OnIMGUI()
{
	static bool show = true;
	ImGui::ShowDemoWindow(&show);

    ImGui::Begin(m_SceneName.c_str());

    for(auto& name : m_SceneNames)
    {
        if(name == "SceneSelect")
            continue;

		if (ImGui::Button(name.c_str()))
		{
            Application::Instance()->GetSceneManager()->JumpToScene(name);
			break;
		}
    }


    ImGui::End();
}

void SceneSelect::OnCleanupScene()
{
    if (m_CurrentScene)
    {
        SAFE_DELETE(m_pCamera);
    }

    Scene::OnCleanupScene();
}
