#include "Scene3D.h"

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
	
	//Create a pendulum
	auto pendulumHolder = m_EntityManager->Create("Pendulum Holder");
	RigidBody3DProperties physicsProperties;
	physicsProperties.Position = Maths::Vector3(12.5f, 22.0f, 20.0f);
	physicsProperties.Mass = 1.0f;
	physicsProperties.Shape = CreateRef<CuboidCollisionShape>(Maths::Vector3(0.5f, 0.5f, 0.5f));
	physicsProperties.Friction = 0.8f;
	physicsProperties.AtRest = true;
	physicsProperties.Static = true;
	physicsProperties.Friction = 0.8f;
	
	Ref<RigidBody3D> pendulumHolderPhysics = CreateRef<RigidBody3D>(physicsProperties);
	
    pendulumHolder.AddComponent<Physics3DComponent>(pendulumHolderPhysics);
    pendulumHolder.AddOrReplaceComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
    //pendulumHolder.AddComponent<Model>(Ref<Graphics::Mesh>(Graphics::CreateCube()), Graphics::PrimitiveType::Cube);
    
	auto pendulum = m_EntityManager->Create("Pendulum");
	Ref<RigidBody3D> pendulumPhysics = CreateRef<RigidBody3D>();
	pendulumPhysics->SetFriction(0.8f);
	pendulumPhysics->SetIsAtRest(true);
	pendulumPhysics->SetInverseMass(1.0);
	pendulumPhysics->SetIsStatic(false);
	pendulumPhysics->SetPosition(Maths::Vector3(12.5f, 17.0f, 20.0f));
	pendulumPhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
    pendulum.AddComponent<Physics3DComponent>(pendulumPhysics);
    pendulum.AddOrReplaceComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
    //pendulum.AddComponent<Model>(Ref<Graphics::Mesh>(Graphics::CreateSphere()), Graphics::PrimitiveType::Sphere);
	pendulum.AddComponent<SpringConstraintComponent>(pendulum, pendulumHolder);
}

void Scene3D::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);

	if(Input::GetInput()->GetKeyPressed(InputCode::Key::P))
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
		if(Input::GetInput()->GetKeyPressed(InputCode::Key::J))
			EntityFactory::AddSphere(this, transform->GetWorldPosition(), -transform->GetForwardDirection());
		if(Input::GetInput()->GetKeyPressed(InputCode::Key::K))
			EntityFactory::AddPyramid(this, transform->GetWorldPosition(), -transform->GetForwardDirection());
		if(Input::GetInput()->GetKeyPressed(InputCode::Key::L))
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
