#include <LumosEngine.h>
#include <App/EntryPoint.h>
#include "Scenes/Scene3D.h"
#include "Scenes/GraphicsScene.h"
#include "Scenes/SceneLuaTest.h"
#include "Scenes/SceneModelViewer.h"
#include "Scenes/SceneSelect.h"
#include "Scenes/Scene2D.h"

using namespace lumos;

class Game : public Application
{
public:
	explicit Game(const WindowProperties& windowProperties) : Application(windowProperties, graphics::RenderAPI::VULKAN)
	{
	}

	~Game()
	{
	}

	void Init() override
	{
		Application::Init();

		const String root = ROOT_DIR;
		VFS::Get()->Mount("Meshes", root + "/Sandbox/res/meshes");
		VFS::Get()->Mount("Textures", root + "/Sandbox/res/textures");
		VFS::Get()->Mount("Sounds", root + "/Sandbox/res/sounds");

		GetSceneManager()->EnqueueScene(new SceneSelect("SceneSelect"));
		GetSceneManager()->EnqueueScene(new SceneLuaTest("Lua Test Scene"));
		GetSceneManager()->EnqueueScene(new SceneModelViewer("SceneModelViewer"));
		GetSceneManager()->EnqueueScene(new Scene2D("2D Test"));
		GetSceneManager()->EnqueueScene(new Scene3D("Physics Scene"));
		GetSceneManager()->EnqueueScene(new GraphicsScene("Terrain Test"));
		GetSceneManager()->JumpToScene(4);
	}
};

lumos::Application* lumos::CreateApplication()
{
    System::CFG cfg(ROOT_DIR"/Sandbox/Settings.cfg");
    WindowProperties windowProperties(cfg);
	windowProperties.ShowConsole = true;
    return new Game(windowProperties);
}
