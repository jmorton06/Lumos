#include "LM.h"
#include "CommonUtils.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Utilities/AssetsManager.h"
#include "Physics/LumosPhysicsEngine/SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Utilities/RandomNumberGenerator.h"
#include "App/SceneManager.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Light.h"
#include "ECS/EntityManager.h"
#include "ECS/Component/Components.h"

namespace Lumos
{
	using namespace Maths;

	Maths::Vector4 CommonUtils::GenColour(float alpha)
	{
		Maths::Vector4 c;
		c.SetW(alpha);

		c.SetX(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		c.SetY(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		c.SetZ(RandomNumberGenerator32::Rand(0.0f, 1.0f));

		return c;
	}

	Entity* CommonUtils::BuildSphereObject(
		const std::string& name,
		const Maths::Vector3& pos,
		float radius,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const Maths::Vector4& color)
	{
		Entity* pSphere = EntityManager::Instance()->CreateEntity(name);

		pSphere->AddComponent<TextureMatrixComponent>(Maths::Matrix4::Scale(Maths::Vector3(10.0f, 10.0f, 10.0f)));
        Ref<Graphics::Mesh> sphereModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->GetAsset("Sphere"));
        pSphere->AddComponent<MeshComponent>(sphereModel);

		Ref<Material> matInstance = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = color;
		properties.roughnessColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.specularColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.usingAlbedoMap   = 0.0f;
		properties.usingRoughnessMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		matInstance->SetMaterialProperites(properties);
		pSphere->AddComponent<MaterialComponent>(matInstance);

		pSphere->AddComponent<TransformComponent>(Maths::Matrix4::Scale(Maths::Vector3(radius, radius, radius)));
		pSphere->SetBoundingRadius(radius);

		if (physics_enabled)
		{
			//Otherwise create a physics object, and set it's position etc
			Ref<PhysicsObject3D> testPhysics = CreateRef<PhysicsObject3D>();

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

			pSphere->AddComponent<Physics3DComponent>(testPhysics);
		}
		else
		{
			pSphere->GetTransformComponent()->GetTransform().SetLocalPosition(pos);
		}

		return pSphere;
	}

	Entity* CommonUtils::BuildCuboidObject(
		const std::string& name,
		const Maths::Vector3& pos,
		const Maths::Vector3& halfdims,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const Maths::Vector4& color)
	{
		Entity* Cube = EntityManager::Instance()->CreateEntity(name);

		Cube->AddComponent<TextureMatrixComponent>(Maths::Matrix4::Scale(Maths::Vector3(10.0f, 10.0f, 10.0f)));
        Ref<Graphics::Mesh> cubeModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->GetAsset("Cube"));
        Cube->AddComponent<MeshComponent>(cubeModel);

		auto matInstance = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = color;
		properties.roughnessColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.specularColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.usingAlbedoMap   = 0.0f;
		properties.usingRoughnessMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		matInstance->SetMaterialProperites(properties);
		matInstance->SetRenderFlags(0);
		Cube->AddComponent<MaterialComponent>(matInstance);

		Cube->AddComponent<TransformComponent>(Maths::Matrix4::Scale(halfdims));
		Cube->SetBoundingRadius(halfdims.Length());

		if (physics_enabled)
		{
			//Otherwise create a physics object, and set it's position etc
			Ref<PhysicsObject3D> testPhysics = CreateRef<PhysicsObject3D>();

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

			Cube->AddComponent<Physics3DComponent>(testPhysics);
		}
		else
		{
			Cube->GetTransformComponent()->GetTransform().SetLocalPosition(pos);
		}

		return Cube;
	}

