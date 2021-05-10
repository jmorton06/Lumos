#include "Precompiled.h"
#include "Maths/Quaternion.h"
#include "Maths/Matrix4.h"

namespace Lumos::Maths
{
    const Quaternion Quaternion::IDENTITY;

    void Quaternion::FromAngleAxis(float angle, const Vector3& axis)
    {
        Vector3 normAxis = axis.Normalised();
        angle *= M_DEGTORAD_2;
        float sinAngle = sinf(angle);
        float cosAngle = cosf(angle);

        w = cosAngle;
        x = normAxis.x * sinAngle;
        y = normAxis.y * sinAngle;
        z = normAxis.z * sinAngle;
    }

    void Quaternion::FromEulerAngles(float pitch, float yaw, float roll)
    {
        // Order of rotations: Z first, then X, then Y (mimics typical FPS camera with gimbal lock at top/bottom)
        pitch *= M_DEGTORAD_2;
        yaw *= M_DEGTORAD_2;
        roll *= M_DEGTORAD_2;
        float sinX = sinf(pitch);
        float cosX = cosf(pitch);
        float sinY = sinf(yaw);
        float cosY = cosf(yaw);
        float sinZ = sinf(roll);
        float cosZ = cosf(roll);

        w = cosY * cosX * cosZ + sinY * sinX * sinZ;
        x = cosY * sinX * cosZ + sinY * cosX * sinZ;
        y = sinY * cosX * cosZ - cosY * sinX * sinZ;
        z = cosY * cosX * sinZ - sinY * sinX * cosZ;
    }

    void Quaternion::FromRotationTo(const Vector3& start, const Vector3& end)
    {
        Vector3 normStart = start.Normalised();
        Vector3 normEnd = end.Normalised();
        float d = normStart.DotProduct(normEnd);

        if(d > -1.0f + M_EPSILON)
        {
            Vector3 c = normStart.CrossProduct(normEnd);
            float s = sqrtf((1.0f + d) * 2.0f);
            float invS = 1.0f / s;

            x = c.x * invS;
            y = c.y * invS;
            z = c.z * invS;
            w = 0.5f * s;
        }
        else
        {
            Vector3 axis = Vector3::RIGHT.CrossProduct(normStart);
            if(axis.Length() < M_EPSILON)
                axis = Vector3::UP.CrossProduct(normStart);

            FromAngleAxis(180.f, axis);
        }
    }

    void Quaternion::FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
    {
        Matrix3 matrix(
            xAxis.x, yAxis.x, zAxis.x,
            xAxis.y, yAxis.y, zAxis.y,
            xAxis.z, yAxis.z, zAxis.z);

        FromRotationMatrix(matrix);
    }

    void Quaternion::FromRotationMatrix(const Matrix3& matrix)
    {
        float t = matrix.m00_ + matrix.m11_ + matrix.m22_;

        if(t > 0.0f)
        {
            float invS = 0.5f / sqrtf(1.0f + t);

            x = (matrix.m21_ - matrix.m12_) * invS;
            y = (matrix.m02_ - matrix.m20_) * invS;
            z = (matrix.m10_ - matrix.m01_) * invS;
            w = 0.25f / invS;
        }
        else
        {
            if(matrix.m00_ > matrix.m11_ && matrix.m00_ > matrix.m22_)
            {
                float invS = 0.5f / sqrtf(1.0f + matrix.m00_ - matrix.m11_ - matrix.m22_);

                x = 0.25f / invS;
                y = (matrix.m01_ + matrix.m10_) * invS;
                z = (matrix.m20_ + matrix.m02_) * invS;
                w = (matrix.m21_ - matrix.m12_) * invS;
            }
            else if(matrix.m11_ > matrix.m22_)
            {
                float invS = 0.5f / sqrtf(1.0f + matrix.m11_ - matrix.m00_ - matrix.m22_);

                x = (matrix.m01_ + matrix.m10_) * invS;
                y = 0.25f / invS;
                z = (matrix.m12_ + matrix.m21_) * invS;
                w = (matrix.m02_ - matrix.m20_) * invS;
            }
            else
            {
                float invS = 0.5f / sqrtf(1.0f + matrix.m22_ - matrix.m00_ - matrix.m11_);

                x = (matrix.m02_ + matrix.m20_) * invS;
                y = (matrix.m12_ + matrix.m21_) * invS;
                z = 0.25f / invS;
                w = (matrix.m10_ - matrix.m01_) * invS;
            }
        }
    }

    bool Quaternion::FromLookRotation(const Vector3& direction, const Vector3& up)
    {
        Quaternion ret;
        Vector3 forward = direction.Normalised();

        Vector3 v = forward.CrossProduct(up);
        // If direction & up are parallel and crossproduct becomes zero, use FromRotationTo() fallback
        if(v.LengthSquared() >= M_EPSILON)
        {
            v.Normalise();
            Vector3 up = v.CrossProduct(forward);
            Vector3 right = up.CrossProduct(forward);
            ret.FromAxes(right, up, forward);
        }
        else
            ret.FromRotationTo(Vector3::FORWARD, forward);

        if(!ret.IsNaN())
        {
            (*this) = ret;
            return true;
        }
        else
            return false;
    }

