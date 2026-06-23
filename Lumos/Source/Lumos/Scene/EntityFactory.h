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

        // Procedural terrain: Perlin heightmap mesh + static TerrainCollisionShape rigid body.
        // gridSize controls vertex count along each axis. scaleXZ = world units per grid cell.
        // heightScale = peak height in world units.
        Entity AddTerrain(Scene* scene, const Vec3& pos, int gridSize = 256, float scaleXZ = 1.0f, float heightScale = 60.0f);

        // Terrain material descriptor. Empty texture paths fall back to
        // a flat albedo coloured material. uvTile multiplies the mesh's existing UVs
        // for normal/albedo tiling (mesh authored with 1/16 tiling already).
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

        // Same as AddTerrain but applies a caller-supplied PBR material
        // (albedo / normal / roughness textures + base colour and tiling).
        Entity AddTerrainEx(Scene* scene, const Vec3& pos, int gridSize, float scaleXZ, float heightScale, const TerrainMaterial& mat);

        // Build / rebuild a terrain on an already-existing entity. Used by the
        // inspector "Generate" flow when the user attaches a TerrainComponent
        // and configures dimensions in-place. Replaces any existing model /
        // terrain component / rigid body on the entity.
        void BuildTerrainOnEntity(Entity entity, int gridSize, float scaleXZ, float heightScale, const TerrainMaterial& mat = {});

        // Capsule-shaped dynamic body for projectiles (arrows, etc.). Named "Arrow" by default.
        // velocity is applied directly as initial linear velocity.
        Entity AddArrow(Scene* scene, const Vec3& pos, const Vec3& velocity, float radius = 0.08f, float length = 0.9f, float mass = 0.2f);

        // Static, brightly-coloured sphere target (no physics body — hit-tested in Lua).
        // Returned Entity can be destroyed between levels.
        Entity AddTarget(Scene* scene, const Vec3& pos, float radius = 4.0f, const Vec4& colour = Vec4(1.0f, 0.85f, 0.15f, 1.0f));
    };
}
