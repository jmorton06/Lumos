#pragma once
#ifdef LUMOS_SSE
#include <smmintrin.h>
#endif
namespace Lumos
{
    namespace Maths
    {
        class Vector2;
        class Vector3;
        class LUMOS_EXPORT MEM_ALIGN Vector4
        {
        public:
            Vector4()
            {
#ifdef LUMOS_SSE
                m_Value = _mm_set1_ps(0.0f);
#else
                x = y = z = w = 0.0f;
#endif
            }

            Vector4(float xVal, float yVal, float zVal, float wVal)
            {
#ifdef LUMOS_SSE
                m_Value = _mm_set_ps(wVal, zVal, yVal, xVal);
#else
                x = xVal;
                y = yVal;
                z = zVal;
                w = wVal;
#endif
            }

            Vector4(float xVal)
            {
#ifdef LUMOS_SSE
                m_Value = _mm_set_ps(xVal, xVal, xVal, xVal);
#else
                x = y = z = w = xVal;
#endif
            }

#ifdef LUMOS_SSE
            Vector4(__m128 m)
                : m_Value(m)
            {
            }
#endif

            Vector4(const Vector4& v)
            {
#ifdef LUMOS_SSE
                m_Value = v.m_Value;
#else
                x = v.x;
                y = v.y;
                z = v.z;
                w = v.w;
#endif
            }

            Vector4(const Vector3& v, float wVal);
            Vector4(float a, float b, const Vector2& cd);
            Vector4(const Vector2& ab, float c, float d);
            Vector4(float a, const Vector2& bc, float d);
            Vector4(const Vector2& ab, const Vector2& cd);

#ifdef LUMOS_SSE
            union
            {
                struct
                {
                    float x;
                    float y;
                    float z;
                    float w;
                };
                __m128 m_Value;
            } MEM_ALIGN;
#else
            float x;
            float y;
            float z;
            float w;
#endif

            float* GetPointer()
            {
                return &x;
            }
            Vector2 ToVector2() const;
            Vector3 ToVector3() const;

            bool IsValid() const;
            bool IsInf() const;
            bool IsNaN() const;

            inline void ToZero()
            {
#ifdef LUMOS_SSE
                m_Value = _mm_setzero_ps();
#else
                x = y = z = w = 0.0f;
#endif
            }

            float Length() const;
            inline float LengthSquared() const
            {
#ifdef LUMOS_SSE
                return _mm_cvtss_f32(_mm_dp_ps(m_Value, m_Value, 0xF1));
#else
                return (x * x) + (y * y) + (z * z) + (w * w);
#endif
            }

            inline void Normalise()
            {
#ifdef LUMOS_SSE
                __m128 dot = _mm_dp_ps(m_Value, m_Value, 0xF1);

                if(_mm_cvtss_f32(dot) == 0.0f)
                {
                    m_Value = _mm_setzero_ps();
                }
                else
                {
                    __m128 dot_broadcasted = _mm_shuffle_ps(dot, dot, _MM_SHUFFLE(0, 0, 0, 0));

                    __m128 rsqrt = _mm_rsqrt_ps(dot_broadcasted);

                    // One iteration of Newton-Raphson refinement
                    __m128 half_dot     = _mm_mul_ps(dot_broadcasted, _mm_set1_ps(0.5f));
                    __m128 three_halves = _mm_set1_ps(1.5f);
                    rsqrt               = _mm_mul_ps(
                        rsqrt,
                        _mm_sub_ps(three_halves, _mm_mul_ps(half_dot, _mm_mul_ps(rsqrt, rsqrt))));

                    m_Value = _mm_mul_ps(m_Value, rsqrt);
                }
#else
                float length = sqrtf(x * x + y * y + z * z + w * w);
                if(length != 0.0f)
                {
                    float inv = 1.0f / length;
                    x *= inv;
                    y *= inv;
                    z *= inv;
                    w *= inv;
                }
                else
                {
                    x = y = z = w = 0.0f;
                }
#endif
            }

            Vector4 Normalised() const
            {
                Vector4 newVec = Vector4(x, y, z, w);
                newVec.Normalise();
                return newVec;
            }

            inline float Dot(const Vector4& v)
            {
#ifdef LUMOS_SSE
                return _mm_cvtss_f32(_mm_dp_ps(m_Value, v.m_Value, 0xF1));
#else
                return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
#endif
            }

