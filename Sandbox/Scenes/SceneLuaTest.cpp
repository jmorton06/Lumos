#include "SceneLuaTest.h"

using namespace lumos;
using namespace maths;

SceneLuaTest::SceneLuaTest(const std::string& SceneName)
	: Scene(SceneName)
{
}

SceneLuaTest::~SceneLuaTest()
{
}

void SceneLuaTest::OnInit()
{
	Scene::OnInit();

	SceneBinding* scene = new SceneBinding(this);
	LuaScripting::GetGlobal()->RegisterObject(SceneBinding::className,"currentScene", scene);


    String root = ROOT_DIR;

	LuaScripting::GetGlobal()->RunFile(root + "/Sandbox/Test.lua");
}

void SceneLuaTest::OnUpdate(TimeStep* timeStep)
{
	LuaScripting::GetGlobal()->SetDeltaTime(static_cast<double>(timeStep->GetSeconds()));
	LuaScripting::GetGlobal()->FixedUpdate();
	Scene::OnUpdate(timeStep);
}

void SceneLuaTest::Render2D()
{
}

void SceneLuaTest::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		SAFE_DELETE(m_pCamera)
	}

	Scene::OnCleanupScene();
}

void SceneLuaTest::OnIMGUI()
{
	ImGui::Begin(m_SceneName.c_str());
 	if(ImGui::Button("<- Back"))
	{
		Application::Instance()->GetSceneManager()->JumpToScene("SceneSelect");
		ImGui::End();
		return;
	}

    ImGui::End();
}
