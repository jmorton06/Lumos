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
		: Application("/Sandbox/", "Sandbox")
	{
        Application::Get().GetWindow()->SetWindowTitle("Sandbox");
	}

	~Game()
	{
	}

	void Init() override
	{
		Application::Init();
        
        GetSceneManager()->EnqueueScene<Scene3D>("Physics");
		GetSceneManager()->EnqueueScene<SceneModelViewer>("SceneModelViewer");
		GetSceneManager()->EnqueueScene<GraphicsScene>("Terrain");
		GetSceneManager()->EnqueueScene<MaterialTest>("Material");
	}
};

Lumos::Application* Lumos::CreateApplication()
{
	return new Game();
}
