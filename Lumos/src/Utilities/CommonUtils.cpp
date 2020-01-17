#include "lmpch.h"
#include "CommonUtils.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Utilities/AssetsManager.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Utilities/RandomNumberGenerator.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Light.h"
#include "ECS/Component/Components.h"
#include "Maths/Transform.h"

namespace Lumos
{
	using namespace Maths;

	Maths::Vector4 CommonUtils::GenColour(float alpha)
	{
		Maths::Vector4 c;
		c.w = alpha;

		c.x = RandomNumberGenerator32::Rand(0.0f, 1.0f);
		c.y = RandomNumberGenerator32::Rand(0.0f, 1.0f);
		c.z = RandomNumberGenerator32::Rand(0.0f, 1.0f);

		return c;
	}

	entt::entity CommonUtils::BuildSphereObject(
		entt::registry& registry,
		const std::string& name,
		const Maths::Vector3& pos,
		float radius,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const Maths::Vector4& color)
	{
		auto pSphere = registry.create();
		registry.assign<NameComponent>(pSphere, name);
	
		Ref<Graphics::Mesh> sphereModel = AssetsManager::DefaultModels()->Get("Sphere");
		registry.assign<MeshComponent>(pSphere, sphereModel);

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
		registry.assign<MaterialComponent>(pSphere, matInstance);
		registry.assign<Maths::Transform>(pSphere, Maths::Matrix4::Scale(Maths::Vector3(radius, radius, radius)));

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
				testPhysics->SetCollisionShape(CreateRef<SphereCollisionShape>(radius));
				testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
			}

			registry.assign<Physics3DComponent>(pSphere, testPhysics);
		}
		else
		{
			registry.get<Maths::Transform>(pSphere).SetLocalPosition(pos);
		}

		return pSphere;
	}

	entt::entity CommonUtils::BuildCuboidObject(
		entt::registry& registry,
		const std::string& name,
		const Maths::Vector3& pos,
		const Maths::Vector3& halfdims,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const Maths::Vector4& color)
	{
		auto cube = registry.create();
		registry.assign<NameComponent>(cube, name);

		Ref<Graphics::Mesh> cubeModel = AssetsManager::DefaultModels()->Get("Cube");
		registry.assign<MeshComponent>(cube, cubeModel);

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
		registry.assign<MaterialComponent>(cube, matInstance);
		registry.assign<Maths::Transform>(cube, Maths::Matrix4::Scale(halfdims));

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
				testPhysics->SetCollisionShape(CreateRef<CuboidCollisionShape>(halfdims));
				testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
			}

			registry.assign<Physics3DComponent>(cube, testPhysics);
		}
		else
		{
			registry.get<Maths::Transform>(cube).SetLocalPosition(pos);
		}

		return cube;
	}

	entt::entity CommonUtils::BuildPyramidObject(
		entt::registry& registry,
		const std::string& name,
		const Maths::Vector3& pos,
		const Maths::Vector3& halfdims,
		bool physics_enabled,
		float inverse_mass,
		bool collidable,
		const Maths::Vector4& color)
	{
		auto pyramid = registry.create();
		registry.assign<NameComponent>(pyramid, name);
		registry.assign<Maths::Transform>(pyramid);

		auto pyramidMeshEntity = registry.create();
		Ref<Graphics::Mesh> pyramidModel = AssetsManager::DefaultModels()->Get("Pyramid");

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
		registry.assign<MaterialComponent>(pyramidMeshEntity, matInstance);
		registry.assign<Maths::Transform>(pyramidMeshEntity, Maths::Quaternion(-90.0f, 0.0f,0.0f).RotationMatrix4() * Maths::Matrix4::Scale(halfdims));
		registry.assign<Hierarchy>(pyramidMeshEntity, pyramid);
		registry.assign<MeshComponent>(pyramidMeshEntity, pyramidModel);

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
				testPhysics->SetCollisionShape(CreateRef<PyramidCollisionShape>(halfdims));
				testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
			}

			registry.assign<Physics3DComponent>(pyramid, testPhysics);
		}
		else
		{
			registry.get<Maths::Transform>(pyramid).SetLocalPosition(pos);
		}

		return pyramid;
	}

	void CommonUtils::AddLightCube(Scene* scene)
	{
		Maths::Vector4 colour = Maths::Vector4(RandomNumberGenerator32::Rand(0.0f, 1.0f),
								 RandomNumberGenerator32::Rand(0.0f, 1.0f),
								 RandomNumberGenerator32::Rand(0.0f, 1.0f),1.0f);

		entt::registry& registry = scene->GetRegistry();

		auto cube = CommonUtils::BuildCuboidObject(
				registry,
				"light Cube",
				scene->GetCamera()->GetPosition(),
				Maths::Vector3(0.5f, 0.5f, 0.5f),
				true,
				1.0f,
				true,
				colour);

		registry.get<Physics3DComponent>(cube).GetPhysicsObject()->SetIsAtRest(true);
		const float radius    = RandomNumberGenerator32::Rand(1.0f, 30.0f);
		const float intensity = RandomNumberGenerator32::Rand(0.0f, 2.0f);

		registry.assign<Graphics::Light>(cube, scene->GetCamera()->GetPosition(), colour, intensity, Graphics::LightType::PointLight, scene->GetCamera()->GetPosition(), radius);
	}

	void CommonUtils::AddSphere(Scene* scene)
	{
		entt::registry& registry = scene->GetRegistry();

		auto sphere = CommonUtils::BuildSphereObject(
				registry,
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

		const Maths::Vector3 forward = -scene->GetCamera()->GetForwardDirection();
		registry.get<Physics3DComponent>(sphere).GetPhysicsObject()->SetLinearVelocity(forward * 30.0f);
	}

	void CommonUtils::AddPyramid(Scene* scene)
	{
		entt::registry& registry = scene->GetRegistry();

		auto sphere = CommonUtils::BuildPyramidObject(
				registry,
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

		const Maths::Vector3 forward = -scene->GetCamera()->GetForwardDirection();

		registry.get<Physics3DComponent>(sphere).GetPhysicsObject()->SetLinearVelocity(forward * 30.0f);
	}
}
