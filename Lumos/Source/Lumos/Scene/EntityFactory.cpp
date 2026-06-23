#include "Precompiled.h"
#include "EntityFactory.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/TerrainCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Scene/Component/ModelComponent.h"
#include "Maths/Random.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Terrain.h"
#include "Graphics/Model.h"
#include "Graphics/Light.h"
#include "Scene/Component/Components.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Component/TerrainComponent.h"
#include "Maths/Transform.h"
#include "Scene/EntityManager.h"
#include "Core/OS/FileSystem.h"

namespace Lumos
{
    using namespace Maths;

    Vec4 EntityFactory::GenColour(float alpha)
    {
        Vec4 c;
        c.w = alpha;

        c.x = Random32::Rand(0.0f, 1.0f);
        c.y = Random32::Rand(0.0f, 1.0f);
        c.z = Random32::Rand(0.0f, 1.0f);

        return c;
    }

    Entity EntityFactory::BuildSphereObject(
        Scene* scene,
        const std::string& name,
        const Vec3& pos,
        float radius,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const Vec4& colour)
    {
        auto sphere     = scene->GetEntityManager()->Create(name);
        sphere.AddComponent<Maths::Transform>(Mat4::Translation(pos) * Mat4::Scale(Vec3(radius * 2.0f)));
        auto& modelComp = sphere.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere);
        auto& model     = modelComp.ModelRef;

        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = colour;
        // Deterministic matte material — random metallic turned spheres into
        // sky mirrors that bloomed to pure white at distance.
        properties.roughness          = 0.85f;
        properties.metallic           = 0.0f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        matInstance->SetMaterialProperites(properties);

