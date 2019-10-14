#include "Scene3D.h"
#include "Graphics/MeshFactory.h"

using namespace Lumos;
using namespace Maths;

class TestComponent
{
public:
	TestComponent() 
	{

	};

	void Init() {};

	void OnImGui() 
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Test");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		static bool test = false;
		ImGui::Checkbox("##Test", &test);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	};
};

Scene3D::Scene3D(const String& SceneName)
		: Scene(SceneName)
{
	//ComponentManager::Instance()->RegisterComponent<TestComponent>();
}

Scene3D::~Scene3D()
{
}

void Scene3D::OnInit()
{
	Scene::OnInit();

    Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
    Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
    Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

	LoadModels();

	m_pCamera = new EditorCamera(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);

	m_SceneBoundingRadius = 20.0f;

	String environmentFiles[11] =
	{
		"/Textures/cubemap/CubeMap0.tga",
		"/Textures/cubemap/CubeMap1.tga",
		"/Textures/cubemap/CubeMap2.tga",
		"/Textures/cubemap/CubeMap3.tga",
		"/Textures/cubemap/CubeMap4.tga",
		"/Textures/cubemap/CubeMap5.tga",
		"/Textures/cubemap/CubeMap6.tga",
		"/Textures/cubemap/CubeMap7.tga",
		"/Textures/cubemap/CubeMap8.tga",
		"/Textures/cubemap/CubeMap9.tga",
		"/Textures/cubemap/CubeMap10.tga"
	};

	m_EnvironmentMap = Graphics::TextureCube::CreateFromVCross(environmentFiles, 11);
    
    auto lightEntity = EntityManager::Instance()->CreateEntity("Directional Light");
    lightEntity->AddComponent<Graphics::Light>(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 0.9f);
	lightEntity->AddComponent<Maths::Transform>(Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::Zero()).ToMatrix4());
    AddEntity(lightEntity);

	auto cameraEntity = EntityManager::Instance()->CreateEntity("Camera");
	cameraEntity->AddComponent<CameraComponent>(m_pCamera);
	AddEntity(cameraEntity);

	Application::Instance()->GetSystem<AudioManager>()->SetListener(m_pCamera);

	auto shadowRenderer = new Graphics::ShadowRenderer();
	shadowRenderer->SetLightEntity(lightEntity);

	auto shadowLayer = new Layer3D(shadowRenderer, "Shadow");

	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif
	auto deferredLayer = new Layer3D(new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight, editor), "Deferred");
	auto skyBoxLayer = new Layer3D(new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap, editor), "Skybox");
	//auto gridLayer = new Layer3D(new Graphics::GridRenderer(m_ScreenWidth, m_ScreenHeight, true), "Grid");
	Application::Instance()->PushLayer(shadowLayer);
    Application::Instance()->PushLayer(deferredLayer);
	Application::Instance()->PushLayer(skyBoxLayer);
	//Application::Instance()->PushLayer(gridLayer);

	Application::Instance()->GetRenderManager()->SetShadowRenderer(shadowRenderer);
    Application::Instance()->GetRenderManager()->SetSkyBoxTexture(m_EnvironmentMap);
}

void Scene3D::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
}

void Scene3D::Render2D()
{
}

void Scene3D::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		SAFE_DELETE(m_pCamera)
        SAFE_DELETE(m_EnvironmentMap);
        Application::Instance()->GetSystem<LumosPhysicsEngine>()->ClearConstraints();
	}

	Scene::OnCleanupScene();
}

