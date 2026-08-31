#pragma once

#include "Maths/Vector3.h"
#include "Maths/Vector4.h"

namespace Lumos
{
    class RigidBody;
    class Scene;
    class Entity;

    namespace EntityFactory
    {
        Vec4 GenColour(float alpha);

        // Generates a default Sphere object with the parameters specified.
        Entity BuildSphereObject(
            Scene* scene,
            const std::string& name,
            const Vec3& pos,
            float radius,
            bool physics_enabled = false,
            float inverse_mass   = 0.0f, // requires physics_enabled = true
            bool collidable      = true, // requires physics_enabled = true
            const Vec4& colour   = Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Generates a default Cuboid object with the parameters specified
        Entity BuildCuboidObject(
            Scene* scene,
            const std::string& name,
            const Vec3& pos,
            const Vec3& scale,
            bool physics_enabled = false,
            float inverse_mass   = 0.0f, // requires physics_enabled = true
            bool collidable      = true, // requires physics_enabled = true
            const Vec4& colour   = Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Generates a default Cuboid object with the parameters specified
        Entity BuildPyramidObject(
            Scene* scene,
            const std::string& name,
            const Vec3& pos,
            const Vec3& scale,
            bool physics_enabled = false,
            float inverse_mass   = 0.0f, // requires physics_enabled = true
            bool collidable      = true, // requires physics_enabled = true
            const Vec4& colour   = Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        void AddLightCube(Scene* scene, const Vec3& pos, const Vec3& dir);
        void AddSphere(Scene* scene, const Vec3& pos, const Vec3& dir);
        void AddPyramid(Scene* scene, const Vec3& pos, const Vec3& dir);
        void AddPlatform(Scene* scene, const Vec3& pos, const Vec3& scale);

        Entity AddTerrain(Scene* scene, const Vec3& pos, int gridSize = 256, float scaleXZ = 1.0f, float heightScale = 60.0f);

        struct TerrainMaterial
        {
            Vec4 albedoColour = Vec4(0.35f, 0.55f, 0.25f, 1.0f);
            float roughness   = 0.95f;
            float metallic    = 0.0f;
            float uvTile      = 1.0f;
            std::string albedoPath;
            std::string normalPath;
            std::string roughnessPath;
            bool noCollider   = false;
        };

        Entity AddTerrainEx(Scene* scene, const Vec3& pos, int gridSize, float scaleXZ, float heightScale, const TerrainMaterial& mat);

        void BuildTerrainOnEntity(Entity entity, int gridSize, float scaleXZ, float heightScale, const TerrainMaterial& mat = {});

        Entity AddArrow(Scene* scene, const Vec3& pos, const Vec3& velocity, float radius = 0.08f, float length = 0.9f, float mass = 0.2f);

        Entity AddTarget(Scene* scene, const Vec3& pos, float radius = 4.0f, const Vec4& colour = Vec4(1.0f, 0.85f, 0.15f, 1.0f));
    };
}
