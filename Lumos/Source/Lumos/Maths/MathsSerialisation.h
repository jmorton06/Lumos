#pragma once
#include "Core/Core.h"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace glm
{
    template <class Archive>
    void serialize(Archive& archive, glm::vec2& v)
    {
        archive(v.x, v.y);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::vec3& v)
    {
        archive(v.x, v.y, v.z);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::vec4& v)
    {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::ivec2& v)
    {
        archive(v.x, v.y);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::ivec3& v)
    {
        archive(v.x, v.y, v.z);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::ivec4& v)
    {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::uvec2& v)
    {
        archive(v.x, v.y);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::uvec3& v)
    {
        archive(v.x, v.y, v.z);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::uvec4& v)
    {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::dvec2& v)
    {
        archive(v.x, v.y);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::dvec3& v)
    {
        archive(v.x, v.y, v.z);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::dvec4& v)
    {
        archive(v.x, v.y, v.z, v.w);
    }

    // glm matrices serialization
    template <class Archive>
    void serialize(Archive& archive, glm::mat2& m)
    {
        archive(m[0], m[1]);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::dmat2& m)
    {
        archive(m[0], m[1]);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::mat3& m)
    {
        archive(m[0], m[1], m[2]);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::mat4& m)
    {
        archive(m[0], m[1], m[2], m[3]);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::dmat4& m)
    {
        archive(m[0], m[1], m[2], m[3]);
    }

    template <class Archive>
    void serialize(Archive& archive, glm::quat& q)
    {
        archive(q.x, q.y, q.z, q.w);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::dquat& q)
    {
        archive(q.x, q.y, q.z, q.w);
    }

    glm::vec3 operator*(const glm::mat4& a, const glm::vec3& b);
}