    Vector3 Quaternion::EulerAngles() const
    {
        // Derivation from http://www.geometrictools.com/Documentation/EulerAngles.pdf
        // Order of rotations: Z first, then X, then Y
        float check = 2.0f * (-y * z + w * x);

        if(check < -0.995f)
        {
            return Vector3(
                -90.0f,
                0.0f,
                -atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * M_RADTODEG);
        }
        else if(check > 0.995f)
        {
            return Vector3(
                90.0f,
                0.0f,
                atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * M_RADTODEG);
        }
        else
        {
            return Vector3(
                asinf(check) * M_RADTODEG,
                atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)) * M_RADTODEG,
                atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)) * M_RADTODEG);
        }
    }

    float Quaternion::YawAngle() const
    {
        return EulerAngles().y;
    }

    float Quaternion::PitchAngle() const
    {
        return EulerAngles().x;
    }

    float Quaternion::RollAngle() const
    {
        return EulerAngles().z;
    }

    Lumos::Maths::Vector3 Quaternion::Axis() const
    {
        return Vector3(x, y, z) / sqrt(1.0f - w * w);
    }

    float Quaternion::Angle() const
    {
        return 2 * Acos(w);
    }

    Matrix4 Quaternion::RotationMatrix4() const
    {
        return Matrix4(RotationMatrix());
    }

    Matrix3 Quaternion::RotationMatrix() const
    {
        return Matrix3(
            1.0f - 2.0f * y * y - 2.0f * z * z,
            2.0f * x * y - 2.0f * w * z,
            2.0f * x * z + 2.0f * w * y,
            2.0f * x * y + 2.0f * w * z,
            1.0f - 2.0f * x * x - 2.0f * z * z,
            2.0f * y * z - 2.0f * w * x,
            2.0f * x * z - 2.0f * w * y,
            2.0f * y * z + 2.0f * w * x,
            1.0f - 2.0f * x * x - 2.0f * y * y);
    }

    Quaternion Quaternion::Slerp(const Quaternion& rhs, float t) const
    {
        // Use fast approximation for Emscripten builds
#ifdef __EMSCRIPTEN__
        float angle = DotProduct(rhs);
        float sign = 1.f; // Multiply by a sign of +/-1 to guarantee we rotate the shorter arc.
        if(angle < 0.f)
        {
            angle = -angle;
            sign = -1.f;
        }

        float a;
        float b;
        if(angle < 0.999f) // perform spherical linear interpolation.
        {
            // angle = acos(angle); // After this, angle is in the range pi/2 -> 0 as the original angle variable ranged from 0 -> 1.
            angle = (-0.69813170079773212f * angle * angle - 0.87266462599716477f) * angle + 1.5707963267948966f;
            float ta = t * angle;
            // Manually compute the two sines by using a very rough approximation.
            float ta2 = ta * ta;
            b = ((5.64311797634681035370e-03f * ta2 - 1.55271410633428644799e-01f) * ta2 + 9.87862135574673806965e-01f) * ta;
            a = angle - ta;
            float a2 = a * a;
            a = ((5.64311797634681035370e-03f * a2 - 1.55271410633428644799e-01f) * a2 + 9.87862135574673806965e-01f) * a;
        }
        else // If angle is close to taking the denominator to zero, resort to linear interpolation (and normalization).
        {
            a = 1.f - t;
            b = t;
        }
        // Lerp and reNormalise.
        return (*this * (a * sign) + rhs * b).Normalised();
#else
        // Favor accuracy for native code builds
        float cosAngle = DotProduct(rhs);
        float sign = 1.0f;
        // Enable shortest path rotation
        if(cosAngle < 0.0f)
        {
            cosAngle = -cosAngle;
            sign = -1.0f;
        }

        float angle = acosf(cosAngle);
        float sinAngle = sinf(angle);
        float t1, t2;

        if(sinAngle > 0.001f)
        {
            float invSinAngle = 1.0f / sinAngle;
            t1 = sinf((1.0f - t) * angle) * invSinAngle;
            t2 = sinf(t * angle) * invSinAngle;
        }
        else
        {
            t1 = 1.0f - t;
            t2 = t;
        }

        return *this * t1 + (rhs * sign) * t2;
#endif
    }

    Quaternion Quaternion::Nlerp(const Quaternion& rhs, float t, bool shortestPath) const
    {
        Quaternion result;
        float fCos = DotProduct(rhs);
        if(fCos < 0.0f && shortestPath)
            result = (*this) + (((-rhs) - (*this)) * t);
        else
            result = (*this) + ((rhs - (*this)) * t);
        result.Normalise();
        return result;
    }
}
