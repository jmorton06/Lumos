#include “LM.h”
#include "MathsBindings.h"
#include "Maths/Maths.h"
#include <sol/sol.hpp>

void Lumos::Sctipting::BindMaths(sol::state &luaState)
{
    using namespace Maths;
//    luaState.new_usertype<Vector2>("Vector2",
//                                      /* Constructors */
//                                      sol::constructors
//                                      <
//                                      Vector2(),
//                                      Vector2(float, float)
//                                      >(),
//
//                                      /* Operators */
//                                      sol::meta_function::addition, &Vector2::operator+,
//                                      sol::meta_function::subtraction, sol::resolve<Vector2(const Vector2&) const>(&Vector2::operator-),
//                                      //sol::meta_function::unary_minus, sol::resolve<Vector2() const>(&Vector2::operator-),
//                                      sol::meta_function::multiplication, &Vector2::operator*,
//                                      sol::meta_function::division, &Vector2::operator/,
//                                      sol::meta_function::to_string, [](const Vector2& target) { return "(" + std::to_string(target.x) + "," + std::to_string(target.y) + ")"; },
//
//                                      /* Variables */
//                                      "x", &Vector2::x,
//                                      "y", &Vector2::y,
//
//                                      /* Data */
//                                      //"One", []() { return Vector2::One; },
//                                      //"Zero", []() { return Vector2::Zero; },
//
//                                      /* Methods */
//                                      "Length", &Vector2::Length
//                                    //"Dot", &Vector2::Dot,
//                                      //"Normalize", &Vector2::Normalize,
//                                      //"Lerp", &Vector2::Lerp,
//                                      //"AngleBetween", &Vector2::AngleBetween
//                                      );
//
//    luaState.new_usertype<Vector3>("Vector3",
//                                      /* Constructors */
//                                      sol::constructors
//                                      <
//                                      Vector3(),
//                                      Vector3(float, float, float)
//                                      >(),
//
//                                      /* Operators */
//                                      sol::meta_function::addition, &Vector3::operator+,
//                                      sol::meta_function::subtraction, sol::resolve<Vector3(const Vector3&) const>(&Vector3::operator-),
//                                      sol::meta_function::unary_minus, sol::resolve<Vector3() const>(&Vector3::operator-),
//                                      sol::meta_function::multiplication, &Vector3::operator*,
//                                      sol::meta_function::division, &Vector3::operator/,
//                                      sol::meta_function::to_string, [](const Vector3& target) { return "(" + std::to_string(target.x) + "," + std::to_string(target.y) + "," + std::to_string(target.z) + ")"; },
//
//                                      /* Variables */
//                                      "x", &Vector3::x,
//                                      "y", &Vector3::y,
//                                      "z", &Vector3::z,
//
//                                      /* Data */
//                                      //"One", []() { return Vector3::One; },
//                                      "Zero", []() { return Vector3::Zero; },
//                                      //"Forward", []() { return Vector3::Forward; },
//                                      //"Up", []() { return Vector3::Up; },
//                                      //"Right", []() { return Vector3::Right; },
//                                      //"Backward", []() { return Vector3::Forward * -1; },
//                                      //"Down", []() { return Vector3::Up * -1; },
//                                     // "Left", []() { return Vector3::Right * -1; },
//
//                                      /* Methods */
//                                      "Length", &Vector3::Length,
//                                      "Dot", &Vector3::Dot,
//                                      "Cross", &Vector3::Cross,
//                                      "Normalize", &Vector3::Normalise,
//                                      "Lerp", &Vector3::Lerp
//                                      //"AngleBetween", &Vector3::AngleBetween
//                                      );
//
//    luaState.new_usertype<Vector4>("Vector4",
//                                      /* Constructors */
//                                      sol::constructors
//                                      <
//                                      Vector4(),
//                                      Vector4(float, float, float, float)
//                                      >(),
//
//                                      /* Operators */
//                                      sol::meta_function::addition, &Vector4::operator+,
//                                      sol::meta_function::subtraction, sol::resolve<Vector4(const Vector4&) const>(&Vector4::operator-),
//                                      sol::meta_function::unary_minus, sol::resolve<Vector4() const>(&Vector4::operator-),
//                                      sol::meta_function::multiplication, &Vector4::operator*,
//                                      sol::meta_function::division, &Vector4::operator/,
//                                      sol::meta_function::to_string, [](const Vector4& target) { return "(" + std::to_string(target.x) + "," + std::to_string(target.y) + "," + std::to_string(target.z) + "," + std::to_string(target.w) + ")"; },
//
//                                      /* Variables */
//                                      "x", &Vector4::x,
//                                      "y", &Vector4::y,
//                                      "z", &Vector4::z,
//                                      "w", &Vector4::w,
//
//                                      /* Data */
//                                     // "One", []() { return Vector4::One; },
//                                      //"Zero", []() { return Vector4::Zero; },
//
//                                      /* Methods */
//                                      "Length", &Vector4::Length,
//                                      "Dot", &Vector4::Dot,
//                                      "Normalize", &Vector4::Normalise
//                                      //"Lerp", &Vector4::Lerp
//                                      );
//
//    luaState.new_usertype<Matrix3>("Matrix3",
//                                      /* Constructors */
//                                      sol::constructors
//                                      <
//                                      Matrix3()//,
//                                      //Matrix3(float)//,
//                                      //Matrix3(float, float, float, float, float, float, float, float, float)
//                                      >(),
//
//                                      /* Operators */
//                                      //sol::meta_function::addition, &Matrix3::operator+,
//                                      //sol::meta_function::subtraction, &Matrix3::operator-,
//                                      sol::meta_function::multiplication, sol::overload
//                                      (
//                                      // sol::resolve<Matrix3(float) const>(&Matrix3::operator*),
//                                       sol::resolve<Vector3(const Vector3&) const>(&Matrix3::operator*),
//                                       sol::resolve<Matrix3(const Matrix3&) const>(&Matrix3::operator*)
//                                       ),
//                                      sol::meta_function::division, sol::overload
//                                      (
//                                      // sol::resolve<Matrix3(float) const>(&Matrix3::operator/),
//                                       //sol::resolve<Matrix3(const Matrix3&) const>(&Matrix3::operator/)
//                                       ),
//                                      sol::meta_function::to_string, [](const Matrix3& target) { return "Can't show matrix as string for now"; },
//
//                                      /* Data */
//                                      //"Identity", []() { return Matrix3::Identity; },
//
//                                      /* Methods */
//                                     // "IsIdentity", &Matrix3::IsIdentity,
//                                      "Determinant", &Matrix3::Determinant,
//                                      "Transpose", &Matrix3::Transpose,
//                                     // "Cofactor", &Matrix3::Cofactor,
//                                     // "Minor", &Matrix3::Minor,
//                                     // "Adjoint", &Matrix3::Adjoint,
//                                      "Inverse", &Matrix3::Inverse,
//                                     // "Translation", &Matrix3::Translation,
//                                     // "Translate", &Matrix3::Translate,
//                                      "Rotation", &Matrix3::Rotation,
//                                     // "Rotate", &Matrix3::Rotate,
//                                     // "Scaling", &Matrix3::Scaling,
//                                      "Scale", &Matrix3::Scale,
//                                      "GetRow", &Matrix3::GetRow,
//                                    //  "GetColumn", &Matrix3::GetColumn,
//                                      "Get", [](Matrix3& target, int row, int col) { return target(row, col); },
//                                      "Set", [](Matrix3& target, int row, int col, float value) { target(row, col) = value; }
//                                      );
//
//    luaState.new_usertype<Matrix4>("Matrix4",
//                                      /* Constructors */
//                                      sol::constructors
//                                      <
//                                      Matrix4()//,
//                                     // Matrix4(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float)
//                                      >(),
//
//                                      /* Operators */
//                                     // sol::meta_function::addition, &Matrix4::operator+,
////                                      sol::meta_function::subtraction, sol::overload
////                                      (
////                                       sol::resolve<Matrix4(float) const>(&Matrix4::operator-),
////                                       sol::resolve<Matrix4(const Matrix4&) const>(&Matrix4::operator-)
////                                       ),
//                                      sol::meta_function::multiplication, sol::overload
//                                      (
//                                       //sol::resolve<Matrix4(float) const>(&Matrix4::operator*),
//                                       sol::resolve<Vector4(const Vector4&) const>(&Matrix4::operator*),
//                                       sol::resolve<Matrix4(const Matrix4&) const>(&Matrix4::operator*)
//                                       ),
////                                      sol::meta_function::division, sol::overload
////                                      (
////                                       sol::resolve<Matrix4(float) const>(&Matrix4::operator/),
////                                       sol::resolve<Matrix4(const Matrix4&) const>(&Matrix4::operator/)
////                                       ),
//                                      sol::meta_function::to_string, [](const Matrix4& target) { return "Can't show matrix as string for now"; },
//
//                                      /* Data */
//                                      "Identity", []() { return Matrix4(); },
//
//                                      /* Methods */
//                                      //"IsIdentity", &Matrix4::IsIdentity,
//                                      //"Determinant", &Matrix4::Determinant,
//                                      "Transpose", &Matrix4::Transpose,
//                                      //"Minor", &Matrix4::GetMinor,
//                                      "Inverse", &Matrix4::Inverse,
//                                      "Translation", &Matrix4::Translation,
//                                      //"Translate", &Matrix4::Translate,
//                                      //"RotationOnAxisX", &Matrix4::RotationOnAxisX,
//                                      //"RotateOnAxisX", &Matrix4::RotateOnAxisX,
//                                      //"RotationOnAxisY", &Matrix4::RotationOnAxisY,
//                                      //"RotateOnAxisY", &Matrix4::RotateOnAxisY,
//                                      //"RotationOnAxisZ", &Matrix4::RotationOnAxisZ,
//                                      //"RotateOnAxisZ", &Matrix4::RotateOnAxisZ,
//                                      //"RotationYXZ", &Matrix4::RotationYXZ,
//                                      //"RotateYXZ", &Matrix4::RotateYXZ,
//                                      //"Scaling", &Matrix4::Scaling,
//                                      "Scale", &Matrix4::Scale,
//                                      "GetRow", &Matrix4::GetRow
//                                      //"GetColumn", &Matrix4::GetColumn,
//                                      //"CreatePerspective", &Matrix4::CreatePerspective,
//                                      //"CreateView", &Matrix4::CreateView,
//                                     // "CreateFrustum", &Matrix4::CreateFrustum,
//                                      //"Get", [](Matrix4& target, int row, int col) { return target[row + column * 4]; },
//                                      //"Set", [](Matrix4& target, int row, int col, float value) { target(row, col) = value; }
//                                      );
//
//    //auto RotatePointOverload = sol::overload
//    //(
//     //sol::resolve<Vector3(const Vector3&, const Quaternion&)>(&Quaternion::RotatePoint),                    // Rotate without pivot
//     //sol::resolve<Vector3(const Vector3&, const Quaternion&, const Vector3&)>(&Quaternion::RotatePoint) // Rotate with pivot
//    // );
//
//    luaState.new_usertype<Quaternion>("Quaternion",
//                                         /* Constructors */
//                                         sol::constructors
//                                         <
//                                         Quaternion(),
//                                         Quaternion(float),
//                                         Quaternion(float, float, float, float),
//                                         Quaternion(const Vector3&)
//                                         >(),
//
//                                         /* Operators */
//                                         sol::meta_function::addition, &Quaternion::operator+,
//                                         //sol::meta_function::subtraction, &Quaternion::operator-,
//                                         //sol::meta_function::division, &Quaternion::operator/,
//                                         sol::meta_function::multiplication, sol::overload
//                                         (
//                                          //sol::resolve<Quaternion(float) const>(&Quaternion::operator*),
//                                          sol::resolve<Quaternion(const Quaternion&) const>(&Quaternion::operator*)//,
//                                         // sol::resolve<Matrix3(const Matrix3&) const>(&Quaternion::operator*),
//                                         // sol::resolve<Vector3(const Vector3&) const>(&Quaternion::operator*)
//                                          ),
//                                         sol::meta_function::to_string, [](const Quaternion& target) { return "(" + std::to_string(target.x) + "," + std::to_string(target.y) + "," + std::to_string(target.z) + "," + std::to_string(target.w) + ")"; },
//
//                                         /* Methods */
//                                         //"IsIdentity", &Quaternion::IsIdentity,
//                                         //"IsPure", &Quaternion::IsPure,
//                                         //"IsNormalized", &Quaternion::IsNormalized,
//                                         //"Dot", &Quaternion::DotProduct,
//                                         //"Normalize", &Quaternion::Normalize,
//                                        // "Length", &Quaternion::Length,
//                                        // "LengthSquare", &Quaternion::LengthSquare,
//                                         //"GetAngle", &Quaternion::GetAngle,
//                                         //"GetRotationAxis", &Quaternion::GetRotationAxis,
//                                         "Inverse", &Quaternion::Inverse,
//                                         "Conjugate", &Quaternion::Conjugate,
//                                        // "Square", &Quaternion::Square,
//                                        // "GetAxisAndAngle", &Quaternion::GetAxisAndAngle,
//                                        // "AngularDistance", &Quaternion::AngularDistance,
//                                         "Lerp", &Quaternion::Lerp,
//                                         "Slerp", &Quaternion::Slerp,
//                                         //"Nlerp", &Quaternion::Nlerp,
//                                         //"RotatePoint", RotatePointOverload,
//                                        // "EulerAngles", &Quaternion::EulerAngles,
//                                         "ToMatrix3", &Quaternion::ToMatrix3,
//                                         "ToMatrix4", &Quaternion::ToMatrix4,
//
//                                         /* Variables */
//                                         "x", &Quaternion::x,
//                                         "y", &Quaternion::y,
//                                         "z", &Quaternion::z,
//                                         "w", &Quaternion::w
//                                         );
}