        modelComp.InstanceMaterials[0] = matInstance;

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            // SharedPtr<RigidBody3D> testPhysics = CreateSharedPtr<RigidBody3D>();
            RigidBody3D* testPhysics = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});

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
        const Vec3& pos,
        const Vec3& halfdims,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const Vec4& colour)
    {
        auto cube       = scene->GetEntityManager()->Create(name);
        cube.AddComponent<Maths::Transform>(Mat4::Translation(pos) * Mat4::Scale(halfdims * 2.0f));
        auto& modelComp = cube.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);
        auto& model     = modelComp.ModelRef;

        auto matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = colour;
        // Deterministic matte, no emissive — glow is opt-in via SetEntityPulse.
        properties.roughness          = 0.85f;
        properties.metallic           = 0.0f;
        properties.emissive           = 0.0f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        properties.emissiveMapFactor  = 0.0f;
        properties.occlusionMapFactor = 0.0f;
        matInstance->SetMaterialProperites(properties);

        modelComp.InstanceMaterials[0] = matInstance;

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            // SharedPtr<RigidBody3D> testPhysics = CreateSharedPtr<RigidBody3D>();
            RigidBody3D* testPhysics = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
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
        const Vec3& pos,
        const Vec3& halfdims,
        bool physics_enabled,
        float inverse_mass,
        bool collidable,
        const Vec4& colour)
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

        // auto shader = Application::Get().GetShaderLibrary()->GetAsset("//CoreShaders/ForwardPBR.shader");
        // matInstance->SetShader(shader);

        pyramidMeshEntity.AddComponent<Maths::Transform>(Quat(-90.0f, 0.0f, 0.0f).Normalised().ToMatrix4() * Mat4::Scale(halfdims));
        pyramidMeshEntity.SetParent(pyramid);
        auto& pyramidModelComp                  = pyramidMeshEntity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Pyramid);
        pyramidModelComp.InstanceMaterials[0]   = matInstance;

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            // SharedPtr<RigidBody3D> testPhysics = CreateSharedPtr<RigidBody3D>();
            RigidBody3D* testPhysics = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
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

    void EntityFactory::AddLightCube(Scene* scene, const Vec3& pos, const Vec3& dir)
    {
        Vec4 colour = Vec4(Random32::Rand(0.0f, 1.0f),
                           Random32::Rand(0.0f, 1.0f),
                           Random32::Rand(0.0f, 1.0f),
                           1.0f);

        entt::registry& registry = scene->GetRegistry();

        auto cube = EntityFactory::BuildCuboidObject(
            scene,
            "light Cube",
            pos,
            Vec3(0.5f, 0.5f, 0.5f),
            true,
            1.0f,
            true,
            colour);

        // cube.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetIsAtRest(true);
        const float radius    = Random32::Rand(1.0f, 30.0f);
        const float intensity = Random32::Rand(0.0f, 2.0f) * 120000.0f;

        cube.AddComponent<Graphics::Light>(pos, colour, intensity, Graphics::LightType::PointLight, pos, radius);
        const Vec3 forward = dir;
        cube.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }

    void EntityFactory::AddSphere(Scene* scene, const Vec3& pos, const Vec3& dir)
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
            Vec4(Random32::Rand(0.0f, 1.0f),
                 Random32::Rand(0.0f, 1.0f),
                 Random32::Rand(0.0f, 1.0f),
                 1.0f));

        const Vec3 forward = dir;
        sphere.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 20.0f);
    }

    void EntityFactory::AddPyramid(Scene* scene, const Vec3& pos, const Vec3& dir)
    {
        entt::registry& registry = scene->GetRegistry();

        auto sphere = EntityFactory::BuildPyramidObject(
            scene,
            "Pyramid",
            pos,
            Vec3(0.5f),
            true,
            1.0f,
            true,
            Vec4(Random32::Rand(0.0f, 1.0f),
                 Random32::Rand(0.0f, 1.0f),
                 Random32::Rand(0.0f, 1.0f),
                 1.0f));

        const Vec3 forward = dir;

        sphere.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }

    Entity EntityFactory::AddTerrainEx(Scene* scene, const Vec3& pos, int gridSize, float scaleXZ, float heightScale, const TerrainMaterial& mat)
    {
        auto entity = scene->GetEntityManager()->Create("Terrain");
        entity.AddComponent<Maths::Transform>(Mat4::Translation(pos));

        // Bake uvTile into the mesh's per-vertex UV scale so we don't need a
        // per-material tiling uniform — keeps the shader path identical to
        // every other PBR object.
        const float uvScale = (1.0f / 16.0f) * std::max(mat.uvTile, 0.0001f);
        // Convert world-space tile position into grid-unit noise offsets so
        // adjacent tiles share edge values (no seam discontinuity).
        const int tileOriginX = (int)std::floor(pos.x / std::max(scaleXZ, 0.0001f));
        const int tileOriginZ = (int)std::floor(pos.z / std::max(scaleXZ, 0.0001f));
        SharedPtr<Lumos::Terrain> terrainMesh = CreateSharedPtr<Lumos::Terrain>(
            gridSize, gridSize,
            50, 10,
            scaleXZ, heightScale, scaleXZ,
            uvScale, uvScale,
            tileOriginX, tileOriginZ);

        SharedPtr<Graphics::Model> model = CreateSharedPtr<Graphics::Model>(
            SharedPtr<Graphics::Mesh>(terrainMesh), Graphics::PrimitiveType::Terrain);

        auto& modelComp = entity.AddComponent<Graphics::ModelComponent>(model);

        // Attach a TerrainComponent capturing the procedural heights so the editor
        // can sculpt them and the scene can serialize the result. Marked clean (no
        // custom edits yet) so back-compat scenes still regenerate procedurally.
        {
            auto& tc = entity.AddComponent<TerrainComponent>();
            tc.GridW       = gridSize;
            tc.GridH       = gridSize;
            tc.ScaleXZ     = scaleXZ;
            tc.ScaleY      = heightScale;
            tc.TileOriginX = tileOriginX;
            tc.TileOriginZ = tileOriginZ;
            const TDArray<float>& heights = terrainMesh->GetHeightData();
            tc.Heights.Resize(heights.Size());
            for(uint32_t i = 0; i < (uint32_t)heights.Size(); i++)
                tc.Heights[i] = heights[i];
            tc.InitSplatWeights();
            tc.HasCustomEdits = false;
        }

        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = mat.albedoColour;
        properties.roughness          = mat.roughness;
        properties.metallic           = mat.metallic;
        // Texture factors auto-flip to 1.0 when the corresponding texture loads
        // below. Default to 0 (flat) so the material reads correctly when no
        // textures are supplied.
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        properties.emissiveMapFactor  = 0.0f;
        properties.occlusionMapFactor = 0.0f;
        matInstance->SetMaterialProperites(properties);

        // Material::SetAlbedoTexture constructs a VKTexture2D unconditionally
        // even when the file fails to load — that leaves a non-null SharedPtr
        // wrapping an object with VK_NULL_HANDLE image, which crashes MoltenVK
        // when bound. Gate on a VFS existence check so missing PNGs are
        // skipped cleanly and fall back to the flat albedo colour.
        auto FileOnDisk = [](const std::string& vfsPath) -> bool
        {
            if(vfsPath.empty()) return false;
            return FileSystem::Get().FileExistsVFS(Str8StdS(vfsPath));
        };

        if(FileOnDisk(mat.albedoPath))
        {
            matInstance->SetAlbedoTexture(mat.albedoPath);
            if(matInstance->GetTextures().albedo)
                matInstance->GetProperties()->albedoMapFactor = 1.0f;
        }
        if(FileOnDisk(mat.normalPath))
        {
            matInstance->SetNormalTexture(mat.normalPath);
            if(matInstance->GetTextures().normal)
                matInstance->GetProperties()->normalMapFactor = 1.0f;
        }
        if(FileOnDisk(mat.roughnessPath))
        {
            matInstance->SetRoughnessTexture(mat.roughnessPath);
            if(matInstance->GetTextures().roughness)
                matInstance->GetProperties()->roughnessMapFactor = 1.0f;
        }
        matInstance->UpdateMaterialPropertiesData();
        modelComp.InstanceMaterials[0] = matInstance;

        if(!mat.noCollider)
        {
            RigidBody3D* body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
            body->SetPosition(pos);
            body->SetIsStatic(true);
            body->SetInverseMass(0.0f);
            body->SetFriction(0.9f);

            const TDArray<float>& heights = terrainMesh->GetHeightData();
            SharedPtr<TerrainCollisionShape> shape = CreateSharedPtr<TerrainCollisionShape>(
                (uint32_t)gridSize, (uint32_t)gridSize,
                const_cast<float*>(heights.Data()),
                scaleXZ, heightScale);

            body->SetCollisionShape(shape);
            body->SetInverseInertia(shape->BuildInverseInertia(0.0f));

            entity.AddComponent<RigidBody3DComponent>(body);
        }
        return entity;
    }

    Entity EntityFactory::AddTerrain(Scene* scene, const Vec3& pos, int gridSize, float scaleXZ, float heightScale)
    {
        auto entity = scene->GetEntityManager()->Create("Terrain");
        entity.AddComponent<Maths::Transform>(Mat4::Translation(pos));

        const int tileOriginX = (int)std::floor(pos.x / std::max(scaleXZ, 0.0001f));
        const int tileOriginZ = (int)std::floor(pos.z / std::max(scaleXZ, 0.0001f));
        SharedPtr<Lumos::Terrain> terrainMesh = CreateSharedPtr<Lumos::Terrain>(
            gridSize, gridSize,
            50, 10,
            scaleXZ, heightScale, scaleXZ,
            1.0f / 16.0f, 1.0f / 16.0f,
            tileOriginX, tileOriginZ);

        SharedPtr<Graphics::Model> model = CreateSharedPtr<Graphics::Model>(
            SharedPtr<Graphics::Mesh>(terrainMesh), Graphics::PrimitiveType::Terrain);

        auto& modelComp = entity.AddComponent<Graphics::ModelComponent>(model);

        {
            auto& tc = entity.AddComponent<TerrainComponent>();
            tc.GridW       = gridSize;
            tc.GridH       = gridSize;
            tc.ScaleXZ     = scaleXZ;
            tc.ScaleY      = heightScale;
            tc.TileOriginX = tileOriginX;
            tc.TileOriginZ = tileOriginZ;
            const TDArray<float>& heights = terrainMesh->GetHeightData();
            tc.Heights.Resize(heights.Size());
            for(uint32_t i = 0; i < (uint32_t)heights.Size(); i++)
                tc.Heights[i] = heights[i];
            tc.InitSplatWeights();
            tc.HasCustomEdits = false;
        }

        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = Vec4(0.35f, 0.55f, 0.25f, 1.0f);
        properties.roughness          = 0.95f;
        properties.metallic           = 0.0f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        matInstance->SetMaterialProperites(properties);
        modelComp.InstanceMaterials[0] = matInstance;

        // Static rigid body with TerrainCollisionShape sharing the mesh's heightmap
        RigidBody3D* body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
        body->SetPosition(pos);
        body->SetIsStatic(true);
        body->SetInverseMass(0.0f);
        body->SetFriction(0.9f);

        const TDArray<float>& heights = terrainMesh->GetHeightData();
        SharedPtr<TerrainCollisionShape> shape = CreateSharedPtr<TerrainCollisionShape>(
            (uint32_t)gridSize, (uint32_t)gridSize,
            const_cast<float*>(heights.Data()),
            scaleXZ, heightScale);

        body->SetCollisionShape(shape);
        body->SetInverseInertia(shape->BuildInverseInertia(0.0f));

        entity.AddComponent<RigidBody3DComponent>(body);
        return entity;
    }

    void EntityFactory::BuildTerrainOnEntity(Entity entity, int gridSize, float scaleXZ, float heightScale, const TerrainMaterial& mat)
    {
        if(!entity.Valid() || gridSize <= 1 || scaleXZ <= 0.0f)
            return;

        // Position drives noise tile origin so a terrain placed at world X derives
        // a matching noise offset. Use the existing transform if present, else (0,0,0).
        Vec3 pos(0.0f);
        if(auto t = entity.TryGetComponent<Maths::Transform>())
            pos = t->GetWorldPosition();
        else
            entity.AddComponent<Maths::Transform>(Mat4::Translation(pos));

        const int tileOriginX = (int)std::floor(pos.x / std::max(scaleXZ, 0.0001f));
        const int tileOriginZ = (int)std::floor(pos.z / std::max(scaleXZ, 0.0001f));
        const float uvScale   = (1.0f / 16.0f) * std::max(mat.uvTile, 0.0001f);

        SharedPtr<Lumos::Terrain> terrainMesh = CreateSharedPtr<Lumos::Terrain>(
            gridSize, gridSize, 50, 10,
            scaleXZ, heightScale, scaleXZ,
            uvScale, uvScale,
            tileOriginX, tileOriginZ);
        SharedPtr<Graphics::Model> model = CreateSharedPtr<Graphics::Model>(
            SharedPtr<Graphics::Mesh>(terrainMesh), Graphics::PrimitiveType::Terrain);

        // Replace any prior ModelComponent on this entity — the user explicitly
        // asked for a terrain, so a non-terrain model would be stale anyway.
        entity.TryRemoveComponent<Graphics::ModelComponent>();
        auto& modelComp = entity.AddComponent<Graphics::ModelComponent>(model);

        // Material.
        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = mat.albedoColour;
        properties.roughness          = mat.roughness;
        properties.metallic           = mat.metallic;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        properties.emissiveMapFactor  = 0.0f;
        properties.occlusionMapFactor = 0.0f;
        matInstance->SetMaterialProperites(properties);
        matInstance->UpdateMaterialPropertiesData();
        modelComp.InstanceMaterials[0] = matInstance;

        // TerrainComponent capturing the freshly generated heights.
        entity.TryRemoveComponent<TerrainComponent>();
        auto& tc = entity.AddComponent<TerrainComponent>();
        tc.GridW       = gridSize;
        tc.GridH       = gridSize;
        tc.ScaleXZ     = scaleXZ;
        tc.ScaleY      = heightScale;
        tc.TileOriginX = tileOriginX;
        tc.TileOriginZ = tileOriginZ;
        const TDArray<float>& heights = terrainMesh->GetHeightData();
        tc.Heights.Resize(heights.Size());
        for(uint32_t i = 0; i < (uint32_t)heights.Size(); i++)
            tc.Heights[i] = heights[i];
        tc.InitSplatWeights();
        tc.HasCustomEdits = false;

        // Optional static collision body.
        entity.TryRemoveComponent<RigidBody3DComponent>();
        if(!mat.noCollider)
        {
            RigidBody3D* body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
            body->SetPosition(pos);
            body->SetIsStatic(true);
            body->SetInverseMass(0.0f);
            body->SetFriction(0.9f);

            SharedPtr<TerrainCollisionShape> shape = CreateSharedPtr<TerrainCollisionShape>(
                (uint32_t)gridSize, (uint32_t)gridSize,
                const_cast<float*>(tc.Heights.Data()),
                scaleXZ, heightScale);
            body->SetCollisionShape(shape);
            body->SetInverseInertia(shape->BuildInverseInertia(0.0f));
            entity.AddComponent<RigidBody3DComponent>(body);
        }
    }

    Entity EntityFactory::AddArrow(Scene* scene, const Vec3& pos, const Vec3& velocity, float radius, float length, float mass)
    {
        auto entity = scene->GetEntityManager()->Create("Arrow");
        entity.AddComponent<Maths::Transform>(Mat4::Translation(pos) * Mat4::Scale(Vec3(radius * 2.0f, length, radius * 2.0f)));

        auto& modelComp = entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Capsule);
        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = Vec4(0.85f, 0.7f, 0.4f, 1.0f);
        properties.roughness          = 0.5f;
        properties.metallic           = 0.2f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        matInstance->SetMaterialProperites(properties);
        modelComp.InstanceMaterials[0] = matInstance;

        float invMass = mass > 0.0f ? 1.0f / mass : 0.0f;
        RigidBody3D* body = Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({});
        body->SetPosition(pos);
        body->SetInverseMass(invMass);
        body->SetCollisionShape(CreateSharedPtr<CapsuleCollisionShape>(radius, length));
        body->SetInverseInertia(body->GetCollisionShape()->BuildInverseInertia(invMass));
        body->SetLinearVelocity(velocity);
        body->WakeUp();
        entity.AddComponent<RigidBody3DComponent>(body);
        return entity;
    }

    Entity EntityFactory::AddTarget(Scene* scene, const Vec3& pos, float radius, const Vec4& colour)
    {
        auto entity = scene->GetEntityManager()->Create("Target");
        entity.AddComponent<Maths::Transform>(Mat4::Translation(pos) * Mat4::Scale(Vec3(radius * 2.0f)));

        auto& modelComp = entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere);
        SharedPtr<Graphics::Material> matInstance = CreateSharedPtr<Graphics::Material>();
        Graphics::MaterialProperties properties;
        properties.albedoColour       = colour;
        properties.roughness          = 0.35f;
        properties.metallic           = 0.6f;
        properties.emissive           = 1.5f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        properties.emissiveMapFactor  = 0.0f;
        properties.occlusionMapFactor = 0.0f;
        matInstance->SetMaterialProperites(properties);
        modelComp.InstanceMaterials[0] = matInstance;

        return entity;
    }

    void EntityFactory::AddPlatform(Scene* scene, const Vec3& pos, const Vec3& scale)
    {
        auto platform = EntityFactory::BuildCuboidObject(
            scene,
            "Platform",
            pos,
            scale,
            true,
            0.0f, // inverse mass = 0 means infinite mass (static)
            true,
            Vec4(0.6f, 0.6f, 0.6f, 1.0f));

        platform.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetIsStatic(true);
        platform.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetFriction(0.8f);
    }
}