            bool Equals(const Vector4& rhs) const;
            Vector4 Lerp(const Vector4& rhs, float t);

#ifdef LUMOS_SSE
            inline Vector4 operator+(float v) const { return _mm_add_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector4 operator-(float v) const { return _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector4 operator*(float v) const { return _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
            inline Vector4 operator/(float v) const { return _mm_div_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator+=(float v) { m_Value = _mm_add_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator-=(float v) { m_Value = _mm_sub_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator*=(float v) { m_Value = _mm_mul_ps(m_Value, _mm_set1_ps(v)); }
            inline void operator/=(float v) { m_Value = _mm_div_ps(m_Value, _mm_set1_ps(v)); }

            inline Vector4 operator+(const Vector4& v) const { return _mm_add_ps(m_Value, v.m_Value); }
            inline Vector4 operator-(const Vector4& v) const { return _mm_sub_ps(m_Value, v.m_Value); }
            inline Vector4 operator*(const Vector4& v) const { return _mm_mul_ps(m_Value, v.m_Value); }
            inline Vector4 operator/(const Vector4& v) const { return _mm_div_ps(m_Value, v.m_Value); }
            inline void operator+=(const Vector4& v) { m_Value = _mm_add_ps(m_Value, v.m_Value); }
            inline void operator-=(const Vector4& v) { m_Value = _mm_sub_ps(m_Value, v.m_Value); }
            inline void operator*=(const Vector4& v) { m_Value = _mm_mul_ps(m_Value, v.m_Value); }
            inline void operator/=(const Vector4& v) { m_Value = _mm_div_ps(m_Value, v.m_Value); }

            inline Vector4 operator-() const { return _mm_set_ps(-w, -z, -y, -x); }
            inline bool operator==(const Vector4& v) const { return _mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) == 0; }
            inline bool operator!=(const Vector4& v) const { return _mm_movemask_ps(_mm_cmpneq_ps(m_Value, v.m_Value)) != 0; }
#else
            inline Vector4 operator+(float v) const { return Vector4(x + v, y + v, z + v, w + v); }
            inline Vector4 operator-(float v) const { return Vector4(x - v, y - v, z - v, w - v); }
            inline Vector4 operator*(float v) const { return Vector4(x * v, y * v, z * v, w * v); }
            inline Vector4 operator/(float v) const { return Vector4(x / v, y / v, z / v, w / v); }
            inline void operator+=(float v)
            {
                x += v;
                y += v;
                z += v;
                w += v;
            }
            inline void operator-=(float v)
            {
                x -= v;
                y -= v;
                z -= v;
                w -= v;
            }
            inline void operator*=(float v)
            {
                x *= v;
                y *= v;
                z *= v;
                w *= v;
            }
            inline void operator/=(float v)
            {
                x /= v;
                y /= v;
                z /= v;
                w /= v;
            }

            inline Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
            inline Vector4 operator-(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
            inline Vector4 operator*(const Vector4& v) const { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
            inline Vector4 operator/(const Vector4& v) const { return Vector4(x / v.x, y / v.y, z / v.z, w / v.w); }
            inline void operator+=(const Vector4& v)
            {
                x += v.x;
                y += v.y;
                z += v.z;
                w += v.w;
            }
            inline void operator-=(const Vector4& v)
            {
                x -= v.x;
                y -= v.y;
                z -= v.z;
                w -= v.w;
            }
            inline void operator*=(const Vector4& v)
            {
                x *= v.x;
                y *= v.y;
                z *= v.z;
                w *= v.w;
            }
            inline void operator/=(const Vector4& v)
            {
                x /= v.x;
                y /= v.y;
                z /= v.z;
                w /= v.w;
            }

            inline Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }
            inline bool operator==(const Vector4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
            inline bool operator!=(const Vector4& v) const
            {
                return (v.x == x && v.y == y && v.z == z && v.w == w) ? false : true;
            }
#endif
            float& operator [](size_t i)
            {
                return GetPointer()[i];
            }
        };

        inline Vector4 operator+(float f, const Vector4& v)
        {
            return v + f;
        }
        inline Vector4 operator*(float f, const Vector4& v)
        {
            return v * f;
        }

#ifdef LUMOS_SSE
        inline Vector4 operator-(float f, const Vector4& v)
        {
            return Vector4(_mm_set1_ps(f)) - v;
        }
        inline Vector4 operator/(float f, const Vector4& v)
        {
            return Vector4(_mm_set1_ps(f)) / v;
        }
#else
        inline Vector4 operator-(float f, const Vector4& v)
        {
            return Vector4(f) - v;
        }
        inline Vector4 operator/(float f, const Vector4& v)
        {
            return Vector4(f) / v;
        }
#endif

    }
    typedef Maths::Vector4 Vec4;
}
