#include "Scene3D.h"
#include "Graphics/MeshFactory.h"

using namespace Lumos;
using namespace Maths;

Scene3D::Scene3D(const String& SceneName)
		: Scene(SceneName)
{
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
    Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetPaused(false);
    Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetDebugDrawFlags(PhysicsDebugFlags::CONSTRAINT | PhysicsDebugFlags::COLLISIONVOLUMES | PhysicsDebugFlags::BROADPHASE);

	LoadModels();
    
    LoadLuaScene(ROOT_DIR"/Sandbox/res/scripts/LuaSceneTest.lua");

	Application::Instance()->GetWindow()->HideMouse(false);

	m_pCamera = new Camera(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
    m_pCamera->SetCameraController(CreateRef<EditorCameraController>(m_pCamera));


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
    
    auto lightEntity = m_Registry.create();
    m_Registry.emplace<Graphics::Light>(lightEntity, Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector4(1.0f), 1.3f);
	m_Registry.emplace<Maths::Transform>(lightEntity,Matrix4::Translation(Maths::Vector3(26.0f, 22.0f, 48.5f)) * Maths::Quaternion::LookAt(Maths::Vector3(26.0f, 22.0f, 48.5f), Maths::Vector3::ZERO).RotationMatrix4());
	m_Registry.emplace<NameComponent>(lightEntity, "Light");

	auto cameraEntity = m_Registry.create();
	m_Registry.emplace<CameraComponent>(cameraEntity, m_pCamera);
	m_Registry.emplace<NameComponent>(cameraEntity, "Camera");

	auto audioSystem = Application::Instance()->GetSystem<AudioManager>();
	if(audioSystem)
		audioSystem->SetListener(m_pCamera);

#ifndef LUMOS_PLATFORM_IOS
    auto shadowRenderer = new Graphics::ShadowRenderer();
    shadowRenderer->SetLightEntity(lightEntity);
    auto shadowLayer = new Layer3D(shadowRenderer);
    Application::Instance()->GetRenderManager()->SetShadowRenderer(shadowRenderer);
    Application::Instance()->PushLayer(shadowLayer);
#endif

	bool editor = false;

#ifdef LUMOS_EDITOR
	editor = true;
#endif

    Application::Instance()->PushLayer(new Layer3D(new Graphics::DeferredRenderer(m_ScreenWidth, m_ScreenHeight, editor), "Deferred"));
	Application::Instance()->PushLayer(new Layer3D(new Graphics::SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap, editor), "Skybox"));

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

	auto ground = m_Registry.create();
	Ref<PhysicsObject3D> testPhysics = CreateRef<PhysicsObject3D>();
	testPhysics->SetRestVelocityThreshold(-1.0f);
	testPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	testPhysics->SetFriction(0.8f);
	testPhysics->SetIsAtRest(true);
	testPhysics->SetIsStatic(true);

	m_Registry.emplace<Maths::Transform>(ground, Matrix4::Scale(Maths::Vector3(groundWidth, groundHeight, groundLength)));
	m_Registry.emplace<Physics3DComponent>(ground, testPhysics);
    
	m_Registry.emplace<Lumos::ScriptComponent>(ground, ROOT_DIR"/Sandbox/res/scripts/LuaComponentTest.lua", this);

	Ref<Graphics::Mesh> groundModel = AssetsManager::DefaultModels()->Get("Cube");
	m_Registry.emplace<MeshComponent>(ground, groundModel);

	MaterialProperties properties;
	properties.albedoColour = Vector4(0.6f,0.1f,0.1f,1.0f);
	properties.roughnessColour = Vector4(0.6f);
	properties.specularColour = Vector4(0.15f);
	properties.usingAlbedoMap     = 0.5f;
	properties.usingRoughnessMap  = 0.0f;
	properties.usingNormalMap     = 0.0f;
	properties.usingSpecularMap   = 0.0f;
	testMaterial->SetMaterialProperites(properties);
	m_Registry.emplace<MaterialComponent>(ground, testMaterial);

	//Create a pendulum
	auto pendulumHolder = m_Registry.create();
	Ref<PhysicsObject3D> pendulumHolderPhysics = CreateRef<PhysicsObject3D>();
	pendulumHolderPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(Maths::Vector3(0.5f, 0.5f, 0.5f)));
	pendulumHolderPhysics->SetFriction(0.8f);
	pendulumHolderPhysics->SetIsAtRest(true);
	pendulumHolderPhysics->SetInverseMass(1.0);
	pendulumHolderPhysics->SetInverseInertia(pendulumHolderPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pendulumHolderPhysics->SetIsStatic(true);
	pendulumHolderPhysics->SetPosition(Maths::Vector3(12.5f, 15.0f, 20.0f));
	m_Registry.emplace<Physics3DComponent>(pendulumHolder,pendulumHolderPhysics);
	m_Registry.emplace<Maths::Transform>(pendulumHolder,Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
	m_Registry.emplace<NameComponent>(pendulumHolder, "Pendulum Holder");
	Ref<Graphics::Mesh> pendulumHolderModel = AssetsManager::DefaultModels()->Get("Cube");
	m_Registry.emplace<MeshComponent>(pendulumHolder,pendulumHolderModel);
	m_Registry.emplace<Lumos::ScriptComponent>(pendulumHolder, ROOT_DIR"/Sandbox/res/scripts/PlayerTest.lua", this);

	auto pendulum = m_Registry.create();
	Ref<PhysicsObject3D> pendulumPhysics = CreateRef<PhysicsObject3D>();
	pendulumPhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(0.5f));
	pendulumPhysics->SetFriction(0.8f);
	pendulumPhysics->SetIsAtRest(true);
	pendulumPhysics->SetInverseMass(1.0);
	pendulumPhysics->SetInverseInertia(pendulumPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pendulumPhysics->SetIsStatic(false);
	pendulumPhysics->SetPosition(Maths::Vector3(12.5f, 10.0f, 20.0f));
	m_Registry.emplace<Physics3DComponent>(pendulum, pendulumPhysics);
	m_Registry.emplace<Maths::Transform>(pendulum, Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
	m_Registry.emplace<NameComponent>(pendulum, "Pendulum");
	Ref<Graphics::Mesh> pendulumModel = AssetsManager::DefaultModels()->Get("Sphere");
	m_Registry.emplace<MeshComponent>(pendulum, pendulumModel);

	auto pendulumConstraint = new SpringConstraint(m_Registry.get<Physics3DComponent>(pendulumHolder).GetPhysicsObject().get(), m_Registry.get<Physics3DComponent>(pendulum).GetPhysicsObject().get(), m_Registry.get<Physics3DComponent>(pendulumHolder).GetPhysicsObject()->GetPosition(), m_Registry.get<Physics3DComponent>(pendulum).GetPhysicsObject()->GetPosition(), 0.9f, 0.5f);
	Application::Instance()->GetSystem<LumosPhysicsEngine>()->AddConstraint(pendulumConstraint);

#if 0
	auto soundFilePath = String("/Sounds/fire.ogg");
	AssetsManager::Sounds()->LoadAsset("Background", soundFilePath);

	{
		auto soundNode = Ref<SoundNode>(SoundNode::Create());
		soundNode->SetSound(AssetsManager::Sounds()->Get("Background").get());
		soundNode->SetVolume(1.0f);
		soundNode->SetPosition(Maths::Vector3(0.1f, 10.0f, 10.0f));
		soundNode->SetLooping(true);
		soundNode->SetIsGlobal(false);
		soundNode->SetPaused(false);
		soundNode->SetReferenceDistance(1.0f);
		soundNode->SetRadius(30.0f);
		m_Registry.emplace<SoundComponent>(pendulum, soundNode);
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

		auto sphere = m_Registry.create();

		m_Registry.emplace<Maths::Transform>(sphere,Matrix4::Translation(Maths::Vector3(float(i), 17.0f, 0.0f)) * Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
		Ref<Graphics::Mesh> sphereModel = AssetsManager::DefaultModels()->Get("Sphere");
		m_Registry.emplace<MeshComponent>(sphere,sphereModel);
		m_Registry.emplace<MaterialComponent>(sphere,m);
		m_Registry.emplace<NameComponent>(sphere, "Sphere" + StringFormat::ToString(numSpheres++));
	}

    //metals
	for (int i = 0; i < 10; i++)
	{
        float roughness = i / 10.0f;
        Vector4 spec(1.0f);
        Vector4 diffuse(0.9f);

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

		auto sphere = m_Registry.create();

		m_Registry.emplace<Maths::Transform>(sphere,Matrix4::Translation(Maths::Vector3(float(i), 18.0f, 0.0f)) * Matrix4::Scale(Maths::Vector3(0.5f, 0.5f, 0.5f)));
		Ref<Graphics::Mesh> sphereModel = AssetsManager::DefaultModels()->Get("Sphere");
		m_Registry.emplace<MeshComponent>(sphere,sphereModel);
		m_Registry.emplace<MaterialComponent>(sphere, m);
		m_Registry.emplace<NameComponent>(sphere, "Sphere" + StringFormat::ToString(numSpheres++));
	}
}

void Scene3D::OnImGui()
{

}
