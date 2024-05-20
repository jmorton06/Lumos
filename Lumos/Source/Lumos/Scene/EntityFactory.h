#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace Lumos
{
    class RigidBody;
    class Scene;
    class Entity;

    namespace EntityFactory
    {
        glm::vec4 GenColour(float alpha);

        // Generates a default Sphere object with the parameters specified.
        Entity BuildSphereObject(
            Scene* scene,
            const std::string& name,
            const glm::vec3& pos,
            float radius,
            bool physics_enabled    = false,
            float inverse_mass      = 0.0f, // requires physics_enabled = true
            bool collidable         = true, // requires physics_enabled = true
            const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Generates a default Cuboid object with the parameters specified
        Entity BuildCuboidObject(
            Scene* scene,
            const std::string& name,
            const glm::vec3& pos,
            const glm::vec3& scale,
            bool physics_enabled    = false,
            float inverse_mass      = 0.0f, // requires physics_enabled = true
            bool collidable         = true, // requires physics_enabled = true
            const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Generates a default Cuboid object with the parameters specified
        Entity BuildPyramidObject(
            Scene* scene,
            const std::string& name,
            const glm::vec3& pos,
            const glm::vec3& scale,
            bool physics_enabled    = false,
            float inverse_mass      = 0.0f, // requires physics_enabled = true
            bool collidable         = true, // requires physics_enabled = true
            const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        void AddLightCube(Scene* scene, const glm::vec3& pos, const glm::vec3& dir);
        void AddSphere(Scene* scene, const glm::vec3& pos, const glm::vec3& dir);
        void AddPyramid(Scene* scene, const glm::vec3& pos, const glm::vec3& dir);
    };
}
