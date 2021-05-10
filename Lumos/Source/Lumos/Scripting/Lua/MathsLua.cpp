#include "Precompiled.h"
#include "MathsLua.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"
#include <sol/sol.hpp>

namespace Lumos
{

    void BindMathsLua(sol::state& state)
    {
        LUMOS_PROFILE_FUNCTION();
        state.new_usertype<Maths::Vector2>("Vector2",
            sol::constructors<Maths::Vector2(float, float)>(),
            "x", &Maths::Vector2::x,
            "y", &Maths::Vector2::y,
            sol::meta_function::addition, sol::overload(static_cast<Maths::Vector2 (Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator+)),
            sol::meta_function::multiplication, sol::overload(static_cast<Maths::Vector2 (Maths::Vector2::*)(float) const>(&Maths::Vector2::operator*), static_cast<Maths::Vector2 (Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator*)),
            sol::meta_function::subtraction, sol::overload(static_cast<Maths::Vector2 (Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator-)),
            sol::meta_function::division, sol::overload(static_cast<Maths::Vector2 (Maths::Vector2::*)(float) const>(&Maths::Vector2::operator/), static_cast<Maths::Vector2 (Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator/)),
            sol::meta_function::equal_to, &Maths::Vector2::operator==);

        state.new_usertype<Maths::Vector3>(
            "Vector3",
            sol::constructors<sol::types<>, sol::types<float, float, float>>(),
            "x", &Maths::Vector3::x,
            "y", &Maths::Vector3::y,
            "z", &Maths::Vector3::z,
            "xy", &Maths::Vector3::xy,
            "Dot", static_cast<float (Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::DotProduct),
            "Cross", static_cast<Maths::Vector3 (Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::CrossProduct),

            sol::meta_function::addition, sol::overload(static_cast<Maths::Vector3 (Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator+)),
            sol::meta_function::multiplication, sol::overload(static_cast<Maths::Vector3 (Maths::Vector3::*)(float) const>(&Maths::Vector3::operator*), static_cast<Maths::Vector3 (Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator*)),
            sol::meta_function::subtraction, sol::overload(static_cast<Maths::Vector3 (Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator-)),
            sol::meta_function::unary_minus, [](Maths::Vector3& v) -> Maths::Vector3
            { return -v; },
            sol::meta_function::division, sol::overload(static_cast<Maths::Vector3 (Maths::Vector3::*)(float) const>(&Maths::Vector3::operator/), static_cast<Maths::Vector3 (Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator/)),
            sol::meta_function::equal_to, &Maths::Vector3::operator==

        );

        state.new_usertype<Maths::Vector4>("Vector4",
            sol::constructors<Maths::Vector4(), Maths::Vector4(float, float, float, float)>(),
            "x", &Maths::Vector4::x,
            "y", &Maths::Vector4::y,
            "z", &Maths::Vector4::z,
            "w", &Maths::Vector4::w,

            sol::meta_function::addition, sol::overload(static_cast<Maths::Vector4 (Maths::Vector4::*)(float) const>(&Maths::Vector4::operator+), static_cast<Maths::Vector4 (Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator+)),
            sol::meta_function::multiplication, sol::overload(static_cast<Maths::Vector4 (Maths::Vector4::*)(float) const>(&Maths::Vector4::operator*), static_cast<Maths::Vector4 (Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator*)),
            sol::meta_function::subtraction, sol::overload(static_cast<Maths::Vector4 (Maths::Vector4::*)(float) const>(&Maths::Vector4::operator-), static_cast<Maths::Vector4 (Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator-)),
            sol::meta_function::division, sol::overload(static_cast<Maths::Vector4 (Maths::Vector4::*)(float) const>(&Maths::Vector4::operator/), static_cast<Maths::Vector4 (Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator/)),
            sol::meta_function::equal_to, &Maths::Vector4::operator==);

        state.new_usertype<Maths::Quaternion>("Quaternion",
            sol::constructors<Maths::Quaternion(float, float, float), Maths::Quaternion(float, float, float, float), Maths::Quaternion(Maths::Vector3)>(),
            "x", &Maths::Quaternion::x,
            "y", &Maths::Quaternion::y,
            "z", &Maths::Quaternion::z,
            "w", &Maths::Quaternion::w,
            "EulerAngles", &Maths::Quaternion::EulerAngles,
            "Conjugate", &Maths::Quaternion::Conjugate,
            "Normalise", &Maths::Quaternion::Normalise,
            "Normal", &Maths::Quaternion::Normalised,
            sol::meta_function::addition, sol::overload(static_cast<Maths::Quaternion (Maths::Quaternion::*)(const Maths::Quaternion&) const>(&Maths::Quaternion::operator+)),
            sol::meta_function::multiplication, sol::overload(static_cast<Maths::Quaternion (Maths::Quaternion::*)(float) const>(&Maths::Quaternion::operator*), static_cast<Maths::Quaternion (Maths::Quaternion::*)(const Maths::Quaternion&) const>(&Maths::Quaternion::operator*)),
            sol::meta_function::subtraction, sol::overload(static_cast<Maths::Quaternion (Maths::Quaternion::*)(const Maths::Quaternion&) const>(&Maths::Quaternion::operator-)),
            sol::meta_function::equal_to, &Maths::Quaternion::operator==);

        state.new_usertype<Maths::Matrix3>("Matrix3",
            sol::constructors<Maths::Matrix3(float, float, float, float, float, float, float, float, float), Maths::Matrix3()>(),
            sol::meta_function::multiplication, sol::overload(static_cast<Maths::Vector3 (Maths::Matrix3::*)(const Maths::Vector3&) const>(&Maths::Matrix3::operator*), static_cast<Maths::Matrix3 (Maths::Matrix3::*)(const Maths::Matrix3&) const>(&Maths::Matrix3::operator*)));

        state.new_usertype<Maths::Matrix4>("Matrix4",
            sol::constructors<Maths::Matrix4(Maths::Matrix3), Maths::Matrix4()>(),

            /*          "Scale", &Maths::Matrix4::Scale,
            "Rotation", &Maths::Matrix4::Rotation,
            "Translation", &Maths::Matrix4::Translation,*/

            sol::meta_function::multiplication, sol::overload(static_cast<Maths::Vector4 (Maths::Matrix4::*)(const Maths::Vector4&) const>(&Maths::Matrix4::operator*), static_cast<Maths::Matrix4 (Maths::Matrix4::*)(const Maths::Matrix4&) const>(&Maths::Matrix4::operator*)));

        state.new_usertype<Maths::Transform>("Transform",
            sol::constructors<Maths::Transform(Maths::Matrix4), Maths::Transform(), Maths::Transform(Maths::Vector3)>(),

            "LocalScale", &Maths::Transform::GetLocalScale,
            "LocalOrientation", &Maths::Transform::GetLocalOrientation,
            "LocalPosition", &Maths::Transform::GetLocalPosition,
            "ApplyTransform", &Maths::Transform::ApplyTransform,
            "UpdateMatrices", &Maths::Transform::UpdateMatrices,
            "SetLocalTransform", &Maths::Transform::SetLocalTransform,
            "SetLocalPosition", &Maths::Transform::SetLocalPosition,
            "SetLocalScale", &Maths::Transform::SetLocalScale,
            "SetLocalOrientation", &Maths::Transform::SetLocalOrientation,
            "GetWorldPosition", &Maths::Transform::GetWorldPosition,
            "GetWorldOrientation", &Maths::Transform::GetWorldOrientation,
            "GetForwardDirection", &Maths::Transform::GetForwardDirection);
    }
}
