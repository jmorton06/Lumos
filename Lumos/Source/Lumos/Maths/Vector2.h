#pragma once
#ifdef LUMOS_SSE_VEC2
#include <smmintrin.h>
#endif
#include <math.h>

namespace Lumos
{
    namespace Maths
    {
        class LUMOS_EXPORT MEM_ALIGN Vector2
        {
        public:
            Vector2() { ToZero(); }
            Vector2(const float x, const float y)
                : x(x)
                , y(y)
            {
            }
            Vector2(const float x)
                : x(x)
                , y(x)
            {
            }
            ~Vector2() { }

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

        public:
            void ToZero()
            {
                x = 0.0f;
                y = 0.0f;
            }

            inline void Normalise()
            {
                float length = Length();

                if(length != 0.0f)
                {
                    length = 1.0f / length;
                    x      = x * length;
                    y      = y * length;
                }
            }

            inline float LengthSquared() const { return ((x * x) + (y * y)); }
            inline float Length() const { return float(sqrt((x * x) + (y * y))); }

            inline Vector2 operator-(const Vector2& a) const { return Vector2(x - a.x, y - a.y); }
            inline Vector2 operator+(const Vector2& a) const { return Vector2(x + a.x, y + a.y); }
            inline Vector2 operator*(const Vector2& v) const { return Vector2(x * v.x, y * v.y); };
            inline Vector2 operator/(const Vector2& v) const { return Vector2(x / v.x, y / v.y); };
            inline Vector2 operator+(const float a) const { return Vector2(x + a, y + a); }
            inline Vector2 operator-(const float a) const { return Vector2(x - a, y - a); }
            inline Vector2 operator*(const float a) const { return Vector2(x * a, y * a); }
            inline Vector2 operator/(const float v) const { return Vector2(x / v, y / v); };

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

            inline void operator+=(const float v)
            {
                x += v;
                y += v;
            }
            inline void operator-=(const float v)
            {
                x -= v;
                y -= v;
            }
            inline void operator*=(const float v)
            {
                x *= v;
                y *= v;
            }
            inline void operator/=(const float v)
            {
                x /= v;
                y /= v;
            }

            inline bool operator<(const Vector2& v) { return x < v.x && y < v.y; }
            inline bool operator<=(const Vector2& v) { return x <= v.x && y <= v.y; }
            inline bool operator>(const Vector2& v) { return x > v.x && y > v.y; }
            inline bool operator>=(const Vector2& v) { return x >= v.x && y >= v.y; }
            inline bool operator==(const Vector2& A) const { return (A.x == x && A.y == y) ? true : false; };

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
                default:
                case 0:
                    return x;
                case 1:
                    return y;
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
