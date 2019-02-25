#include "Scene3D.h"
#include "Graphics/MeshFactory.h"

using namespace Lumos;
using namespace maths;

maths::Vector3 lightDirection;

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

	LumosPhysicsEngine::Instance()->SetDampingFactor(0.998f);
	LumosPhysicsEngine::Instance()->SetIntegrationType(INTEGRATION_RUNGE_KUTTA_4);
	LumosPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 5, std::make_shared<SortAndSweepBroadphase>()));

	SetDebugDrawFlags( DEBUGDRAW_FLAGS_ENTITY_COMPONENTS | DEBUGDRAW_FLAGS_COLLISIONVOLUMES  );

	SetDrawObjects(true);
	SetUseShadow(true);

	LoadModels();

	m_pCamera = new ThirdPersonCamera(-20.0f, -40.0f, maths::Vector3(-3.0f, 10.0f, 15.0f), 60.0f, 0.1f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
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

	m_EnvironmentMap = TextureCube::CreateFromVCross(environmentFiles, 11);

	auto sun = std::make_shared<Light>();
	sun->SetDirection(maths::Vector3(26.0f, 22.0f, 48.5f));
	sun->SetPosition(maths::Vector3(26.0f, 22.0f, 48.5f) * 100.0f);
	m_LightSetup->SetDirectionalLight(sun);

	lightDirection = maths::Vector3(26.0f, 22.0f, 48.5f);

	Application::Instance()->GetAudioManager()->SetListener(m_pCamera);

	m_ShadowTexture = std::unique_ptr<TextureDepthArray>(TextureDepthArray::Create(4096, 4096, 4));
	auto shadowRenderer = new ShadowRenderer();
	auto deferredRenderer = new DeferredRenderer(m_ScreenWidth, m_ScreenHeight);
	auto skyboxRenderer = new SkyboxRenderer(m_ScreenWidth, m_ScreenHeight, m_EnvironmentMap);
	deferredRenderer->SetRenderTarget(Application::Instance()->GetRenderManager()->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0]);
	skyboxRenderer->SetRenderTarget(Application::Instance()->GetRenderManager()->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0]);

	Application::Instance()->PushLayer(new Layer3D(shadowRenderer));
    Application::Instance()->PushLayer(new Layer3D(deferredRenderer));
	Application::Instance()->PushLayer(new Layer3D(skyboxRenderer));
	Application::Instance()->PushOverLay(new ImGuiLayer(true));

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
		m_ShadowTexture.reset();
	}

	Scene::OnCleanupScene();
}

