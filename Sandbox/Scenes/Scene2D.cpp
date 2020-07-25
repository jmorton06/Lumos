#include "Scene2D.h"

using namespace Lumos;
using namespace Maths;

Scene2D::Scene2D(const std::string& SceneName)
	: Scene(SceneName)
{
}

Scene2D::~Scene2D()
{
}

void Scene2D::OnInit()
{
    auto gameControllerEntity = m_EntityManager->Create("GameController");
    gameControllerEntity.AddComponent<LuaScriptComponent>("/Scripts/FlappyBirdTest.lua", this);
    
	Scene::OnInit();
}

void Scene2D::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);
}

void Scene2D::OnCleanupScene()
{
	Scene::OnCleanupScene();
}
