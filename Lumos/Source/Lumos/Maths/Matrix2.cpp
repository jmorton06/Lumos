#include "Precompiled.h"
#include "Maths/Matrix2.h"

namespace Lumos::Maths
{
    const Matrix2 Matrix2::ZERO(
        0.0f, 0.0f,
        0.0f, 0.0f);

    const Matrix2 Matrix2::IDENTITY;

    Matrix2 Matrix2::Inverse() const
    {
        float det = m00_ * m11_ - m01_ * m10_;

        float invDet = 1.0f / det;

        return Matrix2(
                   m11_, -m01_,
                   -m10_, m00_)
            * invDet;
    }
}
