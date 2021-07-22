#include "Precompiled.h"
#include "EntityFactory.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/Model.h"
#include "Maths/Random.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Light.h"
#include "Scene/Component/Components.h"
#include "Maths/Transform.h"
#include "Scene/EntityManager.h"

namespace Lumos
{
    using namespace Maths;

    Maths::Vector4 EntityFactory::GenColour(float alpha)
    {
        Maths::Vector4 c;
        c.w = alpha;

        c.x = Random32::Rand(0.0f, 1.0f);
        c.y = Random32::Rand(0.0f, 1.0f);
        c.z = Random32::Rand(0.0f, 1.0f);

        return c;
    }

    Entity EntityFactory::BuildSphereObject(
        Scene* scene,
        const std::string& name,
        const Maths::Vector3& pos,
        float radius,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const Maths::Vector4& colour)
    {
        auto sphere = scene->GetEntityManager()->Create(name);
        sphere.AddComponent<Maths::Transform>(Maths::Matrix4::Translation(pos) * Maths::Matrix4::Scale(Maths::Vector3(radius, radius, radius)));
        auto& model = sphere.AddComponent<Graphics::Model>(SharedRef<Graphics::Mesh>(Graphics::CreateSphere()), Graphics::PrimitiveType::Sphere);

        SharedRef<Graphics::Material> matInstance = CreateSharedRef<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour = colour;
        properties.roughnessColour = Vector4(Random32::Rand(0.0f, 1.0f));
        properties.metallicColour = Vector4(Random32::Rand(0.0f, 1.0f));
        properties.usingAlbedoMap = 0.0f;
        properties.usingRoughnessMap = 0.0f;
        properties.usingNormalMap = 0.0f;
        properties.usingMetallicMap = 0.0f;
        matInstance->SetMaterialProperites(properties);

        auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        matInstance->SetShader(shader);
        model.GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            //Otherwise create a physics object, and set it's position etc
            SharedRef<RigidBody3D> testPhysics = CreateSharedRef<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                //Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(SphereCollisionShape(radius).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(CreateSharedRef<SphereCollisionShape>(radius));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            sphere.AddComponent<Physics3DComponent>(testPhysics);
        }
        else
        {
            sphere.GetTransform().SetLocalPosition(pos);
        }

