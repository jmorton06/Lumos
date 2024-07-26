#include "Precompiled.h"
#include "IVector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        IVector4::IVector4(const Vector3& v, i32 wVal)
        {
            x = (i32)v.x;
            y = (i32)v.y;
            z = (i32)v.z;
            w = wVal;
        }

        IVector4::IVector4(i32 a, i32 b, const Vector2& cd)
        {
            x = a;
            y = b;
            z = (i32)cd.x;
            w = (i32)cd.y;
        }

        IVector4::IVector4(const Vector2& ab, i32 c, i32 d)
        {
            x = (i32)ab.x;
            y = (i32)ab.y;
            z = c;
            w = d;
        }

        IVector4::IVector4(i32 a, const Vector2& bc, i32 d)
        {
            x = a;
            y = (i32)bc.x;
            z = (i32)bc.y;
            w = d;
        }

        IVector4::IVector4(const Vector2& ab, const Vector2& cd)
        {
            x = (i32)ab.x;
            y = (i32)ab.y;
            z = (i32)cd.x;
            w = (i32)cd.y;
        }

        Vector2 IVector4::ToVector2() const
        {
            return Vector2((f32)x, (f32)y);
        }
        Vector3 IVector4::ToVector3() const
        {
            return Vector3((f32)x, (f32)y, (f32)z);
        }

        i32 IVector4::Length() const
        {
            return Maths::Sqrt((x * x) + (y * y) + (z * z) + (w * w));
        }

        bool IVector4::Equals(const IVector4& rhs) const
        {
            return Maths::Equals(x, rhs.x) && Maths::Equals(y, rhs.y) && Maths::Equals(z, rhs.z) && Maths::Equals(w, rhs.w);
        }
    }
}
