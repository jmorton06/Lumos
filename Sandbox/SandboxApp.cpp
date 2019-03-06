#include <LumosEngine.h>
#include <App/EntryPoint.h>
#include "Scenes/Scene3D.h"
#include "Scenes/GraphicsScene.h"
#include "Scenes/SceneLuaTest.h"
#include "Scenes/SceneModelViewer.h"
#include "Scenes/SceneSelect.h"
#include "Scenes/Scene2D.h"


using namespace Lumos;

class Game : public Application
{
public:
	Game(WindowProperties windowProperties) : Application(windowProperties, RenderAPI::VULKAN)
	{
	}

	~Game()
	{
	}

	void Init() override
	{
		Application::Init();

		const String root = ROOT_DIR;
		Lumos::VFS::Get()->Mount("Meshes", root + "/Sandbox/res/meshes");
		Lumos::VFS::Get()->Mount("Textures", root + "/Sandbox/res/textures");
		Lumos::VFS::Get()->Mount("Sounds", root + "/Sandbox/res/sounds");

		GetSceneManager()->EnqueueScene(new SceneSelect("SceneSelect"));
		GetSceneManager()->EnqueueScene(new SceneLuaTest("Lua Test Scene"));
		GetSceneManager()->EnqueueScene(new SceneModelViewer("SceneModelViewer"));
		GetSceneManager()->EnqueueScene(new Scene2D("2D Test"));
		GetSceneManager()->EnqueueScene(new Scene3D("Physics Scene"));
		GetSceneManager()->EnqueueScene(new GraphicsScene("Terrain Test"));
		GetSceneManager()->JumpToScene(4);
	}
};

Lumos::Application* Lumos::CreateApplication()
{
    System::CFG cfg(ROOT_DIR"/Sandbox/Settings.cfg");
    WindowProperties windowProperties(cfg);

    return new Game(windowProperties);
}
