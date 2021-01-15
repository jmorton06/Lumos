#include <LumosEngine.h>
#include <Core/EntryPoint.h>
#include "Scenes/Scene3D.h"
#include "Scenes/GraphicsScene.h"
#include "Scenes/SceneModelViewer.h"
#include "Scenes/MaterialTest.h"

using namespace Lumos;

class Game : public Application
{
    public:
	explicit Game()
		: Application(std::string("/Sandbox/"), std::string("Sandbox"))
	{
        Application::Get().GetWindow()->SetWindowTitle("Sandbox");
	}

	~Game()
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
        Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Game::OnEvent));
        
        GetSceneManager()->EnqueueScene<Scene3D>(std::string("Physics"));
		GetSceneManager()->EnqueueScene<SceneModelViewer>(std::string("SceneModelViewer"));
		GetSceneManager()->EnqueueScene<GraphicsScene>(std::string("Terrain"));
		GetSceneManager()->EnqueueScene<MaterialTest>(std::string("Material"));
	}
    
    void OnImGui() override
    {
        ImGui::Begin("Test");
        ImGui::TextUnformatted("Test Window");
        ImGui::End();
        Application::OnImGui();
    }
};

Lumos::Application* Lumos::CreateApplication()
{
	return new Game();
}
