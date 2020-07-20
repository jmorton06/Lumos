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
	Scene::OnInit();

	Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(true);

	LoadLuaScene("/Scripts/FlappyBirdTest.lua");

	auto cameraEntity = m_EntityManager->Create("Camera");
	cameraEntity.AddComponent<Camera>(static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight), 10.0f);

	PushLayer(new Layer2D(new Graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight, true, false, true)));
}

void Scene2D::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);
}

void Scene2D::OnCleanupScene()
{
	Scene::OnCleanupScene();
}
