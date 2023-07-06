#include "Precompiled.h"
#include "EntityFactory.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Scene/Component/ModelComponent.h"
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

    glm::vec4 EntityFactory::GenColour(float alpha)
    {
        glm::vec4 c;
        c.w = alpha;

        c.x = Random32::Rand(0.0f, 1.0f);
        c.y = Random32::Rand(0.0f, 1.0f);
        c.z = Random32::Rand(0.0f, 1.0f);

        return c;
    }

    Entity EntityFactory::BuildSphereObject(
        Scene* scene,
        const std::string& name,
        const glm::vec3& pos,
        float radius,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const glm::vec4& colour)
    {
        auto sphere = scene->GetEntityManager()->Create(name);
        sphere.AddComponent<Maths::Transform>(glm::translate(glm::mat4(1.0), pos) * glm::scale(glm::mat4(1.0), glm::vec3(radius * 2.0f)));
        auto& model = sphere.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere).ModelRef;

        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = colour;
        properties.roughness          = Random32::Rand(0.0f, 1.0f);
        properties.metallic           = Random32::Rand(0.0f, 1.0f);
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        matInstance->SetMaterialProperites(properties);

        // auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        // matInstance->SetShader(nullptr);//shader);
        model->GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            SharedPtr<RigidBody3D> testPhysics = CreateSharedPtr<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                // Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(SphereCollisionShape(radius).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(CreateSharedPtr<SphereCollisionShape>(radius));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            sphere.AddComponent<RigidBody3DComponent>(testPhysics);
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
        const glm::vec3& pos,
        const glm::vec3& halfdims,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const glm::vec4& colour)
    {
        auto cube = scene->GetEntityManager()->Create(name);
        cube.AddComponent<Maths::Transform>(glm::translate(glm::mat4(1.0), pos) * glm::scale(glm::mat4(1.0), halfdims * 2.0f));
        auto& model = cube.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube).ModelRef;

        auto matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = colour;
        properties.roughness          = Random32::Rand(0.0f, 1.0f);
        properties.metallic           = Random32::Rand(0.0f, 1.0f);
        properties.emissive           = 3.0f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        properties.emissiveMapFactor  = 0.0f;
        properties.occlusionMapFactor = 0.0f;
        matInstance->SetMaterialProperites(properties);

        // auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        // matInstance->SetShader(shader);
        model->GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            SharedPtr<RigidBody3D> testPhysics = CreateSharedPtr<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                // Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(CuboidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(CreateSharedPtr<CuboidCollisionShape>(halfdims));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            cube.AddComponent<RigidBody3DComponent>(testPhysics);
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
        const glm::vec3& pos,
        const glm::vec3& halfdims,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const glm::vec4& colour)
    {
        auto pyramid           = scene->GetEntityManager()->Create(name);
        auto pyramidMeshEntity = scene->GetEntityManager()->Create();

        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = colour;
        properties.roughness          = Random32::Rand(0.0f, 1.0f);
        properties.metallic           = Random32::Rand(0.0f, 1.0f);
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        matInstance->SetMaterialProperites(properties);

        // auto shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        // matInstance->SetShader(shader);

        pyramidMeshEntity.AddComponent<Maths::Transform>(glm::toMat4(glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f))) * glm::scale(glm::mat4(1.0), halfdims));
        pyramidMeshEntity.SetParent(pyramid);
        auto& model = pyramidMeshEntity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Pyramid).ModelRef;
        model->GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            SharedPtr<RigidBody3D> testPhysics = CreateSharedPtr<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                // Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(PyramidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(CreateSharedPtr<PyramidCollisionShape>(halfdims));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            pyramid.AddComponent<RigidBody3DComponent>(testPhysics);
            pyramid.GetOrAddComponent<Maths::Transform>().SetLocalPosition(pos);
        }
        else
        {
            pyramid.GetTransform().SetLocalPosition(pos);
        }

        return pyramid;
    }

    void EntityFactory::AddLightCube(Scene* scene, const glm::vec3& pos, const glm::vec3& dir)
    {
        glm::vec4 colour = glm::vec4(Random32::Rand(0.0f, 1.0f),
                                     Random32::Rand(0.0f, 1.0f),
                                     Random32::Rand(0.0f, 1.0f),
                                     1.0f);

        entt::registry& registry = scene->GetRegistry();

        auto cube = EntityFactory::BuildCuboidObject(
            scene,
            "light Cube",
            pos,
            glm::vec3(0.5f, 0.5f, 0.5f),
            true,
            1.0f,
            true,
            colour);

        // cube.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetIsAtRest(true);
        const float radius    = Random32::Rand(1.0f, 30.0f);
        const float intensity = Random32::Rand(0.0f, 2.0f) * 120000.0f;

        cube.AddComponent<Graphics::Light>(pos, colour, intensity, Graphics::LightType::PointLight, pos, radius);
        const glm::vec3 forward = dir;
        cube.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }

    void EntityFactory::AddSphere(Scene* scene, const glm::vec3& pos, const glm::vec3& dir)
    {
        entt::registry& registry = scene->GetRegistry();

        auto sphere = EntityFactory::BuildSphereObject(
            scene,
            "Sphere",
            pos,
            Random32::Rand(0.8f, 1.7f),
            true,
            Random32::Rand(0.2f, 1.0f),
            true,
            glm::vec4(Random32::Rand(0.0f, 1.0f),
                      Random32::Rand(0.0f, 1.0f),
                      Random32::Rand(0.0f, 1.0f),
                      1.0f));

        const glm::vec3 forward = dir;
        sphere.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 20.0f);
    }

    void EntityFactory::AddPyramid(Scene* scene, const glm::vec3& pos, const glm::vec3& dir)
    {
        entt::registry& registry = scene->GetRegistry();

        auto sphere = EntityFactory::BuildPyramidObject(
            scene,
            "Pyramid",
            pos,
            glm::vec3(0.5f),
            true,
            1.0f,
            true,
            glm::vec4(Random32::Rand(0.0f, 1.0f),
                      Random32::Rand(0.0f, 1.0f),
                      Random32::Rand(0.0f, 1.0f),
                      1.0f));

        const glm::vec3 forward = dir;

        sphere.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }
}