void Scene3D::LoadModels()
{
	const float groundWidth = 100.0f;
	const float groundHeight = 0.5f;
	const float groundLength = 100.0f;

	auto testMaterial = std::make_shared<Material>();
	testMaterial->LoadMaterial("checkerboard", "/CoreTextures/checkerboard.tga");

	std::shared_ptr<Entity> ground = std::make_shared<Entity>("Ground",this);
	std::shared_ptr<PhysicsObject3D> testPhysics = std::make_shared<PhysicsObject3D>();
	testPhysics->SetRestVelocityThreshold(-1.0f);
	testPhysics->SetCollisionShape(std::make_unique<CuboidCollisionShape>(maths::Vector3(groundWidth, groundHeight, groundLength)));
	testPhysics->SetFriction(0.8f);
	testPhysics->SetIsAtRest(true);
	testPhysics->SetIsStatic(true);

	ground->AddComponent(std::make_unique<TransformComponent>((Matrix4::Translation(maths::Vector3(0.0, 0.0, 0.0f)) *Matrix4::Scale(maths::Vector3(groundWidth, groundHeight, groundLength)))));
	ground->AddComponent(std::make_unique<Physics3DComponent>(testPhysics));

	std::shared_ptr<Model> groundModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Cube"));
	ground->AddComponent(std::make_unique<ModelComponent>(groundModel));

	MaterialProperties properties;
	properties.albedoColour = Vector4(0.6f,0.1f,0.1f,1.0f);
	properties.glossColour = Vector4(0.8f);
	properties.specularColour = Vector4(0.8f);
	properties.usingAlbedoMap   = 0.5f;
	properties.usingGlossMap    = 0.0f;
	properties.usingNormalMap   = 0.0f;
	properties.usingSpecularMap = 0.0f;
	testMaterial->SetMaterialProperites(properties);
	groundModel->SetMaterial(testMaterial);

	AddEntity(ground);

	#ifdef TEST_PBR

	auto grassMaterial = std::make_shared<Material>();
	grassMaterial->LoadPBRMaterial("grass", "/Textures");

	auto stonewallMaterial = std::make_shared<Material>();
	stonewallMaterial->LoadPBRMaterial("stonewall", "/Textures");

	auto castIronMaterial = std::make_shared<Material>();
	castIronMaterial->LoadPBRMaterial("CastIron", "/Textures",".tga");

	auto GunMetalMaterial = std::make_shared<Material>();
	GunMetalMaterial->LoadPBRMaterial("GunMetal", "/Textures",".tga");

	auto WornWoodMaterial = std::make_shared<Material>();
	WornWoodMaterial->LoadPBRMaterial("WornWood", "/Textures",".tga");

	auto marbleMaterial = std::make_shared<Material>();
	marbleMaterial->LoadPBRMaterial("marble", "/Textures");

	auto stoneMaterial = std::make_shared<Material>();
	stoneMaterial->LoadPBRMaterial("stone", "/Textures");

	//Create a Rest Cube
	std::shared_ptr<Entity> cube = std::make_shared<Entity>("cube",this);
	std::shared_ptr<PhysicsObject3D> cubePhysics = std::make_shared<PhysicsObject3D>();
	cubePhysics->SetCollisionShape(std::make_unique<CuboidCollisionShape>(maths::Vector3(0.5f, 0.5f, 0.5f)));
	cubePhysics->SetFriction(0.8f);
	cubePhysics->SetIsAtRest(true);
	cubePhysics->SetInverseMass(1.0);
	cubePhysics->SetInverseInertia(cubePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	cubePhysics->SetIsStatic(false);
	cubePhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 0.0f));
	cube->AddComponent(std::make_unique<Physics3DComponent>(cubePhysics));
	cube->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> cubeModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Cube"));
	cube->AddComponent(std::make_unique<ModelComponent>(cubeModel));

	cube->GetComponent<ModelComponent>()->m_Model->SetMaterial(marbleMaterial);

	AddEntity(cube);

	//Create a Rest Sphere
	std::shared_ptr<Entity> restsphere = std::make_shared<Entity>("Sphere",this);
	std::shared_ptr<PhysicsObject3D> restspherePhysics = std::make_shared<PhysicsObject3D>();
	restspherePhysics->SetCollisionShape(std::make_unique<CuboidCollisionShape>(maths::Vector3(0.5f)));
	restspherePhysics->SetFriction(0.8f);
	restspherePhysics->SetIsAtRest(true);
	restspherePhysics->SetInverseMass(1.0);
	restspherePhysics->SetInverseInertia(restspherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	restspherePhysics->SetIsStatic(false);
	restspherePhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 5.0f));
	restsphere->AddComponent(std::make_unique<Physics3DComponent>(restspherePhysics));
	restsphere->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> restsphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Cube"));
	restsphere->AddComponent(std::make_unique<ModelComponent>(restsphereModel));
	restsphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(castIronMaterial);

	AddEntity(restsphere);

	//Create a Rest Pyramid
	std::shared_ptr<Entity> pyramid = std::make_shared<Entity>("Pyramid",this);
	std::shared_ptr<PhysicsObject3D> pyramidPhysics = std::make_shared<PhysicsObject3D>();
	pyramidPhysics->SetCollisionShape(std::make_unique<PyramidCollisionShape>(maths::Vector3(0.5f)));
	pyramidPhysics->SetFriction(0.8f);
	pyramidPhysics->SetIsAtRest(true);
	pyramidPhysics->SetInverseMass(1.0);
	pyramidPhysics->SetInverseInertia(pyramidPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pyramidPhysics->SetIsStatic(false);
	pyramidPhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 8.0f));
	pyramid->AddComponent(std::make_unique<Physics3DComponent>(pyramidPhysics));
	pyramid->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Rotation(-89.9f, maths::Vector3(1.0f, 0.0f, 0.0f))));

	std::shared_ptr<Model> pyramidModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Pyramid"));
	pyramid->AddComponent(std::make_unique<ModelComponent>(pyramidModel));
	pyramid->GetComponent<ModelComponent>()->m_Model->SetMaterial(marbleMaterial);

	AddEntity(pyramid);

	//Grass
	std::shared_ptr<Entity> grassSphere = std::make_shared<Entity>("grassSphere",this);
	std::shared_ptr<PhysicsObject3D> grassSpherePhysics = std::make_shared<PhysicsObject3D>();
	grassSpherePhysics->SetCollisionShape(std::make_unique<SphereCollisionShape>(0.5f));
	grassSpherePhysics->SetFriction(0.8f);
	grassSpherePhysics->SetIsAtRest(true);
	grassSpherePhysics->SetInverseMass(1.0);
	grassSpherePhysics->SetInverseInertia(grassSpherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	grassSpherePhysics->SetIsStatic(false);
	grassSpherePhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 13.0f));
	grassSphere->AddComponent(std::make_unique<Physics3DComponent>(grassSpherePhysics));
	grassSphere->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> grassSphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
	grassSphere->AddComponent(std::make_unique<ModelComponent>(grassSphereModel));
	grassSphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(grassMaterial);

	AddEntity(grassSphere);

	//Marble
	std::shared_ptr<Entity> marbleSphere = std::make_shared<Entity>("marbleSphere",this);
	std::shared_ptr<PhysicsObject3D> marbleSpherePhysics = std::make_shared<PhysicsObject3D>();
	marbleSpherePhysics->SetCollisionShape(std::make_unique<SphereCollisionShape>(0.5f));
	marbleSpherePhysics->SetFriction(0.8f);
	marbleSpherePhysics->SetIsAtRest(true);
	marbleSpherePhysics->SetInverseMass(1.0);
	marbleSpherePhysics->SetInverseInertia(marbleSpherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	marbleSpherePhysics->SetIsStatic(false);
	marbleSpherePhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 15.0f));
	marbleSphere->AddComponent(std::make_unique<Physics3DComponent>(marbleSpherePhysics));
	marbleSphere->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> marbleSphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
	marbleSphere->AddComponent(std::make_unique<ModelComponent>(marbleSphereModel));
	marbleSphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(marbleMaterial);

	AddEntity(marbleSphere);

	//stone
	std::shared_ptr<Entity> stoneSphere = std::make_shared<Entity>("stoneSphere",this);
	std::shared_ptr<PhysicsObject3D> stoneSpherePhysics = std::make_shared<PhysicsObject3D>();
	stoneSpherePhysics->SetCollisionShape(std::make_unique<SphereCollisionShape>(0.5f));
	stoneSpherePhysics->SetFriction(0.8f);
	stoneSpherePhysics->SetIsAtRest(true);
	stoneSpherePhysics->SetInverseMass(1.0);
	stoneSpherePhysics->SetInverseInertia(stoneSpherePhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	stoneSpherePhysics->SetIsStatic(false);
	stoneSpherePhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 17.0f));
	stoneSphere->AddComponent(std::make_unique<Physics3DComponent>(stoneSpherePhysics));
	stoneSphere->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> stoneSphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
	stoneSphere->AddComponent(std::make_unique<ModelComponent>(stoneSphereModel));
	stoneSphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(stoneMaterial);

	AddEntity(stoneSphere);
