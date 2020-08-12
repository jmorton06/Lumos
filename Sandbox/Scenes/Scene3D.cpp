#include <Graphics/Environment.h>
#include "Scene3D.h"
#include "Graphics/MeshFactory.h"

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

	Application::Get().GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(false);

	LoadModels();

	Application::Get().GetWindow()->HideMouse(false);

	auto environment = m_EntityManager->Create("Environment");
	environment.AddComponent<Graphics::Environment>("/Textures/cubemap/Arches_E_PineTree", 11, 3072, 4096, ".tga");

	auto lightEntity = m_EntityManager->Create("Light");
    lightEntity.AddComponent<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
    lightEntity.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());

	auto cameraEntity = m_EntityManager->Create("Camera");
    cameraEntity.AddComponent<Maths::Transform>(Maths::Vector3(-31.0f, 12.0f, 51.0f));
	cameraEntity.AddComponent<Camera>(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float)m_ScreenWidth / (float)m_ScreenHeight);
    cameraEntity.AddComponent<DefaultCameraController>(DefaultCameraController::ControllerType::EditorCamera);
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
	if(m_CurrentScene)
	{
		Application::Get().GetSystem<LumosPhysicsEngine>()->ClearConstraints();
	}

	Scene::OnCleanupScene();
}

void Scene3D::LoadModels()
{
	const float groundWidth = 100.0f;
	const float groundHeight = 0.5f;
	const float groundLength = 100.0f;

	auto testMaterial = CreateRef<Material>();
	testMaterial->LoadMaterial("checkerboard", "/Textures/checkerboard.tga");

	auto ground = m_EntityManager->Create();
	Ref<RigidBody3D> testPhysics = CreateRef<RigidBody3D>();
	testPhysics->SetRestVelocityThreshold(-1.0f);
	testPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	testPhysics->SetFriction(0.8f);
	testPhysics->SetIsAtRest(true);
	testPhysics->SetIsStatic(true);

    ground.AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(groundWidth, groundHeight, groundLength)));
    ground.AddComponent<Physics3DComponent>(testPhysics);
    ground.AddComponent<Lumos::LuaScriptComponent>("Scripts/LuaComponentTest.lua", this);
    ground.AddComponent<MeshComponent>(Graphics::CreateCube());

	MaterialProperties properties;
	properties.albedoColour = Vector4(0.6f, 0.1f, 0.1f, 1.0f);
	properties.roughnessColour = Vector4(0.6f);
	properties.metallicColour = Vector4(0.15f);
	properties.usingAlbedoMap = 0.5f;
	properties.usingRoughnessMap = 0.0f;
	properties.usingNormalMap = 0.0f;
	properties.usingMetallicMap = 0.0f;
	testMaterial->SetMaterialProperites(properties);
	ground.AddComponent<MaterialComponent>(testMaterial);

	//Create a pendulum
	auto pendulumHolder = m_EntityManager->Create("Pendulum Holder");
	RigidBody3DProperties physicsProperties;
	physicsProperties.Position = Maths::Vector3(12.5f, 15.0f, 20.0f);
	physicsProperties.Mass = 1.0f;
	physicsProperties.Shape = CreateRef<CuboidCollisionShape>(Maths::Vector3(0.5f, 0.5f, 0.5f));
	physicsProperties.Friction = 0.8f;
	physicsProperties.AtRest = true;
	physicsProperties.Static = true;
	physicsProperties.Friction = 0.8f;

	Ref<RigidBody3D> pendulumHolderPhysics = CreateRef<RigidBody3D>(physicsProperties);

    pendulumHolder.AddComponent<Physics3DComponent>(pendulumHolderPhysics);
    pendulumHolder.AddOrReplaceComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
    pendulumHolder.AddComponent<MeshComponent>(Graphics::CreateCube());
    pendulumHolder.AddComponent<Lumos::LuaScriptComponent>("Scripts/PlayerTest.lua", this);

	auto pendulum = m_EntityManager->Create("Pendulum");
	Ref<RigidBody3D> pendulumPhysics = CreateRef<RigidBody3D>();
	pendulumPhysics->SetFriction(0.8f);
	pendulumPhysics->SetIsAtRest(true);
	pendulumPhysics->SetInverseMass(1.0);
	pendulumPhysics->SetIsStatic(false);
	pendulumPhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 20.0f));
	pendulumPhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
    pendulum.AddComponent<Physics3DComponent>(pendulumPhysics);
    pendulum.AddOrReplaceComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
    pendulum.AddComponent<MeshComponent>(Graphics::CreateSphere());

	auto pendulumConstraint = new SpringConstraint(pendulumHolder.GetComponent<Physics3DComponent>().GetRigidBody().get(), pendulum.GetComponent<Physics3DComponent>().GetRigidBody().get(), pendulumHolder.GetComponent<Physics3DComponent>().GetRigidBody()->GetPosition(), pendulum.GetComponent<Physics3DComponent>().GetRigidBody()->GetPosition(), 0.9f, 0.5f);
	Application::Get().GetSystem<LumosPhysicsEngine>()->AddConstraint(pendulumConstraint);

	//plastics
	int numSpheres = 0;
	for(int i = 0; i < 10; i++)
	{
		float roughness = i / 10.0f;
		Maths::Vector4 spec(0.24f);
		Vector4 diffuse(0.9f);

		Ref<Material> m = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.roughnessColour = Vector4(roughness);
		properties.metallicColour = spec;
		properties.usingAlbedoMap = 0.0f;
		properties.usingRoughnessMap = 0.0f;
		properties.usingNormalMap = 0.0f;
		properties.usingMetallicMap = 0.0f;
		m->SetMaterialProperites(properties);

		auto sphere = m_EntityManager->Create("Sphere" + StringFormat::ToString(numSpheres++));

        sphere.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(float(i), 17.0f, 0.0f)) * Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
        sphere.AddComponent<MeshComponent>(Graphics::CreateSphere());
        sphere.AddComponent<MaterialComponent>(m);
	}

	//metals
	for(int i = 0; i < 10; i++)
	{
		float roughness = i / 10.0f;
		Vector4 spec(0.9f);
		Vector4 diffuse(0.9f);

		Ref<Material> m = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.roughnessColour = Vector4(roughness);
		properties.metallicColour = spec;
		properties.usingAlbedoMap = 0.0f;
		properties.usingRoughnessMap = 0.0f;
		properties.usingNormalMap = 0.0f;
		properties.usingMetallicMap = 0.0f;
		m->SetMaterialProperites(properties);

		auto sphere = m_EntityManager->Create("Sphere" + StringFormat::ToString(numSpheres++));

        sphere.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(float(i), 18.0f, 0.0f)) * Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
        sphere.AddComponent<MeshComponent>(Graphics::CreateSphere());
        sphere.AddComponent<MaterialComponent>(m);
	}
}

void Scene3D::OnImGui()
{
}
