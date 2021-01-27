#include "MaterialTest.h"
#include "Graphics/MeshFactory.h"

using namespace Lumos;
using namespace Maths;

MaterialTest::MaterialTest(const std::string& SceneName)
: Scene(SceneName)
{
}

MaterialTest::~MaterialTest()
{
}

void MaterialTest::OnInit()
{
	Scene::OnInit();
	
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<OctreeBroadphase>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));
	
	LoadModels();
	
	auto environment = CreateEntity("Environment");
	environment.AddComponent<Graphics::Environment>("//TextuAssets/cubemap/Arches_E_PineTree", 11, 3072, 4096, 1.0f/32.0f,  ".tga");
	
	auto lightEntity = CreateEntity("Light");
	lightEntity.AddComponent<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
	lightEntity.GetComponent<Maths::Transform>().SetLocalTransform(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	
    auto cameraEntity = CreateEntity("Camera");
    cameraEntity.AddComponent<Maths::Transform>(Maths::Vector3(-31.0f, 12.0f, 51.0f));
    cameraEntity.AddComponent<Camera>(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float)m_ScreenWidth / (float)m_ScreenHeight);
    cameraEntity.AddComponent<DefaultCameraController>(DefaultCameraController::ControllerType::EditorCamera);
}

void MaterialTest::OnUpdate(const TimeStep& timeStep)
{
	Scene::OnUpdate(timeStep);
}

void MaterialTest::Render2D()
{
}

void MaterialTest::OnCleanupScene()
{
	Scene::OnCleanupScene();
}

void MaterialTest::LoadModels()
{
	using namespace Lumos::Graphics;
	std::vector<Lumos::Ref<Material>> materials;
	
	auto grassMaterial = CreateRef<Material>();
	grassMaterial->LoadPBRMaterial("grass", "//TextuAssets/pbr");
	materials.push_back(grassMaterial);
	
	auto stonewallMaterial = CreateRef<Material>();
	stonewallMaterial->LoadPBRMaterial("stonewall", "//TextuAssets/pbr");
	materials.push_back(stonewallMaterial);
	
	auto castIronMaterial = CreateRef<Material>();
	castIronMaterial->LoadPBRMaterial("CastIron", "//TextuAssets/pbr", ".tga");
	materials.push_back(castIronMaterial);
	
	auto GunMetalMaterial = CreateRef<Material>();
	GunMetalMaterial->LoadPBRMaterial("GunMetal", "//TextuAssets/pbr", ".tga");
	materials.push_back(GunMetalMaterial);
	
	auto WornWoodMaterial = CreateRef<Material>();
	WornWoodMaterial->LoadPBRMaterial("WornWood", "//TextuAssets/pbr", ".tga");
	materials.push_back(WornWoodMaterial);
	
	auto marbleMaterial = CreateRef<Material>();
	marbleMaterial->LoadPBRMaterial("marble", "//TextuAssets/pbr");
	materials.push_back(marbleMaterial);
	
	auto stoneMaterial = CreateRef<Material>();
	stoneMaterial->LoadPBRMaterial("stone", "//TextuAssets/pbr");
	materials.push_back(stoneMaterial);
	
	auto testMaterial = CreateRef<Material>();
	testMaterial->LoadMaterial("checkerboard", "//TextuAssets/checkerboard.tga");
	materials.push_back(testMaterial);
	
	const float groundWidth = (float(materials.size()) * 1.2f + 1.0f) / 2.0f;
	const float groundHeight = 0.5f;
	const float groundLength = 3.0f;
	
	auto ground = CreateEntity();
	Ref<RigidBody3D> testPhysics = CreateRef<RigidBody3D>();
	testPhysics->SetRestVelocityThreshold(-1.0f);
	testPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	testPhysics->SetFriction(0.8f);
	testPhysics->SetIsAtRest(true);
	testPhysics->SetIsStatic(true);
	
	ground.AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3((float(materials.size()) * 1.2f) / 2.0f - float(materials.size()) / 2.0f - 0.5f, 0.0f, 0.0f)) * Matrix4::Scale(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	ground.AddComponent<Graphics::Model>(Ref<Graphics::Mesh>(Graphics::CreateCube()), Graphics::PrimitiveType::Cube);
	
	auto groundMaterial = CreateRef<Material>();
	
	Graphics::MaterialProperties properties;
	properties.albedoColour = Vector4(0.6f, 0.1f, 0.1f, 1.0f);
	properties.roughnessColour = Vector4(0.6f);
	properties.metallicColour = Vector4(0.15f);
	properties.usingAlbedoMap = 0.5f;
	properties.usingRoughnessMap = 0.0f;
	properties.usingNormalMap = 0.0f;
	properties.usingMetallicMap = 0.0f;
	groundMaterial->SetMaterialProperites(properties);
	ground.GetComponent<Graphics::Model>().GetMeshes().front()->SetMaterial(groundMaterial);
	
	int numObjects = 0;
	
	for(auto& material : materials)
	{
        Entity modelEntity = m_EntityManager->Create();
        modelEntity.AddComponent<Graphics::Model>("//Meshes/material_sphere/material_sphere.fbx");
        auto& transform = modelEntity.GetOrAddComponent<Maths::Transform>();
		
		transform.SetLocalPosition(Maths::Vector3(float(numObjects) * 1.2f - float(materials.size()) / 2.0f, 1.2f, 0.0f));
		transform.SetLocalScale(Maths::Vector3(0.5f, 0.5f, 0.5f));
		
		if(!modelEntity.GetComponent<Graphics::Model>().GetMeshes().empty())
			modelEntity.GetComponent<Graphics::Model>().GetMeshes().front()->SetMaterial(material ? material : testMaterial);
		numObjects++;
	}
}

void MaterialTest::OnImGui()
{
}
