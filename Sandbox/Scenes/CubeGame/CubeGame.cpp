#include "CubeGame.h"

using namespace Lumos;
using namespace maths;

CubeGame::CubeGame(const std::string& SceneName) : Scene(SceneName) {}

CubeGame::~CubeGame() {}

void CubeGame::OnInit()
{
	Scene::OnInit();
	JMPhysicsEngine::Instance()->SetDampingFactor(0.998f);
	JMPhysicsEngine::Instance()->SetIntegrationType(IntegrationType::INTEGRATION_RUNGE_KUTTA_4);
	JMPhysicsEngine::Instance()->SetBroadphase(new Octree(5, 3, std::make_shared<SortAndSweepBroadphase>()));

	SetWorldRadius(10000.0f);

	LoadModels();

	m_pCamera = new ThirdPersonCamera(45.0f, 1.0f, 1000.0f, (float) m_ScreenWidth / (float) m_ScreenHeight);
	m_pCamera->SetYaw(-40.0f);
	m_pCamera->SetPitch(-20.0f);
	m_pCamera->SetPosition(Vector3(120.0f, 70.0f, 260.0f));

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

	Light* sun = new Light();
	sun->SetDirection(Vector3(26.0f, 22.0f, 48.5f));
	sun->SetPosition(Vector3(26.0f, 22.0f, 48.5f) * 10000.0f);
	m_LightSetup->SetDirectionalLight(sun);

	SoundSystem::Instance()->SetListener(m_pCamera);
}

void CubeGame::OnUpdate(TimeStep* timeStep)
{
	Scene::OnUpdate(timeStep);
}

void CubeGame::OnCleanupScene()
{
	if (m_CurrentScene)
	{
		if (m_pCamera)
		{
			delete m_pCamera;
			m_pCamera = nullptr;
		}
	}
	Scene::OnCleanupScene();
}

