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
    };
}
