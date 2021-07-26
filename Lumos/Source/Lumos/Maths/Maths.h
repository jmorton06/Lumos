#pragma once
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"
#include "Maths/Plane.h"
#include "Maths/Quaternion.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/BoundingBox.h"
#include "Maths/Sphere.h"
#include "Maths/Frustum.h"
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

        inline Vector2 ToVector(const Vector2Simple& vec)
        {
            return Vector2(vec.x, vec.y);
        }

        inline Vector3 ToVector(const Vector3Simple& vec)
        {
            return Vector3(vec.x, vec.y, vec.z);
        }

        inline Vector3Simple ToVector(const Vector3& vec)
        {
            return Vector3Simple(vec.x, vec.y, vec.z);
        }

        inline Vector4 ToVector(const Vector4Simple& vec)
        {
            return Vector4(vec.x, vec.y, vec.z, vec.w);
        }

        inline Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4& mvp, float width, float height, float winPosX = 0.0f, float winPosY = 0.0f)
        {
            Vector4 trans = mvp * Vector4(worldPos, 1.0f);
            trans *= 0.5f / trans.w;
            trans += Vector4(0.5f, 0.5f, 0.0f, 0.0f);
            trans.y = 1.f - trans.y;
            trans.x *= width;
            trans.y *= height;
            trans.x += winPosX;
            trans.y += winPosY;
            return Vector2(trans.x, trans.y);
        }

        template <typename T>
        static const float* ValuePointer(const T& t)
        {
            LUMOS_ASSERT(false, "Unimplemented Value Pointer");
            return nullptr;
        }

        template <typename T>
        static float* ValuePointer(T& t)
        {
            LUMOS_ASSERT(false, "Unimplemented Value Pointer");
            return nullptr;
        }
    }

    template <>
    inline const float* Maths::ValuePointer<Maths::Vector2>(const Maths::Vector2& t)
    {
        return &(t.x);
    }

    template <>
    inline const float* Maths::ValuePointer<Maths::Vector3>(const Maths::Vector3& t)
    {
        return &(t.x);
    }

    template <>
    inline const float* Maths::ValuePointer<Maths::Vector4>(const Maths::Vector4& t)
    {
        return &(t.x);
    }

    template <>
    inline const float* Maths::ValuePointer<Maths::Matrix3>(const Maths::Matrix3& t)
    {
        return &(t.m00_);
    }

    template <>
    inline const float* Maths::ValuePointer<Maths::Matrix4>(const Maths::Matrix4& t)
    {
        return &(t.m00_);
    }

    template <>
    inline const float* Maths::ValuePointer<Maths::Quaternion>(const Maths::Quaternion& t)
    {
        return &(t.w);
    }

    template <>
    inline float* Maths::ValuePointer<Maths::Vector2>(Maths::Vector2& t)
    {
        return &(t.x);
    }

    template <>
    inline float* Maths::ValuePointer<Maths::Vector3>(Maths::Vector3& t)
    {
        return &(t.x);
    }

    template <>
    inline float* Maths::ValuePointer<Maths::Vector4>(Maths::Vector4& t)
    {
        return &(t.x);
    }

    template <>
    inline float* Maths::ValuePointer<Maths::Matrix3>(Maths::Matrix3& t)
    {
        return &(t.m00_);
    }

    template <>
    inline float* Maths::ValuePointer<Maths::Matrix4>(Maths::Matrix4& t)
    {
        return &(t.m00_);
    }

    template <>
    inline float* Maths::ValuePointer<Maths::Quaternion>(Maths::Quaternion& t)
    {
        return &(t.w);
    }
}
