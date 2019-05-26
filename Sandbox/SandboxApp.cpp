#include <LumosEngine.h>
#include <App/EntryPoint.h>
#include "Scenes/Scene3D.h"
#include "Scenes/GraphicsScene.h"
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
		GetSceneManager()->EnqueueScene(new SceneModelViewer("SceneModelViewer"));
		GetSceneManager()->EnqueueScene(new Scene2D("2D Test"));
		GetSceneManager()->EnqueueScene(new Scene3D("Physics Scene"));
		GetSceneManager()->EnqueueScene(new GraphicsScene("Terrain Test"));
		GetSceneManager()->JumpToScene(3);
	}
};

lumos::Application* lumos::CreateApplication()
{
    sol::state* state = LuaScript::Instance()->GetState();
  
    WindowProperties windowProperties;
    
    state->script_file(ROOT_DIR"/Sandbox/Settings.lua");
    windowProperties.Title = state->get<std::string>("title");
    windowProperties.Width = state->get<int>("width");
    windowProperties.Height = state->get<int>("height");
    
	windowProperties.ShowConsole = true;
    return new Game(windowProperties);
}