#endif

	//Create a pendulum
	std::shared_ptr<Entity> pendulumHolder = std::make_shared<Entity>("pendulumHolder",this);
	std::shared_ptr<PhysicsObject3D> pendulumHolderPhysics = std::make_shared<PhysicsObject3D>();
	pendulumHolderPhysics->SetCollisionShape(std::make_unique<CuboidCollisionShape>(maths::Vector3(0.5f, 0.5f, 0.5f)));
	pendulumHolderPhysics->SetFriction(0.8f);
	pendulumHolderPhysics->SetIsAtRest(true);
	pendulumHolderPhysics->SetInverseMass(1.0);
	pendulumHolderPhysics->SetInverseInertia(pendulumHolderPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pendulumHolderPhysics->SetIsStatic(true);
	pendulumHolderPhysics->SetPosition(maths::Vector3(12.5f, 15.0f, 20.0f));
	pendulumHolder->AddComponent(std::make_unique<Physics3DComponent>(pendulumHolderPhysics));
	pendulumHolder->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> pendulumHolderModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Cube"));
	pendulumHolder->AddComponent(std::make_unique<ModelComponent>(pendulumHolderModel));

	AddEntity(pendulumHolder);

	//Grass
	std::shared_ptr<Entity> pendulum = std::make_shared<Entity>("pendulum",this);
	std::shared_ptr<PhysicsObject3D> pendulumPhysics = std::make_shared<PhysicsObject3D>();
	pendulumPhysics->SetCollisionShape(std::make_unique<SphereCollisionShape>(0.5f));
	pendulumPhysics->SetFriction(0.8f);
	pendulumPhysics->SetIsAtRest(true);
	pendulumPhysics->SetInverseMass(1.0);
	pendulumPhysics->SetInverseInertia(pendulumPhysics->GetCollisionShape()->BuildInverseInertia(1.0f));
	pendulumPhysics->SetIsStatic(false);
	pendulumPhysics->SetPosition(maths::Vector3(12.5f, 10.0f, 20.0f));
	pendulum->AddComponent(std::make_unique<Physics3DComponent>(pendulumPhysics));
	pendulum->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))));

	std::shared_ptr<Model> pendulumModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
	pendulum->AddComponent(std::make_unique<ModelComponent>(pendulumModel));

	AddEntity(pendulum);

	auto pendulumConstraint = new SpringConstraint(pendulumHolder->GetComponent<Physics3DComponent>()->m_PhysicsObject.get(), pendulum->GetComponent<Physics3DComponent>()->m_PhysicsObject.get(), pendulumHolder->GetComponent<Physics3DComponent>()->m_PhysicsObject->GetPosition(), pendulum->GetComponent<Physics3DComponent>()->m_PhysicsObject->GetPosition(), 0.9f, 0.5f);
	LumosPhysicsEngine::Instance()->AddConstraint(pendulumConstraint);

