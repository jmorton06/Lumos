#include "Precompiled.h"
#include "Vector3.h"
#include "Vector4.h"
#include "MathsUtilities.h"
#ifdef LUMOS_SSE_VEC3
#include "SSEUtilities.h"
#endif
namespace Lumos
{
    namespace Maths
    {
        Vector3::Vector3(const Vector2& vec2, float zVal)
        {
#ifdef LUMOS_SSE_VEC3
            m_Value = _mm_set_ps(0, zVal, vec2.y, vec2.x);
#else
            x = vec2.x;
            y = vec2.y;
            z = zVal;
#endif
        }

        Vector3::Vector3(const Vector4& vec4)
        {
#ifdef LUMOS_SSE_VEC3
            m_Value = _mm_set_ps(0, vec4.z, vec4.y, vec4.x);
#else
            x = vec4.x;
            y = vec4.y;
            z = vec4.z;
#endif
        }

        float Vector3::Sqrt(float x)
        {
#ifdef LUMOS_SSE_VEC3
            __m128 v = _mm_set_ss(x);
            v        = _mm_sqrt_ss(v);
            return _mm_cvtss_f32(v);
#else
            return Maths::Sqrt(x);
#endif
        }

        Vector2 Vector3::ToVector2() const
        {
            return Vector2(x, y);
        }

        bool Vector3::IsValid() const
        {
            return !IsNaN() && !IsInf();
        }

        bool Vector3::IsInf() const
        {
            return Maths::IsInf(x) || Maths::IsInf(y) || Maths::IsInf(z);
        }

        bool Vector3::IsNaN() const
        {
            return Maths::IsNaN(x) || Maths::IsNaN(y) || Maths::IsNaN(z);
        }

        bool Vector3::IsZero() const
        {
            return Maths::IsZero(x) || Maths::IsZero(y) || Maths::IsZero(z);
        }
    }
}