void CubeGame::LoadModels()
{
	//STADIUM
	//float roughness = 0.5f;
	//Vector3 spec(0.5f);
	//Vector4 diffuseR(1.0f, 0.0f, 0.0f, 1.0f);
	//Vector4 diffuseB(0.0f, 0.0f, 1.0f, 1.0f);

	//Material* red = new Material(AssetsManager::grass);
	//red->SetAlbedo(diffuseR);
	//red->SetSpecular(spec);
	//red->SetGloss(1.0f - roughness);
	//red->UsingNormalMap(false);

	//Material* blue = new Material(AssetsManager::grass);
	//blue->SetAlbedo(diffuseB);
	//blue->SetSpecular(spec);
	//blue->SetGloss(1.0f - roughness);
	//blue->UsingNormalMap(false);

	//AssetsManager::GOInstance = new Object3D("ground"); //Floor
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
 //   AssetsManager::GOInstance->SetMaterial(new Material(AssetsManager::grass));
	//AssetsManager::GOInstance->SetTextureMatrix(Matrix4::Scale(Vector3(20.0f, 20.0f, 20.0f)));
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 2.0f, 150.0f))*Matrix4::Translation(Vector3(0.0f, 0.0f, 0.0f)));
 //   AssetsManager::GOInstance->SetMaterial(new Material(AssetsManager::grass));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol1";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, -1.0f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 2.0f, 150.0f)));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground2");  //Right Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 1.0f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	////AssetsManager::GOInstance->SetMaterial(m2, true);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol2";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 35.5f, -184.f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(-135, 0, 0));

	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground3"); //Left Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 1.0f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, -0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol3";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 34.f, 184.5f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(135, 0, 0));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground4");  //right Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 20.0f, 1.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol4";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 91.f, -219.f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 20.0f, 1.0f)));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground5"); //Left Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 20.0f, 1.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol5";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 89.f, 219.f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 20.0f, 1.0f)));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground6"); //Left top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 1.0f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

 //   //"groundcol6";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 145.f, 185.f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(45, 0, 0));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground7"); //Right top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 1.0f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol7";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 145.5f, -184.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(-45, 0, 0));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground8");// Roof
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(200.f, 1.0f, 150.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol8";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 180.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(200.f, 1.0f, 150.0f)));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground9_left"); //Back Bottom Left goalside
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 50.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol9_left";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-235.0f, 35.f, 100.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 50.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground9_middle"); //Back Bottom middle goalside
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 20.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol9_middle";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-256.2f, 56.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground9_goal_Bottom");// Back Goal Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(29.f, 1.0f, 50.f))*Matrix4::Translation(Vector3(0.0f, 0.0f, 0.0f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground9_goal_Bottom";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-229.0f, 0.0f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(29.f, 1.0f, 50.f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground9_goal_middle"); //Back Goal middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 30.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue); //test
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	// //"groundcol9_goal_middle";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-249.f, 8.0f /*-08.0f*/, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0f, 30.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground9_goal_Right"); //Back Goal Right
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 30.0f, 1.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground9_goal_Right";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-235.0f, 7.5f, -49.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(20.0, 30.0f, 1.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground9_goal_Left"); //Back Goal Left
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 30.0f, 1.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground9_goal_Left";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-234.70f, 7.5f, 49.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(20.0, 30.0f, 1.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground9_goal_Top");//Back Goal Top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.f, 1.0f, 50.f))*Matrix4::Translation(Vector3(0.0f, 0.0f, 0.0f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground9_goal_Top";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-256.0f, 28.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(20.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground9_right"); //Back Bottom right goalside
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 50.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol9_right";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-235.0f, 35.f, -100.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 50.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 45));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground10"); //Back Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 20.f, 150.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol10";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-270.0f, 90.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 150.0f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground11"); //Back top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 50.f, 150.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol11";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-235.0f, 145.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 50.f, 150.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 135));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground12_left"); //Front Bottom Left goalside
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 50.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol12_left";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 35.f, 100.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 50.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground12_middle"); //Front Bottom middle goalside
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 20.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol12_middle";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(255.f, 56.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground12_goal_Bottom");// Front Goal Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(29.f, 1.0f, 50.f))*Matrix4::Translation(Vector3(0.0f, 0.0f, 0.0f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground12_goal_Bottom";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(229.0f, 0.0f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(29.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground12_goal_middle"); //Front Goal middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 30.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol12_goal_middle";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(249.f, -7.0f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0f, 30.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground12_goal_Right"); //Front Goal Left
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 30.0f, 1.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground12_goal_Right";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 7.5f, -49.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(20.0, 30.0f, 1.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground12_goal_Left"); //Front Goal Right
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 30.0f, 1.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground12_goal_Left";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 7.5f, 50.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(20.0, 30.0f, 1.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground12_goal_Top");//Front Goal Top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.f, 1.0f, 50.f))*Matrix4::Translation(Vector3(0.0f, 0.0f, 0.0f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 1.0f));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "ground12_goal_Top";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(255.0f, 28.0f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(20.f, 1.0f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground12_right"); //Front Bottom right goalside
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 50.f, 50.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol12_right";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 35.f, -100.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 50.f, 50.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -45));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground13"); //Front Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 20.f, 150.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red); //red band between goals
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol13";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(270.0f, 90.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 150.0f)));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground14"); //Front top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0, 50.f, 150.0f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
	//AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 1.f, 0.0f));
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol14";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 145.f, 0.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 50.f, 150.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -135));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground15"); //Front Right Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);


	// //"groundcol15";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-217.f, 36.5f, -167.f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 44.f, 49.5f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 135, -215));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground16");       //Front Right Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 20.f, 49.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol16";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-235.0f, 90.5f, -185.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 49.f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, -45, -0.f));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground17"); //Front Right Top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	// //"groundcol17";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-217.0f, 145.f, -167.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 43.f, 49.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, -45, -35));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground118"); //Front Left Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	// //"groundcol18";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-217.0f, 36.f, 167.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 43.f, 49.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, -135, -215));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground19");       //Front Left Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 20.f, 49.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	// //"groundcol19";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-235.0f, 89.5f, 185.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 49.f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 45, -0.f));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground20"); //Front Left Top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(blue);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol20";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(-217.0f, 144.5f, 167.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 43.2f, 49.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 45, -35));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground21"); //Back Right Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol21";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(217.0f, 36.f, -166.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 44.f, 49.2f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 45, 145));
	//this->AddGameObject(AssetsManager::GOInstance);

	//AssetsManager::GOInstance = new Object3D("ground22");       //Back Right Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 20.f, 49.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol22";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 90.5f, -185.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 49.f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, -135, -0.f));

	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground23"); //Back Right Top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol23";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(216.5f, 145.f, -166.5f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 43.f, 49.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 225, -35));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground24"); //Back Left Bottom
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 40.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol24";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(217.0f, 35.5f, 167.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 42.5f, 48.8f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, -45, 145));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground25");       //Back Left Middle
	//AssetsManager::GOInstance->SetModel(AssetsManager::Cube(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 20.f, 49.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	// //"groundcol25";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(235.0f, 89.5f, 185.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0, 20.f, 49.f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 135, -0.f));
	//this->AddGameObject(AssetsManager::GOInstance);


	//AssetsManager::GOInstance = new Object3D("ground26"); //Back Left Top
	//AssetsManager::GOInstance->SetModel(AssetsManager::Triangle(), false);
	//AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(20.0, 41.f, 40.f))*Matrix4::Translation(Vector3(0.0f, 0.f, 0.f)));
 //   AssetsManager::GOInstance->SetMaterial(red);
	//AssetsManager::GOInstance->SetBoundingRadius(80.0f * 80.f);

	//// "groundcol26";
	//AssetsManager::GOInstance->CreatePhysicsNode();
	//AssetsManager::GOInstance->Physics()->SetPosition(Vector3(217.0f, 145.f, 167.0f));
	//AssetsManager::GOInstance->Physics()->SetCollisionShape(new PyramidCollisionShape(Vector3(1.0, 44.f, 49.0f)));
	//AssetsManager::GOInstance->Physics()->SetOrientation(Quaternion::EulerAnglesToQuaternion(0, -225, -35));
	//this->AddGameObject(AssetsManager::GOInstance);

	//{//Player_1
	//	//AssetsManager::Player_1 = new Player("car");
	//	////set scene
	//	//AssetsManager::Player_1->SetScene(this);

	//	////AssetsManager::Player_1->SetModel(AssetsManager::Car(), false);
	//	//AssetsManager::Player_1->SetModel(AssetsManager::Cube(), false);
	//	////SetMaterial&bumpMap
	//	////AssetsManager::Player_1->SetMaterial(new Material(AssetsManager::marbleCat, false);

	//	//AssetsManager::Player_1->SetLocalTransform(Matrix4::Scale(Vector3(1.0f, 1.0f, 1.0f)));
	//	////AssetsManager::Player_1->SetColour(Vector4(0.2f, 10.0f, 0.5f, 1.0f));
	//	//AssetsManager::Player_1->SetBoundingRadius(1.0f * 1.0f);

	//	//AssetsManager::Player_1->Physics()->name = "car";
	//	//AssetsManager::Player_1->Physics()->SetInverseMass(0.1f);
	//	//AssetsManager::Player_1->Physics()->SetPosition(Vector3(-40.0f, 2.0f, 0.0f));
	//	//AssetsManager::Player_1->Physics()->SetCollisionShape(new CuboidCollisionShape(Vector3(1.0f, 1.0f, 1.0f)));
	//	//AssetsManager::Player_1->Physics()->SetCar(true);

	//	//Matrix3 inertia(1.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	//	//AssetsManager::Player_1->Physics()->SetInverseInertia(inertia);

	//	//this->AddGameObject(AssetsManager::Player_1);

	//	//AssetsManager::Wheel = new ObjectMesh("wheel");
	//	//Matrix3 inertia_2(0.1f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.1f);

	//	//AssetsManager::Wheel->SetModel(AssetsManager::Sphere(), false);
	//	//AssetsManager::Wheel->SetLocalTransform(Matrix4::Scale(Vector3(4.0f, 4.0f, 4.0f)));
	//	//AssetsManager::Wheel->SetColour(Vector4(1.0f, 1.0f, 0.5f, 1.0f));
	//	//AssetsManager::Wheel->SetBoundingRadius(4.0f * 4.f);

	//	//AssetsManager::Wheel->Physics()->name = "Wheel";
	//	//AssetsManager::Wheel->Physics()->SetInverseMass(15.0f);
	//	//AssetsManager::Wheel->Physics()->SetPosition(Vector3(10.0f, 4.5f, 0.0f));
	//	//AssetsManager::Wheel->Physics()->SetCollisionShape(
	//	//	new CylinderCollisionShape(1.0f, 1.0f, Vector3(1.0f, 1.0f, 1.0f)));
	//	//AssetsManager::Wheel->Physics()->SetInverseInertia(inertia);

	//	//this->AddGameObject(AssetsManager::Wheel);
	//}

	//{//ball
	//	AssetsManager::GOInstance = new Object3D("ball");

	//	AssetsManager::GOInstance->SetModel(AssetsManager::Sphere(), false);
	//	AssetsManager::GOInstance->SetLocalTransform(Matrix4::Scale(Vector3(3.0f, 3.0f, 3.0f)));
	//	AssetsManager::GOInstance->SetColour(Vector4(1.0f, 1.0f, 0.5f, 1.0f));
	//	AssetsManager::GOInstance->SetBoundingRadius(4.0f * 4.f);
 //       AssetsManager::GOInstance->SetMaterial(new Material(AssetsManager::castIron));

	//	// "ball";
	//	AssetsManager::GOInstance->CreatePhysicsNode();
	//	AssetsManager::GOInstance->Physics()->SetInverseMass(1.0f);
	//	AssetsManager::GOInstance->Physics()->SetPosition(Vector3(0.0f, 4.5f, 0.0f));
	//	AssetsManager::GOInstance->Physics()->SetCollisionShape(new SphereCollisionShape(3.0f));
	//	AssetsManager::GOInstance->Physics()->SetInverseInertia(
	//		AssetsManager::GOInstance->Physics()->GetCollisionShape()->BuildInverseInertia(1.0f));

	//	this->AddGameObject(AssetsManager::GOInstance);

	///*	std::function<bool(PhysicsObject3D*)> ballHit = [&](PhysicsObject3D* otherObject){
	//		Vector3 temp;
	//		temp = this->m->FindGameObject("ball")->Physics()->GetForce();
	//		this->m_RootGameObject->FindGameObject("ball")->Physics()->SetForce(temp - Vector3(0.f, 2.f, 0.f));
	//		return true;
	//	};

	//	AssetsManager::GOInstance->Physics()->SetOnCollisionCallback(ballHit);*/
	//}
}
