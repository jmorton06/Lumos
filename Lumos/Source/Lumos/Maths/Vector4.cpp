#include "Precompiled.h"
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        Vector4::Vector4(const Vector3& v, float wVal)
        {
#ifdef LUMOS_SSE
            m_Value = _mm_set_ps(wVal, v.z, v.y, v.x);
#else
            x = v.x;
            y = v.y;
            z = v.z;
            w = wVal;
#endif
        }

        Vector4::Vector4(float a, float b, const Vector2& cd)
        {
            x = a;
            y = b;
            z = cd.x;
            w = cd.y;
        }

        Vector4::Vector4(const Vector2& ab, float c, float d)
        {
            x = ab.x;
            y = ab.y;
            z = c;
            w = d;
        }

        Vector4::Vector4(float a, const Vector2& bc, float d)
        {
            x = a;
            y = bc.x;
            z = bc.y;
            w = d;
        }

        Vector4::Vector4(const Vector2& ab, const Vector2& cd)
        {
            x = ab.x;
            y = ab.y;
            z = cd.x;
            w = cd.y;
        }

        Vector2 Vector4::ToVector2() const
        {
            return Vector2(x, y);
        }
        Vector3 Vector4::ToVector3() const
        {
            return Vector3(x, y, z);
        }

        float Vector4::Length() const
        {
#ifdef LUMOS_SSE
            return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m_Value, m_Value, 0xF1)));
#else
            return Maths::Sqrt((x * x) + (y * y) + (z * z) + (w * w));
#endif
        }

        bool Vector4::Equals(const Vector4& rhs) const
        {
            return Maths::Equals(x, rhs.x) && Maths::Equals(y, rhs.y) && Maths::Equals(z, rhs.z) && Maths::Equals(w, rhs.w);
        }    
                
        Vector4 Vector4::Lerp(const Vector4& rhs, float t)
        {
            return Vector4(Maths::Lerp(x, rhs.x, t), Maths::Lerp(y, rhs.y, t), Maths::Lerp(z, rhs.z, t), Maths::Lerp(w, rhs.w, t));
        }  

        bool Vector4::IsValid() const
        {
            return !IsNaN() && !IsInf();
        }

        bool Vector4::IsInf() const
        {
            return Maths::IsInf(x) || Maths::IsInf(y) || Maths::IsInf(z) || Maths::IsInf(w);
        }

        bool Vector4::IsNaN() const
        {
            return Maths::IsNaN(x) || Maths::IsNaN(y) || Maths::IsNaN(z) || Maths::IsNaN(w);
        }
    }
}
