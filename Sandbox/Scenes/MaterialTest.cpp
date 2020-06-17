#include "MaterialTest.h"
#include "Graphics/MeshFactory.h"

using namespace Lumos;
using namespace Maths;

MaterialTest::MaterialTest(const String& SceneName)
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
	Application::Get().GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

	LoadModels();

	m_SceneBoundingRadius = 20.0f;
    
    auto environment = m_Registry.create();
    m_Registry.emplace<Graphics::Environment>(environment, "/Textures/cubemap/Arches_E_PineTree", 11, 3072, 4096, ".tga");
    m_Registry.emplace<NameComponent>(environment, "Environment");

	auto lightEntity = m_Registry.create();
	m_Registry.emplace<Graphics::Light>(lightEntity, Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
	m_Registry.emplace<Maths::Transform>(lightEntity, Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	m_Registry.emplace<NameComponent>(lightEntity, "Light");

	auto cameraEntity = m_Registry.create();
	auto& camera = m_Registry.emplace<Camera>(cameraEntity, -1.0f, 358.0f, Maths::Vector3(-0.23f, 2.4f, 11.4f), 60.0f, 0.1f, 1000.0f, (float)m_ScreenWidth / (float)m_ScreenHeight);
	m_Registry.emplace<NameComponent>(cameraEntity, "Camera");
	Application::Get().GetSystem<AudioManager>()->SetListener(&camera);

	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

	PushLayer(new Layer3D(new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight, editor), "Deferred"));
	PushLayer(new Layer3D(new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, editor), "Skybox"));
    
    #ifndef LUMOS_PLATFORM_IOS
        auto shadowRenderer = new Graphics::ShadowRenderer();
        shadowRenderer->SetLightEntity(lightEntity);
        auto shadowLayer = new Layer3D(shadowRenderer);
        Application::Get().GetRenderManager()->SetShadowRenderer(shadowRenderer);
        PushLayer(shadowLayer);
    #endif
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
	if (m_CurrentScene)
	{
		Application::Get().GetSystem<LumosPhysicsEngine>()->ClearConstraints();
	}

	Scene::OnCleanupScene();
}

void MaterialTest::LoadModels()
{
	std::vector<Lumos::Ref<Material>> materials;

	auto grassMaterial = CreateRef<Material>();
	grassMaterial->LoadPBRMaterial("grass", "/Textures/pbr");
	materials.push_back(grassMaterial);

	auto stonewallMaterial = CreateRef<Material>();
	stonewallMaterial->LoadPBRMaterial("stonewall", "/Textures/pbr");
	materials.push_back(stonewallMaterial);

	auto castIronMaterial = CreateRef<Material>();
	castIronMaterial->LoadPBRMaterial("CastIron", "/Textures/pbr", ".tga");
	materials.push_back(castIronMaterial);

	auto GunMetalMaterial = CreateRef<Material>();
	GunMetalMaterial->LoadPBRMaterial("GunMetal", "/Textures/pbr", ".tga");
	materials.push_back(GunMetalMaterial);

	auto WornWoodMaterial = CreateRef<Material>();
	WornWoodMaterial->LoadPBRMaterial("WornWood", "/Textures/pbr", ".tga");
	materials.push_back(WornWoodMaterial);

	auto marbleMaterial = CreateRef<Material>();
	marbleMaterial->LoadPBRMaterial("marble", "/Textures/pbr");
	materials.push_back(marbleMaterial);

	auto stoneMaterial = CreateRef<Material>();
	stoneMaterial->LoadPBRMaterial("stone", "/Textures/pbr");
	materials.push_back(stoneMaterial);

	auto testMaterial = CreateRef<Material>();
	testMaterial->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");
	materials.push_back(testMaterial);

	const float groundWidth = (float(materials.size()) * 1.2f + 1.0f) / 2.0f;
	const float groundHeight = 0.5f;
	const float groundLength = 3.0f;

	auto ground = m_Registry.create();
	Ref<PhysicsObject3D> testPhysics = CreateRef<PhysicsObject3D>();
	testPhysics->SetRestVelocityThreshold(-1.0f);
	testPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	testPhysics->SetFriction(0.8f);
	testPhysics->SetIsAtRest(true);
	testPhysics->SetIsStatic(true);

	m_Registry.emplace<Maths::Transform>(ground,Matrix4::Translation(Maths::Vector3((float(materials.size()) * 1.2f) / 2.0f - float(materials.size()) / 2.0f - 0.5f, 0.0f, 0.0f)) * Matrix4::Scale(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	Ref<Graphics::Mesh> groundModel = AssetsManager::DefaultModels()->Get("Cube");
	m_Registry.emplace<MeshComponent>(ground, groundModel);

	auto groundMaterial = CreateRef<Material>();

	MaterialProperties properties;
	properties.albedoColour = Vector4(0.6f, 0.1f, 0.1f, 1.0f);
	properties.roughnessColour = Vector4(0.6f);
	properties.metallicColour = Vector4(0.15f);
	properties.usingAlbedoMap = 0.5f;
	properties.usingRoughnessMap = 0.0f;
	properties.usingNormalMap = 0.0f;
	properties.usingMetallicMap = 0.0f;
	groundMaterial->SetMaterialProperites(properties);
	m_Registry.emplace<MaterialComponent>(ground, groundMaterial);

	int numObjects = 0;

	for (auto& material : materials)
	{
		auto obj = m_Registry.create();

		m_Registry.emplace<Maths::Transform>(obj, Matrix4::Translation(Maths::Vector3(float(numObjects) * 1.2f - float(materials.size()) / 2.0f, 1.2f, 0.0f)) * Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
		m_Registry.emplace<MeshComponent>(obj, AssetsManager::DefaultModels()->Get("Sphere"));
        
        m_Registry.emplace<MaterialComponent>(obj,material ? material : testMaterial);
		m_Registry.emplace<NameComponent>(obj, "Test Object" + StringFormat::ToString(numObjects++));
	}

	//m_Registry.destroy(testMesh);
}

void MaterialTest::OnImGui()
{
}
