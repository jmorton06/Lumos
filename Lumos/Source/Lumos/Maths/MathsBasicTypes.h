#pragma once
#include "Core/Core.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Lumos
{
    namespace Maths
    {
        struct Vector2Simple
        {
            float x, y;
        };

        struct Vector3Simple
        {
            Vector3Simple(float x, float y, float z)
                : x(x)
                , y(y)
                , z(z) {};
            float x, y, z;
        };

        struct Vector4Simple
        {
            float x, y, z, w;
        };

        inline glm::vec2 ToVector(const Vector2Simple& vec)
        {
            return glm::vec2(vec.x, vec.y);
        }

        inline glm::vec3 ToVector(const Vector3Simple& vec)
        {
            return glm::vec3(vec.x, vec.y, vec.z);
        }

        inline glm::vec4 ToVector4(const Vector3Simple& vec)
        {
            return glm::vec4(vec.x, vec.y, vec.z, 1.0f);
        }

        inline Vector3Simple ToVector(const glm::vec3& vec)
        {
            return Vector3Simple(vec.x, vec.y, vec.z);
        }

        inline glm::vec4 ToVector(const Vector4Simple& vec)
        {
            return glm::vec4(vec.x, vec.y, vec.z, vec.w);
        }
    }
}