        return sphere;
    }

    Entity EntityFactory::BuildCuboidObject(
        Scene* scene,
        const std::string& name,
        const Maths::Vector3& pos,
        const Maths::Vector3& halfdims,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const Maths::Vector4& colour)
    {
        auto cube = scene->GetEntityManager()->Create(name);
        cube.AddComponent<Maths::Transform>(Maths::Matrix4::Translation(pos) * Maths::Matrix4::Scale(halfdims));
        auto& model = cube.AddComponent<Graphics::Model>(SharedRef<Graphics::Mesh>(Graphics::CreateCube()), Graphics::PrimitiveType::Cube);

        auto matInstance = CreateSharedRef<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour = colour;
        properties.roughnessColour = Vector4(Random32::Rand(0.0f, 1.0f));
        properties.metallicColour = Vector4(Random32::Rand(0.0f, 1.0f));
        properties.emissiveColour = colour;
        properties.usingAlbedoMap = 0.0f;
        properties.usingRoughnessMap = 0.0f;
        properties.usingNormalMap = 0.0f;
        properties.usingMetallicMap = 0.0f;
        matInstance->SetMaterialProperites(properties);

        auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        matInstance->SetShader(shader);
        model.GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            //Otherwise create a physics object, and set it's position etc
            SharedRef<RigidBody3D> testPhysics = CreateSharedRef<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                //Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(CuboidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(CreateSharedRef<CuboidCollisionShape>(halfdims));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            cube.AddComponent<Physics3DComponent>(testPhysics);
        }
        else
        {
            cube.GetTransform().SetLocalPosition(pos);
        }

        return cube;
    }

    Entity EntityFactory::BuildPyramidObject(
        Scene* scene,
        const std::string& name,
        const Maths::Vector3& pos,
        const Maths::Vector3& halfdims,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const Maths::Vector4& colour)
    {
        auto pyramid = scene->GetEntityManager()->Create(name);
        auto pyramidMeshEntity = scene->GetEntityManager()->Create();

        SharedRef<Graphics::Material> matInstance = CreateSharedRef<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour = colour;
        properties.roughnessColour = Vector4(Random32::Rand(0.0f, 1.0f));
        properties.metallicColour = Vector4(Random32::Rand(0.0f, 1.0f));
        properties.usingAlbedoMap = 0.0f;
        properties.usingRoughnessMap = 0.0f;
        properties.usingNormalMap = 0.0f;
        properties.usingMetallicMap = 0.0f;
        matInstance->SetMaterialProperites(properties);

        auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        matInstance->SetShader(shader);

        pyramidMeshEntity.AddComponent<Maths::Transform>(Maths::Quaternion(-90.0f, 0.0f, 0.0f).RotationMatrix4() * Maths::Matrix4::Scale(halfdims));
        pyramidMeshEntity.SetParent(pyramid);
        auto& model = pyramidMeshEntity.AddComponent<Graphics::Model>(SharedRef<Graphics::Mesh>(Graphics::CreatePyramid()), Graphics::PrimitiveType::Pyramid);
        model.GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            //Otherwise create a physics object, and set it's position etc
            SharedRef<RigidBody3D> testPhysics = CreateSharedRef<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                //Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(PyramidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(CreateSharedRef<PyramidCollisionShape>(halfdims));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            pyramid.AddComponent<Physics3DComponent>(testPhysics);
            pyramid.GetOrAddComponent<Maths::Transform>().SetLocalPosition(pos);
        }
        else
        {
            pyramid.GetTransform().SetLocalPosition(pos);
        }

        return pyramid;
    }

    void EntityFactory::AddLightCube(Scene* scene, const Maths::Vector3& pos, const Maths::Vector3& dir)
    {
        Maths::Vector4 colour = Maths::Vector4(Random32::Rand(0.0f, 1.0f),
            Random32::Rand(0.0f, 1.0f),
            Random32::Rand(0.0f, 1.0f),
            1.0f);

        entt::registry& registry = scene->GetRegistry();

        auto cube = EntityFactory::BuildCuboidObject(
            scene,
            "light Cube",
            pos,
            Maths::Vector3(0.5f, 0.5f, 0.5f),
            true,
            1.0f,
            true,
            colour);

        cube.GetComponent<Physics3DComponent>().GetRigidBody()->SetIsAtRest(true);
        const float radius = Random32::Rand(1.0f, 30.0f);
        const float intensity = Random32::Rand(0.0f, 2.0f);

        cube.AddComponent<Graphics::Light>(pos, colour, intensity, Graphics::LightType::PointLight, pos, radius);
    }

    void EntityFactory::AddSphere(Scene* scene, const Maths::Vector3& pos, const Maths::Vector3& dir)
    {
        entt::registry& registry = scene->GetRegistry();

        auto sphere = EntityFactory::BuildSphereObject(
            scene,
            "Sphere",
            pos,
            0.5f,
            true,
            1.0f,
            true,
            Maths::Vector4(Random32::Rand(0.0f, 1.0f),
                Random32::Rand(0.0f, 1.0f),
                Random32::Rand(0.0f, 1.0f),
                1.0f));

        const Maths::Vector3 forward = dir;
        sphere.GetComponent<Physics3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }

    void EntityFactory::AddPyramid(Scene* scene, const Maths::Vector3& pos, const Maths::Vector3& dir)
    {
        entt::registry& registry = scene->GetRegistry();

        auto sphere = EntityFactory::BuildPyramidObject(
            scene,
            "Pyramid",
            pos,
            Maths::Vector3(0.5f),
            true,
            1.0f,
            true,
            Maths::Vector4(Random32::Rand(0.0f, 1.0f),
                Random32::Rand(0.0f, 1.0f),
                Random32::Rand(0.0f, 1.0f),
                1.0f));

        const Maths::Vector3 forward = dir;

        sphere.GetComponent<Physics3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }
}
