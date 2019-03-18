#include "LM.h"
#include "CommonUtils.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Utilities/AssetsManager.h"
#include "Physics/LumosPhysicsEngine/SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/Model/Model.h"
#include "Utilities/RandomNumberGenerator.h"
#include "App/SceneManager.h"
#include "App/Scene.h"
#include "App/Application.h"

namespace Lumos
{
	using namespace maths;

	maths::Vector4 CommonUtils::GenColour(float scalar, float alpha)
	{
		maths::Vector4 c;
		c.SetW(alpha);

		c.SetX(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		c.SetY(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		c.SetZ(RandomNumberGenerator32::Rand(0.0f, 1.0f));

		return c;
	}

	std::shared_ptr<Entity> CommonUtils::BuildSphereObject(
		const std::string& name,
		const maths::Vector3& pos,
		float radius,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const maths::Vector4& color)
	{
		std::shared_ptr<Entity> pSphere = std::make_shared<Entity>(name, Application::Instance()->GetSceneManager()->GetCurrentScene());

		pSphere->AddComponent(std::make_unique<TextureMatrixComponent>(maths::Matrix4::Scale(maths::Vector3(10.0f, 10.0f, 10.0f))));
		std::shared_ptr<Model> sphereModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Sphere").get());
		pSphere->AddComponent(std::make_unique<ModelComponent>(sphereModel));

		std::shared_ptr<Material> matInstance = std::make_shared<Material>();
		MaterialProperties properties;
		properties.albedoColour = color;
		properties.glossColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.specularColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.usingAlbedoMap   = 0.0f;
		properties.usingGlossMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		matInstance->SetMaterialProperites(properties);
		pSphere->GetComponent<ModelComponent>()->m_Model->SetMaterial(matInstance);

		pSphere->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4::Scale(maths::Vector3(radius, radius, radius))));// *maths::Matrix4::Translation(pos)));
		pSphere->SetBoundingRadius(radius);

		if (physics_enabled)
		{
			//Otherwise create a physics object, and set it's position etc
			std::shared_ptr<PhysicsObject3D> testPhysics = std::make_shared<PhysicsObject3D>();

			testPhysics->SetPosition(pos);
			testPhysics->SetInverseMass(inverse_mass);

			if (!collidable)
			{
				//Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
				testPhysics->SetInverseInertia(SphereCollisionShape(radius).BuildInverseInertia(inverse_mass));
			}
			else
			{
				testPhysics->SetCollisionShape(std::make_unique<SphereCollisionShape>(radius));
				testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
			}

			pSphere->AddComponent(std::make_unique<Physics3DComponent>(testPhysics));
		}

		return pSphere;
	}