void Scene3D::LoadModels()
{
	const float groundWidth = 100.0f;
	const float groundHeight = 0.5f;
	const float groundLength = 100.0f;

	auto testMaterial = CreateRef<Material>();
    testMaterial->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

	auto ground = EntityManager::Instance()->CreateEntity("Ground");
	Ref<PhysicsObject3D> testPhysics = CreateRef<PhysicsObject3D>();
	testPhysics->SetRestVelocityThreshold(-1.0f);
	testPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	testPhysics->SetFriction(0.8f);
	testPhysics->SetIsAtRest(true);
	testPhysics->SetIsStatic(true);

	ground->AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	ground->AddComponent<Physics3DComponent>(testPhysics);
	//ground->AddComponent<TestComponent>();

	Ref<Graphics::Mesh> groundModel = AssetsManager::DefaultModels()->Get("Cube");
	ground->AddComponent<MeshComponent>(groundModel);

	MaterialProperties properties;
	properties.albedoColour = Vector4(0.6f,0.1f,0.1f,1.0f);
	properties.roughnessColour = Vector4(0.6f);
	properties.specularColour = Vector4(0.15f);
	properties.usingAlbedoMap     = 0.5f;
	properties.usingRoughnessMap  = 0.0f;
	properties.usingNormalMap     = 0.0f;
	properties.usingSpecularMap   = 0.0f;
	testMaterial->SetMaterialProperites(properties);
	ground->AddComponent<MaterialComponent>(testMaterial);

	AddEntity(ground);

	#if 0

	auto grassMaterial = CreateRef<Material>();
	grassMaterial->LoadPBRMaterial("grass", "/Textures/pbr");

	auto stonewallMaterial = CreateRef<Material>();
	stonewallMaterial->LoadPBRMaterial("stonewall", "/Textures/pbr");

	auto castIronMaterial = CreateRef<Material>();
	castIronMaterial->LoadPBRMaterial("CastIron", "/Textures/pbr",".tga");

	auto GunMetalMaterial = CreateRef<Material>();
	GunMetalMaterial->LoadPBRMaterial("GunMetal", "/Textures/pbr",".tga");

	auto WornWoodMaterial = CreateRef<Material>();
	WornWoodMaterial->LoadPBRMaterial("WornWood", "/Textures/pbr",".tga");

	auto marbleMaterial = CreateRef<Material>();
	marbleMaterial->LoadPBRMaterial("marble", "/Textures/pbr");

	auto stoneMaterial = CreateRef<Material>();
	stoneMaterial->LoadPBRMaterial("stone", "/Textures/pbr");

	//Create a Rest Cube
	auto cube = EntityManager::Instance()->CreateEntity("cube");
	Ref<PhysicsObject3D> cubePhysics = CreateRef<PhysicsObject3D>();
	cubePhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(0.5f, 0.5f, 0.5f)));
	cubePhysics->SetFriction(0.8f);
	cubePhysics->SetIsAtRest(true);
	cubePhysics->SetInverseMass(1.0);
	cubePhysics->SetInverseInertia(cubePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	cubePhysics->SetIsStatic(false);
	cubePhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 0.0f));
	cube->AddComponent<Physics3DComponent>(cubePhysics);
	cube->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> cubeModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->Get("Cube"));
	cube->AddComponent<MeshComponent>(cubeModel);

	cube->AddComponent<MaterialComponent>(marbleMaterial);

	AddEntity(cube);

	//Create a Rest Sphere
	auto restsphere = EntityManager::Instance()->CreateEntity("Sphere");
	Ref<PhysicsObject3D> restspherePhysics = CreateRef<PhysicsObject3D>();
	restspherePhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(0.5f)));
	restspherePhysics->SetFriction(0.8f);
	restspherePhysics->SetIsAtRest(true);
	restspherePhysics->SetInverseMass(1.0);
	restspherePhysics->SetInverseInertia(restspherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	restspherePhysics->SetIsStatic(false);
	restspherePhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 5.0f));
	restsphere->AddComponent<Physics3DComponent>(restspherePhysics);
	restsphere->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> restsphereModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->Get("Cube"));
	restsphere->AddComponent<MeshComponent>(restsphereModel);
	restsphere->AddComponent<MaterialComponent>(castIronMaterial);

	AddEntity(restsphere);

	//Create a Rest Pyramid
	auto pyramid = EntityManager::Instance()->CreateEntity("Pyramid");
	Ref<PhysicsObject3D> pyramidPhysics = CreateRef<PhysicsObject3D>();
	pyramidPhysics->SetCollisionShape(CreateRef<PyramidCollisionShape>(Maths::Vector3(0.5f)));
	pyramidPhysics->SetFriction(0.8f);
	pyramidPhysics->SetIsAtRest(true);
	pyramidPhysics->SetInverseMass(1.0);
	pyramidPhysics->SetInverseInertia(pyramidPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pyramidPhysics->SetIsStatic(false);
	pyramidPhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 8.0f));
	pyramid->AddComponent<Physics3DComponent>(pyramidPhysics);
	pyramid->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> pyramidModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->Get("Pyramid"));
	pyramid->AddComponent<MeshComponent>(pyramidModel);
	pyramid->AddComponent<MaterialComponent>(marbleMaterial);

	AddEntity(pyramid);

	//Grass
	auto grassSphere = EntityManager::Instance()->CreateEntity("grassSphere");
	Ref<PhysicsObject3D> grassSpherePhysics = CreateRef<PhysicsObject3D>();
	grassSpherePhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
	grassSpherePhysics->SetFriction(0.8f);
	grassSpherePhysics->SetIsAtRest(true);
	grassSpherePhysics->SetInverseMass(1.0);
	grassSpherePhysics->SetInverseInertia(grassSpherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	grassSpherePhysics->SetIsStatic(false);
	grassSpherePhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 13.0f));
	grassSphere->AddComponent<Physics3DComponent>(grassSpherePhysics);
	grassSphere->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> grassSphereModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->Get("Sphere"));
	grassSphere->AddComponent<MeshComponent>(grassSphereModel);
	grassSphere->AddComponent<MaterialComponent>(grassMaterial);

	AddEntity(grassSphere);

	//Marble
	auto marbleSphere = EntityManager::Instance()->CreateEntity("marbleSphere");
	Ref<PhysicsObject3D> marbleSpherePhysics = CreateRef<PhysicsObject3D>();
	marbleSpherePhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
	marbleSpherePhysics->SetFriction(0.8f);
	marbleSpherePhysics->SetIsAtRest(true);
	marbleSpherePhysics->SetInverseMass(1.0);
	marbleSpherePhysics->SetInverseInertia(marbleSpherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	marbleSpherePhysics->SetIsStatic(false);
	marbleSpherePhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 15.0f));
	marbleSphere->AddComponent<Physics3DComponent>(marbleSpherePhysics);
	marbleSphere->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> marbleSphereModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->Get("Sphere"));
	marbleSphere->AddComponent<MeshComponent>(marbleSphereModel);
	marbleSphere->AddComponent<MaterialComponent>(marbleMaterial);

	AddEntity(marbleSphere);

	//stone
	auto stoneSphere = EntityManager::Instance()->CreateEntity("stoneSphere");
	Ref<PhysicsObject3D> stoneSpherePhysics = CreateRef<PhysicsObject3D>();
	stoneSpherePhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
	stoneSpherePhysics->SetFriction(0.8f);
	stoneSpherePhysics->SetIsAtRest(true);
	stoneSpherePhysics->SetInverseMass(1.0);
	stoneSpherePhysics->SetInverseInertia(stoneSpherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	stoneSpherePhysics->SetIsStatic(false);
	stoneSpherePhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 17.0f));
	stoneSphere->AddComponent<Physics3DComponent>(stoneSpherePhysics);
	stoneSphere->AddComponent<TransformComponent>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> stoneSphereModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->Get("Sphere"));
	stoneSphere->AddComponent<MeshComponent>(stoneSphereModel);
	stoneSphere->AddComponent<MaterialComponent>(stoneMaterial);

	AddEntity(stoneSphere);