#if 0
	auto soundFilePath = String("/Sounds/fire.ogg");
	bool loadedSound = Sound::AddSound("Background", soundFilePath);

	if(loadedSound)
	{
		auto soundNode = std::shared_ptr<SoundNode>(SoundNode::Create());
		soundNode->SetSound(Sound::GetSound("Background"));
		soundNode->SetVolume(1.0f);
		soundNode->SetPosition(maths::Vector3(0.1f, 10.0f, 10.0f));
		soundNode->SetLooping(true);
		soundNode->SetIsGlobal(false);
		soundNode->SetPaused(false);
		soundNode->SetReferenceDistance(1.0f);
		soundNode->SetRadius(30.0f);

		pendulum->AddComponent(std::make_unique<SoundComponent>(soundNode));
	}
#endif

    int numSpheres = 0;
	for (int i = 0; i < 10; i++)
	{
		float roughness = i / 10.0f;
		maths::Vector4 spec(0.1f,0.1f,0.1f,1.0f);
		Vector4 diffuse(1.0f, 0.0f, 0.0f, 1.0f);

		std::shared_ptr<Material> m = std::make_shared<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.glossColour = Vector4(1.0f - roughness);
		properties.specularColour = spec;
		properties.usingAlbedoMap   = 0.0f;
		properties.usingGlossMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		m->SetMaterialProperites(properties);

        std::shared_ptr<Entity> sphere = std::make_shared<Entity>("Sphere" + StringFormat::ToString(numSpheres++),this);

		sphere->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translation(maths::Vector3(i * 2.0f, 30.0f, 0.0f))));
		sphere->AddComponent(std::make_unique<TextureMatrixComponent>(Matrix4()));
		std::shared_ptr<Model> sphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
		sphere->AddComponent(std::make_unique<ModelComponent>(sphereModel));
		sphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(m);

		AddEntity(sphere);
	}

	for (int i = 0; i < 10; i++)
	{
		float roughness = i / 10.0f;
		Vector4 spec(0.9f);
		Vector4 diffuse(0.0f, 0.0f, 0.0f, 1.0f);

		std::shared_ptr<Material> m = std::make_shared<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.glossColour = Vector4(1.0f - roughness);
		properties.specularColour = spec;
		properties.usingAlbedoMap   = 0.0f;
		properties.usingGlossMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		m->SetMaterialProperites(properties);

		std::shared_ptr<Entity> sphere = std::make_shared<Entity>("Sphere" + StringFormat::ToString(numSpheres++),this);

		sphere->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translation(maths::Vector3(i * 2.0f, 30.0f, 3.0f))));
		sphere->AddComponent(std::make_unique<TextureMatrixComponent>(Matrix4()));
		std::shared_ptr<Model> sphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
		sphere->AddComponent(std::make_unique<ModelComponent>(sphereModel));
		sphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(m);

		AddEntity(sphere);
	}

    int numCubes = 0;
	for (int i = 0; i < 10; i++)
	{
		float roughness = i / 10.0f;
		Vector4 spec(i / 10.0f,i / 10.0f,i / 10.0f,1.0f);
		Vector4 diffuse(1.0f, 0.0f, 0.0f, 1.0f);

		std::shared_ptr<Material> m = std::make_shared<Material>();
		MaterialProperties properties;
		properties.albedoColour = diffuse;
		properties.glossColour = Vector4(1.0f - roughness);
		properties.specularColour = spec;
		properties.usingAlbedoMap   = 0.0f;
		properties.usingGlossMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		m->SetMaterialProperites(properties);

		std::shared_ptr<Entity> cube2 = std::make_shared<Entity>("Cube" + StringFormat::ToString(numCubes++),this);

		cube2->AddComponent(std::make_unique<TransformComponent>(Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translation(maths::Vector3(i * 2.0f, 30.0f, -3.0f))));
		cube2->AddComponent(std::make_unique<TextureMatrixComponent>(Matrix4()));
		std::shared_ptr<Model> cubeModel1 = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Cube"));
		cube2->AddComponent(std::make_unique<ModelComponent>(cubeModel1));
		cube2->GetComponent<ModelComponent>()->m_Model->SetMaterial(m);

		AddEntity(cube2);
	}
}

bool show_demo_window = true;

void Scene3D::OnIMGUI()
{
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

	//ImGui::Begin(m_SceneName.c_str());
    
    ImVec4 test = ImVec4(lightDirection.GetX(),lightDirection.GetY(), lightDirection.GetZ(), 1.0f);
    
    ImGui::Text("Light");
    ImGui::DragFloat4("Direction", &test.x);
    
    lightDirection = maths::Vector3(test.x,test.y,test.z);
    m_LightSetup->GetDirectionalLight()->SetDirection(lightDirection);
    
 	if(ImGui::Button("<- SceneSelect"))
	{
		Application::Instance()->GetSceneManager()->JumpToScene("SceneSelect");
		//ImGui::End();
		return;
	}

    //ImGui::End();
}
