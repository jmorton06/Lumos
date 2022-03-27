#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Maths/BoundingBox.h"
#include "Maths/BoundingSphere.h"
#include "Maths/Frustum.h"
#include "Maths/Plane.h"
#include "Maths/MathsUtilities.h"
#include "Core/LMLog.h"
#include "Core/Core.h"

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

        inline glm::vec2 WorldToScreen(const glm::vec3& worldPos, const glm::mat4& mvp, float width, float height, float winPosX = 0.0f, float winPosY = 0.0f)
        {
            glm::vec4 trans = mvp * glm::vec4(worldPos, 1.0f);
            trans *= 0.5f / trans.w;
            trans += glm::vec4(0.5f, 0.5f, 0.0f, 0.0f);
            trans.y = 1.f - trans.y;
            trans.x *= width;
            trans.y *= height;
            trans.x += winPosX;
            trans.y += winPosY;
            return glm::vec2(trans.x, trans.y);
        }

        void SetScale(glm::mat4& transform, float scale);

        void SetScale(glm::mat4& transform, const glm::vec3& scale);

        void SetRotation(glm::mat4& transform, const glm::vec3& rotation);

        void SetTranslation(glm::mat4& transform, const glm::vec3& translation);

        glm::vec3 GetScale(const glm::mat4& transform);
        glm::vec3 GetRotation(const glm::mat4& transform);

        glm::mat4 Mat4FromTRS(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale);
    }
}

namespace glm
{

    template <class Archive>
    void serialize(Archive& archive, glm::vec2& v) { archive(v.x, v.y); }
    template <class Archive>
    void serialize(Archive& archive, glm::vec3& v) { archive(v.x, v.y, v.z); }
    template <class Archive>
    void serialize(Archive& archive, glm::vec4& v) { archive(v.x, v.y, v.z, v.w); }
    template <class Archive>
    void serialize(Archive& archive, glm::ivec2& v) { archive(v.x, v.y); }
    template <class Archive>
    void serialize(Archive& archive, glm::ivec3& v) { archive(v.x, v.y, v.z); }
    template <class Archive>
    void serialize(Archive& archive, glm::ivec4& v) { archive(v.x, v.y, v.z, v.w); }
    template <class Archive>
    void serialize(Archive& archive, glm::uvec2& v) { archive(v.x, v.y); }
    template <class Archive>
    void serialize(Archive& archive, glm::uvec3& v) { archive(v.x, v.y, v.z); }
    template <class Archive>
    void serialize(Archive& archive, glm::uvec4& v) { archive(v.x, v.y, v.z, v.w); }
    template <class Archive>
    void serialize(Archive& archive, glm::dvec2& v) { archive(v.x, v.y); }
    template <class Archive>
    void serialize(Archive& archive, glm::dvec3& v) { archive(v.x, v.y, v.z); }
    template <class Archive>
    void serialize(Archive& archive, glm::dvec4& v) { archive(v.x, v.y, v.z, v.w); }

    // glm matrices serialization
    template <class Archive>
    void serialize(Archive& archive, glm::mat2& m) { archive(m[0], m[1]); }
    template <class Archive>
    void serialize(Archive& archive, glm::dmat2& m) { archive(m[0], m[1]); }
    template <class Archive>
    void serialize(Archive& archive, glm::mat3& m) { archive(m[0], m[1], m[2]); }
    template <class Archive>
    void serialize(Archive& archive, glm::mat4& m) { archive(m[0], m[1], m[2], m[3]); }
    template <class Archive>
    void serialize(Archive& archive, glm::dmat4& m) { archive(m[0], m[1], m[2], m[3]); }

    template <class Archive>
    void serialize(Archive& archive, glm::quat& q) { archive(q.x, q.y, q.z, q.w); }
    template <class Archive>
    void serialize(Archive& archive, glm::dquat& q) { archive(q.x, q.y, q.z, q.w); }

    glm::vec3 operator*(const glm::mat4& a, const glm::vec3& b);
}