#endif

	//Create a pendulum
	auto pendulumHolder = EntityManager::Instance()->CreateEntity("pendulumHolder");
	Ref<PhysicsObject3D> pendulumHolderPhysics = CreateRef<PhysicsObject3D>();
	pendulumHolderPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(0.5f, 0.5f, 0.5f)));
	pendulumHolderPhysics->SetFriction(0.8f);
	pendulumHolderPhysics->SetIsAtRest(true);
	pendulumHolderPhysics->SetInverseMass(1.0);
	pendulumHolderPhysics->SetInverseInertia(pendulumHolderPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pendulumHolderPhysics->SetIsStatic(true);
	pendulumHolderPhysics->SetPosition(Maths::Vector3(12.5f, 15.0f, 20.0f));
	pendulumHolder->AddComponent<Physics3DComponent>(pendulumHolderPhysics);
	pendulumHolder->AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> pendulumHolderModel = AssetsManager::DefaultModels()->Get("Cube");
	pendulumHolder->AddComponent<MeshComponent>(pendulumHolderModel);

	AddEntity(pendulumHolder);

	//Grass
	auto pendulum = EntityManager::Instance()->CreateEntity("pendulum");
	Ref<PhysicsObject3D> pendulumPhysics = CreateRef<PhysicsObject3D>();
	pendulumPhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
	pendulumPhysics->SetFriction(0.8f);
	pendulumPhysics->SetIsAtRest(true);
	pendulumPhysics->SetInverseMass(1.0);
	pendulumPhysics->SetInverseInertia(pendulumPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pendulumPhysics->SetIsStatic(false);
	pendulumPhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 20.0f));
	pendulum->AddComponent<Physics3DComponent>(pendulumPhysics);
	pendulum->AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));

	Ref<Graphics::Mesh> pendulumModel = AssetsManager::DefaultModels()->Get("Sphere");
	pendulum->AddComponent<MeshComponent>(pendulumModel);

	AddEntity(pendulum);

	auto pendulumConstraint = new SpringConstraint(pendulumHolder->GetComponent<Physics3DComponent>()->GetPhysicsObject().get(), pendulum->GetComponent<Physics3DComponent>()->GetPhysicsObject().get(), pendulumHolder->GetComponent<Physics3DComponent>()->GetPhysicsObject()->GetPosition(), pendulum->GetComponent<Physics3DComponent>()->GetPhysicsObject()->GetPosition(), 0.9f, 0.5f);
	Application::Instance()->GetSystem<LumosPhysicsEngine>()->AddConstraint(pendulumConstraint);