	Entity* CommonUtils::BuildPyramidObject(
		const std::string& name,
		const Maths::Vector3& pos,
		const Maths::Vector3& halfdims,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const Maths::Vector4& color)
	{
		Entity* Cube = EntityManager::Instance()->CreateEntity(name);
		Entity* meshEntity = EntityManager::Instance()->CreateEntity("Mesh");

        Ref<Graphics::Mesh> pyramidModel = CreateRef<Graphics::Mesh>(*AssetsManager::DefaultModels()->GetAsset("Pyramid"));
		meshEntity->AddComponent<MeshComponent>(pyramidModel);

		Ref<Material> matInstance = CreateRef<Material>();
		MaterialProperties properties;
		properties.albedoColour = color;
		properties.roughnessColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.specularColour = Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f));
		properties.usingAlbedoMap   = 0.0f;
		properties.usingRoughnessMap    = 0.0f;
		properties.usingNormalMap   = 0.0f;
		properties.usingSpecularMap = 0.0f;
		matInstance->SetMaterialProperites(properties);
		meshEntity->AddComponent<MaterialComponent>(matInstance);

		meshEntity->AddComponent<TransformComponent>(Maths::Matrix4::Scale(halfdims) * Maths::Matrix4::RotationX(-90.0f));
		meshEntity->SetBoundingRadius(halfdims.Length());
		
		Cube->AddChild(meshEntity);

		if (physics_enabled)
		{
			//Otherwise create a physics object, and set it's position etc
			Ref<PhysicsObject3D> testPhysics = CreateRef<PhysicsObject3D>();

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

			Cube->AddComponent<Physics3DComponent>(testPhysics);
		}
		else
		{
			Cube->GetTransformComponent()->GetTransform().SetLocalPosition(pos);
		}

		return Cube;
	}

	void CommonUtils::AddLightCube(Scene* scene)
	{
		Maths::Vector4 colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f),
								 RandomNumberGenerator32::Rand(0.0f, 1.0f),
								 RandomNumberGenerator32::Rand(0.0f, 1.0f),1.0f);

		Entity* cube = CommonUtils::BuildCuboidObject(
				"light Cube",
				scene->GetCamera()->GetPosition(),
				Maths::Vector3(0.5f, 0.5f, 0.5f),
				true,
				1.0f,
				true,
				colour);

		cube->GetComponent<Physics3DComponent>()->GetPhysicsObject()->SetIsAtRest(true);
		const float radius    = RandomNumberGenerator32::Rand(1.0f, 30.0f);
		const float intensity = RandomNumberGenerator32::Rand(0.0f, 2.0f);

		Ref<Graphics::Light> light = CreateRef<Graphics::Light>(scene->GetCamera()->GetPosition(), colour,  intensity, Graphics::LightType::PointLight, scene->GetCamera()->GetPosition(), radius);
		cube->AddComponent<LightComponent>(light);
		scene->AddEntity(cube);
	}

	void CommonUtils::AddSphere(Scene* scene)
	{
		Entity* sphere = CommonUtils::BuildSphereObject(
				"Sphere",
				scene->GetCamera()->GetPosition(),
				0.5f,
				true,
				1.0f,
				true,
				Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						1.0f));

		scene->GetCamera()->BuildViewMatrix();
		const Maths::Matrix4 temp = scene->GetCamera()->GetViewMatrix();
		Maths::Matrix3 viewRotation = Maths::Matrix3(temp);
		viewRotation = Maths::Matrix3::Inverse(viewRotation);
		const Maths::Vector3 forward = viewRotation * Maths::Vector3(0.0f, 0.0f, -1.0f);
		sphere->GetComponent<Physics3DComponent>()->GetPhysicsObject()->SetLinearVelocity(forward * 30.0f);

		scene->AddEntity(sphere);
	}

	void CommonUtils::AddPyramid(Scene* scene)
	{
		Entity* sphere = CommonUtils::BuildPyramidObject(
				"Pyramid",
				scene->GetCamera()->GetPosition(),
				Maths::Vector3(0.5f),
				true,
				1.0f,
				true,
				Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						RandomNumberGenerator32::Rand(0.0f, 1.0f),
						1.0f));

		scene->GetCamera()->BuildViewMatrix();
		const Maths::Matrix4 temp = scene->GetCamera()->GetViewMatrix();
		Maths::Matrix3 viewRotation = Maths::Matrix3(temp);
		viewRotation = Maths::Matrix3::Inverse(viewRotation);
		const Maths::Vector3 forward = viewRotation * Maths::Vector3(0.0f, 0.0f, -1.0f);
		sphere->GetComponent<Physics3DComponent>()->GetPhysicsObject()->SetLinearVelocity(forward * 30.0f);

		scene->AddEntity(sphere);
	}
}
