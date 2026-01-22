#include "Precompiled.h"
#include "MathsLua.h"
#include "Maths/Transform.h"
#include <sol/sol.hpp>
#include "Maths/Vector3.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{

    void BindMathsLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        auto Vec2type = state.new_usertype<Vec2>("Vec2", sol::constructors<Vec2(float, float)>());

        auto mult_overloads_vec2 = sol::overload(
            [](const Vec2& v1, const Vec2& v2) -> Vec2
            { return v1 * v2; },
            [](const Vec2& v1, float f) -> Vec2
            { return v1 * f; },
            [](float f, const Vec2& v1) -> Vec2
            { return f * v1; });

        Vec2type[sol::meta_function::multiplication] = mult_overloads_vec2;

        // Fields
        Vec2type["x"] = &Vec2::x;
        Vec2type["y"] = &Vec2::y;

        // Meta functions
        Vec2type[sol::meta_function::addition] = [](const Vec2& a, const Vec2& b)
        {
            return a + b;
        };

        Vec2type[sol::meta_function::subtraction] = [](const Vec2& a, const Vec2& b)
        {
            return a - b;
        };
        Vec2type[sol::meta_function::division] = [](const Vec2& a, const Vec2& b)
        {
            return a / b;
        };
        Vec2type[sol::meta_function::equal_to] = [](const Vec2& a, const Vec2& b)
        {
            return a == b;
        };

        // Methods
        Vec2type["Length"] = [](const Vec2& v)
        {
            return Maths::Length(v);
        };
        Vec2type["Distance"] = [](const Vec2& a, const Vec2& b)
        {
            return Maths::Distance(a, b);
        };
        Vec2type["Distance2"] = [](const Vec2& a, const Vec2& b)
        {
            return Maths::Distance2(a, b);
        };

        auto mult_overloads = sol::overload(
            [](const Vec3& v1, const Vec3& v2) -> Vec3
            { return v1 * v2; },
            [](const Vec3& v1, float f) -> Vec3
            { return v1 * f; },
            [](float f, const Vec3& v1) -> Vec3
            { return f * v1; });

        auto Vec3type = state.new_usertype<Vec3>("Vec3", sol::constructors<Vec3(), Vec3(float, float, float)>());

        // Fields
        Vec3type["x"] = &Vec3::x;
        Vec3type["y"] = &Vec3::y;
        Vec3type["z"] = &Vec3::z;

        // Meta functions
        Vec3type[sol::meta_function::addition] = [](const Vec3& a, const Vec3& b)
        {
            return a + b;
        };

        Vec3type[sol::meta_function::multiplication] = mult_overloads;

        Vec3type[sol::meta_function::subtraction] = [](const Vec3& a, const Vec3& b)
        {
            return a - b;
        };

        Vec3type[sol::meta_function::unary_minus] = [](Vec3& v) -> Vec3
        {
            return -v;
        };

        Vec3type[sol::meta_function::division] = [](const Vec3& a, const Vec3& b)
        {
            return a / b;
        };

        Vec3type[sol::meta_function::equal_to] = [](const Vec3& a, const Vec3& b)
        {
            return a == b;
        };

        // Methods
        Vec3type["Normalise"] = [](Vec3& v)
        {
            return v.Normalised();
        };

        Vec3type["Length"] = [](const Vec3& v)
        {
            return Maths::Length(v);
        };

        Vec3type["Distance"] = [](const Vec3& a, const Vec3& b)
        {
            return Maths::Distance(a, b);
        };

        Vec3type["Distance2"] = [](const Vec3& a, const Vec3& b)
        {
            return Maths::Distance2(a, b);
        };

        auto Vec4type = state.new_usertype<Vec4>("Vec4", sol::constructors<Vec4(), Vec4(float, float, float, float)>());

        // Fields
        Vec4type["x"] = &Vec4::x;
        Vec4type["y"] = &Vec4::y;
        Vec4type["z"] = &Vec4::z;
        Vec4type["w"] = &Vec4::w;

        // Meta functions
        Vec4type[sol::meta_function::addition] = [](const Vec4& a, const Vec4& b)
        {
            return a + b;
        };

        Vec4type[sol::meta_function::multiplication] = sol::overload(
            [](const Vec4& v1, const Vec4& v2) -> Vec4
            {
                return v1 * v2;
            },
            [](const Vec4& v1, float f) -> Vec4
            {
                return v1 * f;
            },
            [](float f, const Vec4& v1) -> Vec4
            {
                return f * v1;
            });

        Vec4type[sol::meta_function::subtraction] = [](const Vec4& a, const Vec4& b)
        {
            return a - b;
        };

        Vec4type[sol::meta_function::division] = [](const Vec4& a, const Vec4& b)
        {
            return a / b;
        };

        Vec4type[sol::meta_function::equal_to] = [](const Vec4& a, const Vec4& b)
        {
            return a == b;
        };

        // Methods
        Vec4type["Normalise"] = [](Vec4& v)
        {
            return v.Normalise();
        };

        Vec4type["Length"] = [](const Vec4& v)
        {
            return Maths::Length(v);
        };

        Vec4type["Distance"] = [](const Vec4& a, const Vec4& b)
        {
            return Maths::Distance(a, b);
        };

        Vec4type["Distance2"] = [](const Vec4& a, const Vec4& b)
        {
            return Maths::Distance2(a, b);
        };

        auto QuaternionType = state.new_usertype<Quat>("Quat", sol::constructors<Quat(float, float, float, float), Quat(Vec3), Quat(float, float, float)>());

        // Fields
        QuaternionType["x"] = &Quat::x;
        QuaternionType["y"] = &Quat::y;
        QuaternionType["z"] = &Quat::z;
        QuaternionType["w"] = &Quat::w;

        // Meta functions
        QuaternionType[sol::meta_function::addition] = [](const Quat& a, const Quat& b)
        {
            return a + b;
        };

        QuaternionType[sol::meta_function::multiplication] = [](const Quat& a, const Quat& b)
        {
            return a * b;
        };

        QuaternionType[sol::meta_function::subtraction] = [](const Quat& a, const Quat& b)
        {
            return a - b;
        };

        QuaternionType[sol::meta_function::equal_to] = [](const Quat& a, const Quat& b)
        {
            return a == b;
        };

        // Methods
        QuaternionType["Normalise"] = [](Quat& q)
        {
            return q.Normalised();
        };

        auto Matrix3Type = state.new_usertype<Mat3>("Matrix3", sol::constructors<Mat3(float, float, float, float, float, float, float, float, float), Mat3()>());

        // Meta functions
        Matrix3Type[sol::meta_function::multiplication] = [](const Mat3& a, const Mat3& b)
        {
            return a * b;
        };

        auto Matrix4Type = state.new_usertype<Mat4>("Matrix4");

        // Constructors
        Matrix4Type[sol::call_constructor] = sol::constructors<
            Mat4(float),
            Mat4()>();

        // Meta functions
        Matrix4Type[sol::meta_function::multiplication] = [](const Mat4& a, const Mat4& b)
        {
            return a * b;
        };

        auto TransformType = state.new_usertype<Maths::Transform>("Transform", sol::constructors<Maths::Transform(Mat4), Maths::Transform(), Maths::Transform(Vec3)>());

        // Fields
        TransformType["LocalScale"]       = &Maths::Transform::GetLocalScale;
        TransformType["LocalOrientation"] = &Maths::Transform::GetLocalOrientation;
        TransformType["LocalPosition"]    = &Maths::Transform::GetLocalPosition;

        // Methods
        TransformType["SetLocalTransform"]   = &Maths::Transform::SetLocalTransform;
        TransformType["SetLocalPosition"]    = &Maths::Transform::SetLocalPosition;
        TransformType["SetLocalScale"]       = &Maths::Transform::SetLocalScale;
        TransformType["SetLocalOrientation"] = &Maths::Transform::SetLocalOrientation;
        TransformType["GetWorldPosition"]    = &Maths::Transform::GetWorldPosition;
        TransformType["GetWorldOrientation"] = &Maths::Transform::GetWorldOrientation;
        TransformType["GetForwardDirection"] = &Maths::Transform::GetForwardDirection;
        TransformType["GetRightDirection"]   = &Maths::Transform::GetRightDirection;

        state["SineOut"]          = Maths::SineOut;
        state["SineIn"]           = Maths::SineIn;
        state["SineInOut"]        = Maths::SineInOut;
        state["ExponentialOut"]   = Maths::ExponentialOut;
        state["ExponentialIn"]    = Maths::ExponentialIn;
        state["ExponentialInOut"] = Maths::ExponentialInOut;
        state["ElasticIn"]        = Maths::ElasticIn;
        state["ElasticOut"]       = Maths::ElasticOut;
        state["ElasticInOut"]     = Maths::ElasticInOut;
        state["AnimateToTarget"]  = Maths::AnimateToTarget;
    }
}
