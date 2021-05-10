#include "Scene3D.h"

#include <Lumos/Physics/LumosPhysicsEngine/HullCollisionShape.h>

using namespace Lumos;
using namespace Maths;

Scene3D::Scene3D(const std::string& SceneName)
	: Scene(SceneName)
{
}

Scene3D::~Scene3D()
{
}

void Scene3D::OnInit()
{
	Scene::OnInit();
}

void Scene3D::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);

	if(Input::Get().GetKeyPressed(InputCode::Key::P))
    {
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(!Application::Get().GetSystem<LumosPhysicsEngine>()->IsPaused());
        Application::Get().GetSystem<B2PhysicsEngine>()->SetPaused(!Application::Get().GetSystem<B2PhysicsEngine>()->IsPaused());
    }

	Camera* cameraComponent = nullptr;
    Maths::Transform* transform = nullptr;

	auto cameraView = m_EntityManager->GetEntitiesWithType<Camera>();
	if(cameraView.Size() > 0)
	{
		cameraComponent = cameraView.Front().TryGetComponent<Camera>();
        transform = cameraView.Front().TryGetComponent<Maths::Transform>();
	}

	if(transform)
	{
		if(Input::Get().GetKeyPressed(InputCode::Key::J))
			EntityFactory::AddSphere(this, transform->GetWorldPosition(), -transform->GetForwardDirection());
		if(Input::Get().GetKeyPressed(InputCode::Key::K))
			EntityFactory::AddPyramid(this, transform->GetWorldPosition(), -transform->GetForwardDirection());
		if(Input::Get().GetKeyPressed(InputCode::Key::L))
			EntityFactory::AddLightCube(this, transform->GetWorldPosition(), -transform->GetForwardDirection());
	}
}

void Scene3D::Render2D()
{
}

void Scene3D::OnCleanupScene()
{
	Scene::OnCleanupScene();
}

void Scene3D::OnImGui()
{
}