	std::shared_ptr<Entity> CommonUtils::BuildCuboidObject(
		const std::string& name,
		const maths::Vector3& pos,
		const maths::Vector3& halfdims,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const maths::Vector4& color)
	{
		std::shared_ptr<Entity> Cube = std::make_shared<Entity>(name, Application::Instance()->GetSceneManager()->GetCurrentScene());

		Cube->AddComponent(std::make_unique<TextureMatrixComponent>(maths::Matrix4::Scale(maths::Vector3(10.0f, 10.0f, 10.0f))));
		std::shared_ptr<Model> cubeModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Cube").get());
		Cube->AddComponent(std::make_unique<ModelComponent>(cubeModel));

		Material* matInstance = new Material();//std::shared_ptr<Shader>(Shader::CreateFromFile("ForwardRender", "/Shaders/Scene/ForwardRender")));
		//matInstance->SetUniform("uColour", color);
		MaterialProperties properties;
		properties.albedoColour = color;
		properties.glossColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.specularColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.usingAlbedoMap   = 0.0f;
		properties.usingGlossMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		matInstance->SetMaterialProperites(properties);
		matInstance->SetRenderFlags(0);
		matInstance->SetRenderFlag(Material::RenderFlags::FORWARDRENDER); //TODO: Temporary;
		Cube->GetComponent<ModelComponent>()->m_Model->SetMaterial(std::make_shared<Material>(*matInstance));

		Cube->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4::Scale(halfdims)));
		Cube->SetBoundingRadius(halfdims.Length());

		if (physics_enabled)
		{
			//Otherwise create a physics object, and set it's position etc
			std::shared_ptr<PhysicsObject3D> testPhysics = std::make_shared<PhysicsObject3D>();

			testPhysics->SetPosition(pos);
			testPhysics->SetInverseMass(inverse_mass);
			
			if (!collidable)
			{
				//Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
				testPhysics->SetInverseInertia(CuboidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
			}
			else
			{
				testPhysics->SetCollisionShape(std::make_unique<CuboidCollisionShape>(halfdims));
				testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
			}

			Cube->AddComponent(std::make_unique<Physics3DComponent>(testPhysics));
		}

		return Cube;
	}

	std::shared_ptr<Entity> CommonUtils::BuildPyramidObject(
		const std::string& name,
		const maths::Vector3& pos,
		const maths::Vector3& halfdims,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const maths::Vector4& color)
	{
		std::shared_ptr<Entity> Cube = std::make_shared<Entity>(name, Application::Instance()->GetSceneManager()->GetCurrentScene());

		Cube->AddComponent(std::make_unique<TextureMatrixComponent>(maths::Matrix4::Scale(maths::Vector3(10.0f, 10.0f, 10.0f))));
		std::shared_ptr<Model> pyramidModel = std::make_shared<Model>(*AssetsManager::DefaultModels()->GetAsset("Pyramid").get());
		Cube->AddComponent(std::make_unique<ModelComponent>(pyramidModel));

		std::shared_ptr<Material> matInstance = std::make_shared<Material>();// *AssetsManager::s_DefualtPBRMaterial);
		//matInstance->SetAlbedo(color);
		MaterialProperties properties;
		properties.albedoColour = color;
		properties.glossColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.specularColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.usingAlbedoMap   = 0.0f;
		properties.usingGlossMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		matInstance->SetMaterialProperites(properties);
		Cube->GetComponent<ModelComponent>()->m_Model->SetMaterial(matInstance);

		Cube->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4::Scale(halfdims) * maths::Matrix4::RotationX(-90.0f)));
		Cube->SetBoundingRadius(halfdims.Length());

		if (physics_enabled)
		{
			//Otherwise create a physics object, and set it's position etc
			std::shared_ptr<PhysicsObject3D> testPhysics = std::make_shared<PhysicsObject3D>();

			testPhysics->SetPosition(pos);
			testPhysics->SetInverseMass(inverse_mass);

			if (!collidable)
			{
				//Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
				testPhysics->SetInverseInertia(PyramidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
			}
			else
			{
				testPhysics->SetCollisionShape(std::make_unique<PyramidCollisionShape>(halfdims));
				testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
			}

			Cube->AddComponent(std::make_unique<Physics3DComponent>(testPhysics));
		}

		return Cube;
	}

	void CommonUtils::AddLightCube(Scene* scene)
	{
		maths::Vector3 colour = maths::Vector3(RandomNumberGenerator32::Rand(0.0f, 1.0f),
								 RandomNumberGenerator32::Rand(0.0f, 1.0f),
								 RandomNumberGenerator32::Rand(0.0f, 1.0f));

		std::shared_ptr<Entity> cube = CommonUtils::BuildCuboidObject(
				"light Cube",
				scene->GetCamera()->GetPosition(),
				maths::Vector3(0.5f, 0.5f, 0.5f),
				true,
				1.0f,
				true,
				maths::Vector4(colour, 1.0f));

		cube->GetComponent<Physics3DComponent>()->m_PhysicsObject->SetIsAtRest(true);
		const float radius    = RandomNumberGenerator32::Rand(0.0f, 10.0f);
		const float intensity = RandomNumberGenerator32::Rand(0.0f, 10.0f);

		std::shared_ptr<Light> light = std::make_shared<Light>(scene->GetCamera()->GetPosition(), colour, radius, intensity, LightType::PointLight);
		cube->AddComponent(std::make_unique<LightComponent>(light));
		scene->AddEntity(cube);
	}

	void CommonUtils::AddSphere(Scene* scene)
	{
		std::shared_ptr<Entity> sphere = CommonUtils::BuildSphereObject(
				"Sphere",
				scene->GetCamera()->GetPosition(),
				0.5f,
				true,
				1.0f,
				true,
				maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						1.0f));

		scene->GetCamera()->BuildViewMatrix();
		const maths::Matrix4 temp = scene->GetCamera()->GetViewMatrix();
		maths::Matrix3 viewRotation = maths::Matrix3(temp);
		viewRotation = maths::Matrix3::Inverse(viewRotation);
		const maths::Vector3 forward = viewRotation * maths::Vector3(0.0f, 0.0f, -1.0f);
		sphere->GetComponent<Physics3DComponent>()->m_PhysicsObject->SetLinearVelocity(forward * 30.0f);

		scene->AddEntity(sphere);
	}

	void CommonUtils::AddPyramid(Scene* scene)
	{
		std::shared_ptr<Entity> sphere = CommonUtils::BuildPyramidObject(
				"Pyramid",
				scene->GetCamera()->GetPosition(),
				maths::Vector3(0.5f),
				true,
				1.0f,
				true,
				maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						1.0f));

		scene->GetCamera()->BuildViewMatrix();
		const maths::Matrix4 temp = scene->GetCamera()->GetViewMatrix();
		maths::Matrix3 viewRotation = maths::Matrix3(temp);
		viewRotation = maths::Matrix3::Inverse(viewRotation);
		const maths::Vector3 forward = viewRotation * maths::Vector3(0.0f, 0.0f, -1.0f);
		sphere->GetComponent<Physics3DComponent>()->m_PhysicsObject->SetLinearVelocity(forward * 30.0f);
		sphere->GetComponent<TransformComponent>()->m_Transform.SetWorldMatrix(maths::Matrix4::Scale(maths::Vector3(0.5f, 0.5f, 0.5f))
																		* maths::Matrix4::Rotation(-89.9f, maths::Vector3(1.0f, 0.0f, 0.0f)));

		scene->AddEntity(sphere);
	}
}
