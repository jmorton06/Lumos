#pragma once
#include "Core/Core.h"
#ifdef LUMOS_SSE_VEC3
#include <smmintrin.h>
#endif
namespace Lumos
{
    namespace Maths
    {
        class Vector2;
        class Vector4;
        class MEM_ALIGN Vector3
        {
        public:
            Vector3()
            {
#ifdef LUMOS_SSE_VEC3
                m_Value = _mm_set1_ps(0.0f);
#else
                x = y = z = 0.0f;
#endif
            }

            Vector3(const float xVal, const float yVal, const float zVal)
            {
#ifdef LUMOS_SSE_VEC3
                m_Value = _mm_set_ps(0, zVal, yVal, xVal);
#else
                x = xVal;
                y = yVal;
                z = zVal;
#endif
            }

            explicit Vector3(const float value)
            {
#ifdef LUMOS_SSE_VEC3
                m_Value = _mm_set_ps(0, value, value, value);
#else
                x = y = z = value;
#endif
            }

            Vector3(const Vector2& vec2, float zVal);
            Vector3(const Vector4& vec4);
            Vector3(float* value)
            {
#ifdef LUMOS_SSE_VEC3
                m_Value = _mm_set_ps(0, value[2], value[1], value[0]);
#else
                x = value[0];
                y = value[1];
                z = value[2];
#endif
            }

#ifdef LUMOS_SSE_VEC3
            Vector3(const __m128 m)
                : m_Value(m)
            {
            }
#endif

            ~Vector3() = default;

            static Vector3 Zero() { return Vector3(); };
            static Vector3 ZAxis() { return Vector3(0.0f, 0.0f, 1.0f); };

#ifdef LUMOS_SSE_VEC3
            union
            {
                struct
                {
                    float x;
                    float y;
                    float z;
                    // Add padding to align the size to 16 bytes for SSE
                    float _p0; // This ensures the size of the struct is 16 bytes
                };
                __m128 m_Value;
            } MEM_ALIGN;
#else
            float x;
            float y;
            float z;
            // Add padding to align the size to 16 bytes for SSE
            float _p0; // This ensures the size of the struct is 16 bytes
#endif

            float* ValuePtr()
            {
                return &x;
            }

            void ToZero()
            {
#ifdef LUMOS_SSE_VEC3
                m_Value = _mm_setzero_ps();
#else
                x = y = z = 0.0f;
#endif
            }

            bool IsValid() const;
            bool IsInf() const;
            bool IsNaN() const;
            bool IsZero() const;

            static float Sqrt(float x);

            float Length() const
            {
                return Sqrt(Dot(*this, *this));
            }

            float LengthSquared() const
            {
#ifdef LUMOS_SSE_VEC3
                return _mm_cvtss_f32(_mm_dp_ps(m_Value, m_Value, 0x71)); // Mask 0x71 specifies x, y, z elements and stores the result in the x element
#else
                return (x * x + y * y + z * z);
#endif
            }

            Vector3 Inverse() const
            {
                return Vector3(-x, -y, -z);
            }

            void Invert()
            {
#ifdef LUMOS_SSE_VEC3
                m_Value = _mm_mul_ps(m_Value, _mm_set1_ps(-1.0f));
#else
                x = -x;
                y = -y;
                z = -z;
#endif
            }

            void Normalise()
            {
#ifdef LUMOS_SSE_VEC3
                __m128 dot = _mm_dp_ps(m_Value, m_Value, 0x7F);

                if(_mm_cvtss_f32(dot) == 0.0f)
                {
                    // Handle the zero vector case, e.g., return a zero vector
                    m_Value = _mm_setzero_ps();
                }
                else
                {
                    __m128 rsqrt = _mm_rsqrt_ps(dot);

                    // One iteration of Newton-Raphson refinement
                    __m128 half_dot     = _mm_mul_ps(dot, _mm_set1_ps(0.5f));
                    __m128 three_halves = _mm_set1_ps(1.5f);
                    rsqrt               = _mm_mul_ps(rsqrt, _mm_sub_ps(three_halves, _mm_mul_ps(half_dot, _mm_mul_ps(rsqrt, rsqrt))));

                    m_Value = _mm_mul_ps(m_Value, rsqrt);
                }
#else
                float length = Length();
                if(length != 0.0f)
                {
                    length = 1.0f / length;
                    x *= length;
                    y *= length;
                    z *= length;
                }
#endif
            }

            Vector3 Normalised() const
            {
                Vector3 newVec = Vector3(x, y, z);
                newVec.Normalise();
                return newVec;
            }

            Vector2 ToVector2() const;

            static float Dot(const Vector3& a, const Vector3& b)
            {
#ifdef LUMOS_SSE_VEC3
                return _mm_cvtss_f32(_mm_dp_ps(a.m_Value, b.m_Value, 0x7f));
#else
                return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
#endif
            }

            float Dot(const Vector3& other) const
            {
                return Dot(*this, other);
            }

