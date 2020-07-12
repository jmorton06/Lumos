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

	auto cameraEntity = GetRegistry().create();
	auto& camera = GetRegistry().emplace<Camera>(cameraEntity, static_cast<float>(m_ScreenWidth) / static_cast<float>(m_ScreenHeight), 10.0f);
	camera.SetCameraController(CreateRef<CameraController2D>());
	camera.SetIsOrthographic(true);
	GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");

	PushLayer(new Layer2D(new Graphics::Renderer2D(m_ScreenWidth, m_ScreenHeight, true, false, false)));
}

void Scene2D::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);
}

void Scene2D::OnCleanupScene()
{
	Scene::OnCleanupScene();
}
