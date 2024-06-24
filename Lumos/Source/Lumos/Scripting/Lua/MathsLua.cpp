#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "MathsLua.h"
#include "Maths/Transform.h"
#include <sol/sol.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Lumos
{

    void BindMathsLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        auto Vector2type = state.new_usertype<glm::vec2>("Vector2", sol::constructors<glm::vec2(float, float)>());

        // Fields
        Vector2type["x"] = &glm::vec2::x;
        Vector2type["y"] = &glm::vec2::y;

        // Meta functions
        Vector2type[sol::meta_function::addition] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return a + b;
        };
        Vector2type[sol::meta_function::multiplication] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return a * b;
        };
        Vector2type[sol::meta_function::subtraction] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return a - b;
        };
        Vector2type[sol::meta_function::division] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return a / b;
        };
        Vector2type[sol::meta_function::equal_to] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return a == b;
        };

        // Methods
        Vector2type["Length"] = [](const glm::vec2& v)
        {
            return glm::length(v);
        };
        Vector2type["Distance"] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return glm::distance(a, b);
        };
        Vector2type["Distance2"] = [](const glm::vec2& a, const glm::vec2& b)
        {
            return glm::distance2(a, b);
        };

        auto mult_overloads = sol::overload(
            [](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3
            { return v1 * v2; },
            [](const glm::vec3& v1, float f) -> glm::vec3
            { return v1 * f; },
            [](float f, const glm::vec3& v1) -> glm::vec3
            { return f * v1; });

        auto Vector3type = state.new_usertype<glm::vec3>("Vector3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>());

        // Fields
        Vector3type["x"] = &glm::vec3::x;
        Vector3type["y"] = &glm::vec3::y;
        Vector3type["z"] = &glm::vec3::z;

        // Meta functions
        Vector3type[sol::meta_function::addition] = [](const glm::vec3& a, const glm::vec3& b)
        {
            return a + b;
        };

        Vector3type[sol::meta_function::multiplication] = mult_overloads;

        Vector3type[sol::meta_function::subtraction] = [](const glm::vec3& a, const glm::vec3& b)
        {
            return a - b;
        };

        Vector3type[sol::meta_function::unary_minus] = [](glm::vec3& v) -> glm::vec3
        {
            return -v;
        };

        Vector3type[sol::meta_function::division] = [](const glm::vec3& a, const glm::vec3& b)
        {
            return a / b;
        };

        Vector3type[sol::meta_function::equal_to] = [](const glm::vec3& a, const glm::vec3& b)
        {
            return a == b;
        };

        // Methods
        Vector3type["Normalise"] = [](glm::vec3& v)
        {
            return glm::normalize(v);
        };

        Vector3type["Length"] = [](const glm::vec3& v)
        {
            return glm::length(v);
        };

        Vector3type["Distance"] = [](const glm::vec3& a, const glm::vec3& b)
        {
            return glm::distance(a, b);
        };

        Vector3type["Distance2"] = [](const glm::vec3& a, const glm::vec3& b)
        {
            return glm::distance2(a, b);
        };

        auto Vector4type = state.new_usertype<glm::vec4>("Vector4", sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>());

        // Fields
        Vector4type["x"] = &glm::vec4::x;
        Vector4type["y"] = &glm::vec4::y;
        Vector4type["z"] = &glm::vec4::z;
        Vector4type["w"] = &glm::vec4::w;

        // Meta functions
        Vector4type[sol::meta_function::addition] = [](const glm::vec4& a, const glm::vec4& b)
        {
            return a + b;
        };

        Vector4type[sol::meta_function::multiplication] = sol::overload(
            [](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4
            {
                return v1 * v2;
            },
            [](const glm::vec4& v1, float f) -> glm::vec4
            {
                return v1 * f;
            },
            [](float f, const glm::vec4& v1) -> glm::vec4
            {
                return f * v1;
            });

        Vector4type[sol::meta_function::subtraction] = [](const glm::vec4& a, const glm::vec4& b)
        {
            return a - b;
        };

        Vector4type[sol::meta_function::division] = [](const glm::vec4& a, const glm::vec4& b)
        {
            return a / b;
        };

        Vector4type[sol::meta_function::equal_to] = [](const glm::vec4& a, const glm::vec4& b)
        {
            return a == b;
        };

        // Methods
        Vector4type["Normalise"] = [](glm::vec4& v)
        {
            return glm::normalize(v);
        };

        Vector4type["Length"] = [](const glm::vec4& v)
        {
            return glm::length(v);
        };

        Vector4type["Distance"] = [](const glm::vec4& a, const glm::vec4& b)
        {
            return glm::distance(a, b);
        };

        Vector4type["Distance2"] = [](const glm::vec4& a, const glm::vec4& b)
        {
            return glm::distance2(a, b);
        };

        auto QuaternionType = state.new_usertype<glm::quat>("Quaternion", sol::constructors<glm::quat(float, float, float, float), glm::quat(glm::vec3)>());

        // Fields
        QuaternionType["x"] = &glm::quat::x;
        QuaternionType["y"] = &glm::quat::y;
        QuaternionType["z"] = &glm::quat::z;
        QuaternionType["w"] = &glm::quat::w;

        // Meta functions
        QuaternionType[sol::meta_function::addition] = [](const glm::quat& a, const glm::quat& b)
        {
            return a + b;
        };

        QuaternionType[sol::meta_function::multiplication] = [](const glm::quat& a, const glm::quat& b)
        {
            return a * b;
        };

        QuaternionType[sol::meta_function::subtraction] = [](const glm::quat& a, const glm::quat& b)
        {
            return a - b;
        };

        QuaternionType[sol::meta_function::equal_to] = [](const glm::quat& a, const glm::quat& b)
        {
            return a == b;
        };

        // Methods
        QuaternionType["Normalise"] = [](glm::quat& q)
        {
            return glm::normalize(q);
        };

        auto Matrix3Type = state.new_usertype<glm::mat3>("Matrix3", sol::constructors<glm::mat3(float, float, float, float, float, float, float, float, float), glm::mat3()>());

        // Meta functions
        Matrix3Type[sol::meta_function::multiplication] = [](const glm::mat3& a, const glm::mat3& b)
        {
            return a * b;
        };

        auto Matrix4Type = state.new_usertype<glm::mat4>("Matrix4");

        // Constructors
        Matrix4Type[sol::call_constructor] = sol::constructors<
            glm::mat4(float),
            glm::mat4()>();

        // Meta functions
        Matrix4Type[sol::meta_function::multiplication] = [](const glm::mat4& a, const glm::mat4& b)
        {
            return a * b;
        };

        Matrix4Type[sol::meta_function::addition] = [](const glm::mat4& a, const glm::mat4& b)
        {
            return a + b;
        };

        Matrix4Type[sol::meta_function::subtraction] = [](const glm::mat4& a, const glm::mat4& b)
        {
            return a - b;
        };

        auto TransformType = state.new_usertype<Maths::Transform>("Transform", sol::constructors<Maths::Transform(glm::mat4), Maths::Transform(), Maths::Transform(glm::vec3)>());

        // Fields
        TransformType["LocalScale"]       = &Maths::Transform::GetLocalScale;
        TransformType["LocalOrientation"] = &Maths::Transform::GetLocalOrientation;
        TransformType["LocalPosition"]    = &Maths::Transform::GetLocalPosition;

        // Methods
        TransformType["ApplyTransform"]      = &Maths::Transform::ApplyTransform;
        TransformType["UpdateMatrices"]      = &Maths::Transform::UpdateMatrices;
        TransformType["SetLocalTransform"]   = &Maths::Transform::SetLocalTransform;
        TransformType["SetLocalPosition"]    = &Maths::Transform::SetLocalPosition;
        TransformType["SetLocalScale"]       = &Maths::Transform::SetLocalScale;
        TransformType["SetLocalOrientation"] = &Maths::Transform::SetLocalOrientation;
        TransformType["GetWorldPosition"]    = &Maths::Transform::GetWorldPosition;
        TransformType["GetWorldOrientation"] = &Maths::Transform::GetWorldOrientation;
        TransformType["GetForwardDirection"] = &Maths::Transform::GetForwardDirection;
        TransformType["GetRightDirection"]   = &Maths::Transform::GetRightDirection;
    }
}
