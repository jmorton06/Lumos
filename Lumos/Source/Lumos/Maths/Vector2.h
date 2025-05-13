#pragma once

#include <math.h>

#ifdef LUMOS_SSE_VEC2
#include <smmintrin.h>
#endif

namespace Lumos
{
    namespace Maths
    {
        class LUMOS_EXPORT MEM_ALIGN Vector2
        {
        public:
            Vector2()
            {
                ToZero();
            }

            Vector2(float x, float y)
                : x(x)
                , y(y)
            {
            }

            Vector2(float value)
                : x(value)
                , y(value)
            {
            }

            ~Vector2() = default;

#ifdef LUMOS_SSE_VEC2
            union
            {
                struct
                {
                    float x, y;
                };
                __m128 mmvalue;
            } MEM_ALIGN;
#else
            float x, y;
#endif

            void ToZero()
            {
                x = 0.0f;
                y = 0.0f;
            }

            void Normalise()
            {
                float len = Length();
                if(len != 0.0f)
                {
                    float invLen = 1.0f / len;
                    x *= invLen;
                    y *= invLen;
                }
            }

            Vector2 Normalised() const
            {
                Vector2 result(*this);
                result.Normalise();
                return result;
            }

            float Dot(const Vector2& other) const
            {
                return (x * other.x) + (y * other.y);
            }

            float LengthSquared() const
            {
                return (x * x + y * y);
            }

            float Length() const
            {
                return sqrtf(LengthSquared());
            }

            // Arithmetic Operators
            inline Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
            inline Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
            inline Vector2 operator*(const Vector2& v) const { return { x * v.x, y * v.y }; }
            inline Vector2 operator/(const Vector2& v) const { return { x / v.x, y / v.y }; }

            inline Vector2 operator+(float s) const { return { x + s, y + s }; }
            inline Vector2 operator-(float s) const { return { x - s, y - s }; }
            inline Vector2 operator*(float s) const { return { x * s, y * s }; }
            inline Vector2 operator/(float s) const { return { x / s, y / s }; }

            inline void operator+=(const Vector2& v)
            {
                x += v.x;
                y += v.y;
            }
            inline void operator-=(const Vector2& v)
            {
                x -= v.x;
                y -= v.y;
            }
            inline void operator*=(const Vector2& v)
            {
                x *= v.x;
                y *= v.y;
            }
            inline void operator/=(const Vector2& v)
            {
                x /= v.x;
                y /= v.y;
            }

            inline void operator+=(float s)
            {
                x += s;
                y += s;
            }
            inline void operator-=(float s)
            {
                x -= s;
                y -= s;
            }
            inline void operator*=(float s)
            {
                x *= s;
                y *= s;
            }
            inline void operator/=(float s)
            {
                x /= s;
                y /= s;
            }

            // Comparison Operators
            inline bool operator==(const Vector2& v) const { return x == v.x && y == v.y; }
            inline bool operator!=(const Vector2& v) const { return !(*this == v); }
            inline bool operator<(const Vector2& v) const { return x < v.x && y < v.y; }
            inline bool operator<=(const Vector2& v) const { return x <= v.x && y <= v.y; }
            inline bool operator>(const Vector2& v) const { return x > v.x && y > v.y; }
            inline bool operator>=(const Vector2& v) const { return x >= v.x && y >= v.y; }

            // Indexing
            inline float operator[](int i) const
            {
                switch(i)
                {
                case 0:
                    return x;
                case 1:
                    return y;
                default:
                    return 0.0f;
                }
            }

            inline float& operator[](int i)
            {
                switch(i)
                {
                case 0:
                    return x;
                case 1:
                    return y;
                default:
                    return x; // fallback
                }
            }
        };

        inline Vector2 operator*(float lhs, const Vector2& rhs)
        {
            return rhs * lhs;
        }
    }

    typedef Maths::Vector2 Vec2;
}
