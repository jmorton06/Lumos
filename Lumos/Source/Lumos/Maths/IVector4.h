#pragma once
#include "Core/Core.h"

namespace Lumos
{
    namespace Maths
    {
        class Vector2;
        class Vector3;
        class LUMOS_EXPORT MEM_ALIGN IVector4
        {
        public:
            IVector4()
            {
                x = y = z = w = 0;
            }

            IVector4(i32 xVal, i32 yVal, i32 zVal, i32 wVal)
            {
                x = xVal;
                y = yVal;
                z = zVal;
                w = wVal;
            }

            IVector4(i32 xVal)
            {
                x = y = z = w = xVal;
            }

            IVector4(const IVector4& v)
            {
                x = v.x;
                y = v.y;
                z = v.z;
                w = v.w;
            }

            IVector4(const Vector3& v, i32 wVal);
            IVector4(i32 a, i32 b, const Vector2& cd);
            IVector4(const Vector2& ab, i32 c, i32 d);
            IVector4(i32 a, const Vector2& bc, i32 d);
            IVector4(const Vector2& ab, const Vector2& cd);

            i32 x;
            i32 y;
            i32 z;
            i32 w;

            i32* GetPointer() { return &x; }

            Vector2 ToVector2() const;
            Vector3 ToVector3() const;

            inline void ToZero()
            {
                x = y = z = w = 0;
            }

            i32 Length() const;
            inline i32 LengthSquared() const
            {
                return (x * x) + (y * y) + (z * z) + (w * w);
            }

            inline void Normalise()
            {
                i32 length = Length();

                if(length != 0)
                {
                    length = 1 / length;
                    x      = x * length;
                    y      = y * length;
                    z      = z * length;
                    w      = w * length;
                }
            }

            IVector4 Normalised() const
            {
                IVector4 newVec = IVector4(x, y, z, w);
                newVec.Normalise();
                return newVec;
            }

            inline i32 Dot(const IVector4& v)
            {
                return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
            }

            bool Equals(const IVector4& rhs) const;

            inline IVector4 operator+(i32 v) const { return IVector4(x + v, y + v, z + v, w + v); }
            inline IVector4 operator-(i32 v) const { return IVector4(x - v, y - v, z - v, w - v); }
            inline IVector4 operator*(i32 v) const { return IVector4(x * v, y * v, z * v, w * v); }
            inline IVector4 operator/(i32 v) const { return IVector4(x / v, y / v, z / v, w / v); }
            inline void operator+=(i32 v)
            {
                x += v;
                y += v;
                z += v;
                w += v;
            }
            inline void operator-=(i32 v)
            {
                x -= v;
                y -= v;
                z -= v;
                w -= v;
            }
            inline void operator*=(i32 v)
            {
                x *= v;
                y *= v;
                z *= v;
                w *= v;
            }
            inline void operator/=(i32 v)
            {
                x /= v;
                y /= v;
                z /= v;
                w /= v;
            }

            inline IVector4 operator+(const IVector4& v) const { return IVector4(x + v.x, y + v.y, z + v.z, w + v.w); }
            inline IVector4 operator-(const IVector4& v) const { return IVector4(x - v.x, y - v.y, z - v.z, w - v.w); }
            inline IVector4 operator*(const IVector4& v) const { return IVector4(x * v.x, y * v.y, z * v.z, w * v.w); }
            inline IVector4 operator/(const IVector4& v) const { return IVector4(x / v.x, y / v.y, z / v.z, w / v.w); }
            inline void operator+=(const IVector4& v)
            {
                x += v.x;
                y += v.y;
                z += v.z;
                w += v.w;
            }
            inline void operator-=(const IVector4& v)
            {
                x -= v.x;
                y -= v.y;
                z -= v.z;
                w -= v.w;
            }
            inline void operator*=(const IVector4& v)
            {
                x *= v.x;
                y *= v.y;
                z *= v.z;
                w *= v.w;
            }
            inline void operator/=(const IVector4& v)
            {
                x /= v.x;
                y /= v.y;
                z /= v.z;
                w /= v.w;
            }

            inline IVector4 operator-() const { return IVector4(-x, -y, -z, -w); }
            inline bool operator==(const IVector4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
            inline bool operator!=(const IVector4& v) const
            {
                return (v.x == x && v.y == y && v.z == z && v.w == w) ? false : true;
            }
        };

        inline IVector4 operator+(i32 f, const IVector4& v)
        {
            return v + f;
        }
        inline IVector4 operator*(i32 f, const IVector4& v)
        {
            return v * f;
        }

        inline IVector4 operator-(i32 f, const IVector4& v)
        {
            return IVector4(f) - v;
        }
        inline IVector4 operator/(i32 f, const IVector4& v)
        {
            return IVector4(f) / v;
        }
    }
    typedef Maths::IVector4 IVec4;
}