            static Vector3 Cross(const Vector3& a, const Vector3& b)
            {
#ifdef LUMOS_SSE_VEC3
                return _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a.m_Value, a.m_Value, _MM_SHUFFLE(3, 0, 2, 1)),
                               _mm_shuffle_ps(b.m_Value, b.m_Value, _MM_SHUFFLE(3, 1, 0, 2))),
                    _mm_mul_ps(_mm_shuffle_ps(a.m_Value, a.m_Value, _MM_SHUFFLE(3, 1, 0, 2)),
                               _mm_shuffle_ps(b.m_Value, b.m_Value, _MM_SHUFFLE(3, 0, 2, 1))));
#else
                return { (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) };
#endif
            }

            Vector3 Cross(const Vector3& other) const
            {
                return Cross(*this, other);
            }

#ifdef LUMOS_SSE_VEC3
            inline Vector3 operator+(float v) const { return _mm_add_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector3 operator-(float v) const { return _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector3 operator*(float v) const { return _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector3 operator/(float v) const { return _mm_div_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator+=(float v) { m_Value = _mm_add_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator-=(float v) { m_Value = _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator*=(float v) { m_Value = _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator/=(float v) { m_Value = _mm_div_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector3 operator-() const { return _mm_set_ps(0, -z, -y, -x); }

            inline Vector3 operator+(const Vector3& v) const { return _mm_add_ps(m_Value, v.m_Value); }
            inline Vector3 operator-(const Vector3& v) const { return _mm_sub_ps(m_Value, v.m_Value); }
            inline Vector3 operator*(const Vector3& v) const { return _mm_mul_ps(m_Value, v.m_Value); }
            inline Vector3 operator/(const Vector3& v) const { return _mm_div_ps(m_Value, v.m_Value); }
            inline void operator+=(const Vector3& v) { m_Value = _mm_add_ps(m_Value, v.m_Value); }
            inline void operator-=(const Vector3& v) { m_Value = _mm_sub_ps(m_Value, v.m_Value); }
            inline void operator*=(const Vector3& v) { m_Value = _mm_mul_ps(m_Value, v.m_Value); }
            inline void operator/=(const Vector3& v) { m_Value = _mm_div_ps(m_Value, v.m_Value); }
#else
            inline Vector3 operator+(const float v) const { return Vector3(x + v, y + v, z + v); }
            inline Vector3 operator-(const float v) const { return Vector3(x - v, y - v, z - v); }
            inline Vector3 operator*(const float v) const { return Vector3(x * v, y * v, z * v); }
            inline Vector3 operator/(const float v) const { return Vector3(x / v, y / v, z / v); };
            inline void operator+=(const float v)
            {
                x += v;
                y += v;
                z += v;
            }
            inline void operator-=(const float v)
            {
                x -= v;
                y -= v;
                z -= v;
            }
            inline void operator*=(const float v)
            {
                x *= v;
                y *= v;
                z *= v;
            }
            inline void operator/=(const float v)
            {
                x /= v;
                y /= v;
                z /= v;
            }
            inline Vector3 operator-() const { return Vector3(-x, -y, -z); }

            inline Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
            inline Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
            inline Vector3 operator*(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
            inline Vector3 operator/(const Vector3& v) const { return Vector3(x / v.x, y / v.y, z / v.z); };
            inline void operator+=(const Vector3& v)
            {
                x += v.x;
                y += v.y;
                z += v.z;
            }
            inline void operator-=(const Vector3& v)
            {
                x -= v.x;
                y -= v.y;
                z -= v.z;
            }
            inline void operator*=(const Vector3& v)
            {
                x *= v.x;
                y *= v.y;
                z *= v.z;
            }
            inline void operator/=(const Vector3& v)
            {
                x /= v.x;
                y /= v.y;
                z /= v.z;
            }
#endif

            inline bool operator<(const Vector3& other) const { return x < other.x && y < other.y && z < other.z; }
            inline bool operator<=(const Vector3& other) const { return x <= other.x && y <= other.y && z <= other.z; }
            inline bool operator>(const Vector3& other) const { return x > other.x && y > other.y && z > other.z; }
            inline bool operator>=(const Vector3& other) const { return x >= other.x && y >= other.y && z >= other.z; }

            inline bool operator==(const Vector3& v) const
            {
#ifdef LUMOS_SSE_VEC3
                return (_mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) & 0x7) == 0;
#else
                return (v.x == x && v.y == y && v.z == z) ? true : false;
#endif
            }

            inline bool operator!=(const Vector3& v) const
            {
#ifdef LUMOS_SSE_VEC3
                return (_mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) & 0x7) != 0;
#else
                return (v.x == x && v.y == y && v.z == z) ? false : true;
#endif
            }

            inline float operator[](int i) const
            {
                switch(i)
                {
                case 0:
                    return x;
                case 1:
                    return y;
                case 2:
                    return z;
                default:
                    return 0.0f;
                }
            }

            static inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
            {
                return a * (1.0f - t) + b * t;
            }
        };
        inline Vector3 operator*(float lhs, const Vector3& rhs)
        {
            return rhs * lhs;
        }
    }

    typedef Maths::Vector3 Vec3;
}

static_assert(sizeof(Lumos::Maths::Vector3) == MEM_ALIGNMENT, "Vector3 size must be 16 bytes");
