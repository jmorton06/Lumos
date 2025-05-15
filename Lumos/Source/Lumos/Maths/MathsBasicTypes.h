#pragma once
#include "Core/Core.h"

#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"

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
                , z(z) { };
            float x, y, z;
        };

        struct Vector4Simple
        {
            float x, y, z, w;
        };

        inline Vec2 ToVector(const Vector2Simple& vec)
        {
            return Vec2(vec.x, vec.y);
        }

        inline Vec3 ToVector(const Vector3Simple& vec)
        {
            return Vec3(vec.x, vec.y, vec.z);
        }

        inline Vec4 ToVector4(const Vector3Simple& vec)
        {
            return Vec4(vec.x, vec.y, vec.z, 1.0f);
        }

        inline Vector3Simple ToVector(const Vec3& vec)
        {
            return Vector3Simple(vec.x, vec.y, vec.z);
        }

        inline Vec4 ToVector(const Vector4Simple& vec)
        {
            return Vec4(vec.x, vec.y, vec.z, vec.w);
        }
    }
}