#if 0
	auto soundFilePath = String("/Sounds/fire.ogg");
	bool loadedSound = SoundManager::Instance()->AddSound("Background", soundFilePath);

	if(loadedSound)
	{
		auto soundNode = Ref<SoundNode>(SoundNode::Create());
		soundNode->SetSound(SoundManager::Instance()->GetSound("Background"));
		soundNode->SetVolume(1.0f);
		soundNode->SetPosition(Maths::Vector3(0.1f, 10.0f, 10.0f));
		soundNode->SetLooping(true);
		soundNode->SetIsGlobal(false);
		soundNode->SetPaused(false);
		soundNode->SetReferenceDistance(1.0f);
		soundNode->SetRadius(30.0f);

		pendulum->AddComponent<SoundComponent>(soundNode);
	}
#endif

    //plastics
    int numSpheres = 0;
	for (int i = 0; i < 10; i++)
	{
		float roughness = i / 10.0f;
		Maths::Vector4 spec(0.04f);
		Vector4 diffuse(0.9f);

		Ref<Material> m = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.roughnessColour = Vector4(roughness);
		properties.specularColour = spec;
		properties.usingAlbedoMap   = 0.0f;
		properties.usingRoughnessMap = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		m->SetMaterialProperites(properties);

		auto sphere = EntityManager::Instance()->CreateEntity("Sphere" + StringFormat::ToString(numSpheres++));

		sphere->AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translation(Maths::Vector3(i * 2.0f, 30.0f, 0.0f)));
		Ref<Graphics::Mesh> sphereModel = AssetsManager::DefaultModels()->Get("Sphere");
		sphere->AddComponent<MeshComponent>(sphereModel);
		sphere->AddComponent<MaterialComponent>(m);

		AddEntity(sphere);
	}

    //metals
	for (int i = 0; i < 10; i++)
	{
        float roughness = i / 10.0f;
        Vector4 spec(1.0f);
        Vector4 diffuse(0.0f,0.0f,0.0f,1.0f);

		Ref<Material> m = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.roughnessColour = Vector4(roughness);
		properties.specularColour = spec;
		properties.usingAlbedoMap   = 0.0f;
		properties.usingRoughnessMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		m->SetMaterialProperites(properties);

		auto sphere = EntityManager::Instance()->CreateEntity("Sphere" + StringFormat::ToString(numSpheres++));

		sphere->AddComponent<Maths::Transform>(Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translation(Maths::Vector3(i * 2.0f, 33.0f, 0.0f)));
		Ref<Graphics::Mesh> sphereModel = AssetsManager::DefaultModels()->Get("Sphere");
		sphere->AddComponent<MeshComponent>(sphereModel);
		sphere->AddComponent<MaterialComponent>(m);

		AddEntity(sphere);
	}
}

void Scene3D::OnImGui()
{
}
