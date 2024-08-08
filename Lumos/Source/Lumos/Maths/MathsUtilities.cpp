#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"

namespace Lumos::Maths
{
    /// Check whether an unsigned integer is a power of two.
    bool IsPowerOfTwo(unsigned value)
    {
        return !(value & (value - 1));
    }
    
    /// Round up to next power of two.
    unsigned NextPowerOfTwo(unsigned value)
    {
        // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        --value;
        value |= value >> 1u;
        value |= value >> 2u;
        value |= value >> 4u;
        value |= value >> 8u;
        value |= value >> 16u;
        return ++value;
    }
    
    /// Round up or down to the closest power of two.
    unsigned ClosestPowerOfTwo(unsigned value)
    {
        unsigned next = NextPowerOfTwo(value);
        unsigned prev = next >> (unsigned)1;
        return (value - prev) > (next - value) ? next : prev;
    }
    
    /// Return log base two or the MSB position of the given value.
    unsigned LogBaseTwo(unsigned value)
    {
        // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
        unsigned ret = 0;
        while(value >>= 1) // Unroll for more speed...
            ++ret;
        return ret;
    }
    
    /// Count the number of set bits in a mask.
    unsigned CountSetBits(unsigned value)
    {
        // Brian Kernighan's method
        unsigned count = 0;
        for(count = 0; value; count++)
            value &= value - 1;
        return count;
    }
    
    /// Update a hash with the given 8-bit value using the SDBM algorithm.
    constexpr unsigned SDBMHash(unsigned hash, unsigned char c)
    {
        return c + (hash << 6u) + (hash << 16u) - hash;
    }
    
    /// Convert float to half float
    unsigned short FloatToHalf(float value)
    {
        unsigned inu = FloatToRawIntBits(value);
        unsigned t1  = inu & 0x7fffffffu; // Non-sign bits
        unsigned t2  = inu & 0x80000000u; // Sign bit
        unsigned t3  = inu & 0x7f800000u; // Exponent
        
        t1 >>= 13; // Align mantissa on MSB
        t2 >>= 16; // Shift sign bit into position
        
        t1 -= 0x1c000; // Adjust bias
        
        t1 = (t3 < 0x38800000) ? 0 : t1;      // Flush-to-zero
        t1 = (t3 > 0x47000000) ? 0x7bff : t1; // Clamp-to-max
        t1 = (t3 == 0 ? 0 : t1);              // Denormals-as-zero
        
        t1 |= t2; // Re-insert sign bit
        
        return (unsigned short)t1;
    }
    
    /// Convert half float to float
    float HalfToFloat(unsigned short value)
    {
        unsigned t1 = value & 0x7fffu; // Non-sign bits
        unsigned t2 = value & 0x8000u; // Sign bit
        unsigned t3 = value & 0x7c00u; // Exponent
        
        t1 <<= 13; // Align mantissa on MSB
        t2 <<= 16; // Shift sign bit into position
        
        t1 += 0x38000000; // Adjust bias
        
        t1 = (t3 == 0 ? 0 : t1); // Denormals-as-zero
        
        t1 |= t2; // Re-insert sign bit
        
        float out;
        *((unsigned*)&out) = t1;
        return out;
    }
    
    void SinCos(float angle, float& sin, float& cos)
    {
        float angleRadians = angle * M_DEGTORAD;
#if defined(HAVE_SINCOSF)
        sincosf(angleRadians, &sin, &cos);
#elif defined(HAVE___SINCOSF)
        __sincosf(angleRadians, &sin, &cos);
#else
        sin = sinf(angleRadians);
        cos = cosf(angleRadians);
#endif
    }
    
    uint32_t nChoosek(uint32_t n, uint32_t k)
    {
        if(k > n)
            return 0;
        if(k * 2 > n)
            k = n - k;
        if(k == 0)
            return 1;
        
        uint32_t result = n;
        for(uint32_t i = 2; i <= k; ++i)
        {
            result *= (n - i + 1);
            result /= i;
        }
        return result;
    }
    
    Vec3 ComputeClosestPointOnSegment(const Vec3& segPointA, const Vec3& segPointB, const Vec3& pointC)
    {
        const Vec3 ab = segPointB - segPointA;
        
        float abLengthSquare = Maths::Length2(ab);
        
        // If the segment has almost zero length
        if(abLengthSquare < M_EPSILON)
        {
            // Return one end-point of the segment as the closest point
            return segPointA;
        }
        
        // Project point C onto "AB" line
        float t = Maths::Dot((pointC - segPointA), ab) / abLengthSquare;
        
        // If projected point onto the line is outside the segment, clamp it to the segment
        t = Clamp(t, 0.0f, 1.0f);
        
        // Return the closest point on the segment
        return segPointA + t * ab;
    }
    
    void ClosestPointBetweenTwoSegments(const Vec3& seg1PointA, const Vec3& seg1PointB,
                                        const Vec3& seg2PointA, const Vec3& seg2PointB,
                                        Vec3& closestPointSeg1, Vec3& closestPointSeg2)
    {
        
        const Vec3 d1 = seg1PointB - seg1PointA;
        const Vec3 d2 = seg2PointB - seg2PointA;
        const Vec3 r  = seg1PointA - seg2PointA;
        float a       = Maths::Length2(d1);
        float e       = Maths::Length2(d2);
        float f       = Maths::Dot(d2, r);
        float s, t;
        
        // If both segments degenerate into points
        if(a <= M_EPSILON && e <= M_EPSILON)
        {
            closestPointSeg1 = seg1PointA;
            closestPointSeg2 = seg2PointA;
            return;
        }
        if(a <= M_EPSILON)
        { // If first segment degenerates into a point
            
            s = 0.0f;
            
            // Compute the closest point on second segment
            t = Maths::Clamp(f / e, 0.0f, 1.0f);
        }
        else
        {
            
            float c = Maths::Dot(d1, r);
            
            // If the second segment degenerates into a point
            if(e <= M_EPSILON)
            {
                
                t = 0.0f;
                s = Clamp(-c / a, 0.0f, 1.0f);
            }
            else
            {
                float b     = Maths::Dot(d1, d2);
                float denom = a * e - b * b;
                
                // If the segments are not parallel
                if(denom != 0.0f)
                {
                    
                    // Compute the closest point on line 1 to line 2 and
                    // clamp to first segment.
                    s = Clamp((b * f - c * e) / denom, 0.0f, 1.0f);
                }
                else
                {
                    
                    // Pick an arbitrary point on first segment
                    s = 0.0f;
                }
                
                // Compute the point on line 2 closest to the closest point
                // we have just found
                t = (b * s + f) / e;
                
                // If this closest point is inside second segment (t in [0, 1]), we are done.
                // Otherwise, we clamp the point to the second segment and compute again the
                // closest point on segment 1
                if(t < 0.0f)
                {
                    t = 0.0f;
                    s = Clamp(-c / a, 0.0f, 1.0f);
                }
                else if(t > 1.0f)
                {
                    t = 1.0f;
                    s = Clamp((b - c) / a, 0.0f, 1.0f);
                }
            }
        }
        
        // Compute the closest points on both segments
        closestPointSeg1 = seg1PointA + d1 * s;
        closestPointSeg2 = seg2PointA + d2 * t;
    }
    
    bool AreVectorsParallel(const Vec3& v1, const Vec3& v2)
    {
        return v1.Cross(v2).LengthSquared() < M_EPSILON;
    }
    
    Vec2 WorldToScreen(const Vec3& worldPos, const Mat4& mvp, float width, float height, float winPosX, float winPosY)
    {
        Vec4 trans = mvp * Vec4(worldPos, 1.0f);
        trans *= 0.5f / trans.w;
        trans += Vec4(0.5f, 0.5f, 0.0f, 0.0f);
        trans.y = 1.f - trans.y;
        trans.x *= width;
        trans.y *= height;
        trans.x += winPosX;
        trans.y += winPosY;
        return Vec2(trans.x, trans.y);
    }
    
    void SetScale(Mat4& transform, float scale)
    {
        transform.SetScaling(Vector3(scale));
    }
    
    void SetScale(Mat4& transform, const Vec3& scale)
    {
        transform.SetScaling(scale);
    }
    
    void SetRotation(Mat4& transform, const Vec3& rotation)
    {
        LERROR("ERROR SetRotation");
        // transform.SetRotation(Quaternion(rotation.x, rotation.y, rotation.z));
    }
    
    void SetTranslation(Mat4& transform, const Vec3& translation)
    {
        transform.SetTranslation(translation);
    }
    
    Mat4 ToMat4(const Quat& quat)
    {
        return quat.ToMatrix4();
    }
    
    Vec3 GetScale(const Mat4& transform)
    {
        return transform.Scale();
    }
    
    Vec3 GetRotation(const Mat4& transform)
    {
        return transform.Rotation().ToEuler();
    }
    
    Mat4 Mat4FromTRS(const Vec3& translation, const Vec3& rotation, const Vec3& scale)
    {
        Mat4 transform = Mat4();
        SetScale(transform, scale);
        SetRotation(transform, rotation);
        SetTranslation(transform, translation);
        return transform;
    }
    
    Vec3 Cross(const Vec3& a, const Vec3& b)
    {
        return a.Cross(b);
    }
    
    float Dot(const Vec3& a, const Vec3& b)
    {
        return a.Dot(b);
    }
    
    float Length(const Vec2& vec)
    {
        return vec.Length();
    }
    
    float Length(const Vec3& vec)
    {
        return vec.Length();
    }
    
    float Length2(const Vec3& vec)
    {
        return vec.LengthSquared();
    }
    
    float Distance(const Vec2& a, const Vec2& b)
    {
        return (a - b).Length();
    }
    
    float Distance2(const Vec2& a, const Vec2& b)
    {
        return (a - b).LengthSquared();
    }
    
    float Distance(const Vec3& a, const Vec3& b)
    {
        return (a - b).Length();
    }
    
    float Distance2(const Vec3& a, const Vec3& b)
    {
        return (a - b).LengthSquared();
    }
    
    float Distance(const Vec4& a, const Vec4& b)
    {
        return (a - b).Length();
    }
    
    float Distance2(const Vec4& a, const Vec4& b)
    {
        return (a - b).LengthSquared();
    }
    
    float* ValuePtr(Vec2& vec)
    {
        return &vec.x;
    }
    
    float* ValuePtr(Vec3& vec)
    {
        return &vec.x;
    }
    
    float* ValuePtr(Vec4& vec)
    {
        return &vec.x;
    }
    
    float* ValuePtr(Quat& quat)
    {
        return &quat.x;
    }
    
    float* ValuePtr(Mat3& mat)
    {
        return mat.values;
    }
    
    float* ValuePtr(Mat4& mat)
    {
        return mat.values;
    }
    
    const float* ValuePtr(const Vec2& vec)
    {
        return &vec.x;
    }
    
    const float* ValuePtr(const Vec3& vec)
    {
        return &vec.x;
    }
    
    const float* ValuePtr(const Vec4& vec)
    {
        return &vec.x;
    }
    
    const float* ValuePtr(const Quat& quat)
    {
        return &quat.x;
    }
    
    const float* ValuePtr(const Mat3& mat)
    {
        return mat.values;
    }
    
    const float* ValuePtr(const Mat4& mat)
    {
        return mat.values;
    }
    
    Matrix3 Transpose(const Mat3& mat)
    {
        return Matrix3::Transpose(mat);
    }
    
    Matrix4 Transpose(const Mat4& mat)
    {
        return mat.Transpose();
    }
    
#define ENABLE_MATHS_TEST 0
#if ENABLE_MATHS_TEST
#define GIVEN(x) LINFO(x);
#define WHEN(x) LINFO(x);
#define THEN(x) LINFO(x);
#define AND_THEN(x) LINFO(x);
#define CHECK(x)          \
if(x)                 \
{                     \
LINFO("SUCCESS"); \
}                     \
else                  \
{                     \
LERROR("FAIL");   \
LERROR(#x);       \
ASSERT(false);    \
}
#define REQUIRE(x)        \
if(x)                 \
{                     \
LINFO("SUCCESS"); \
}                     \
else                  \
{                     \
LERROR("FAIL");   \
LERROR(#x);       \
ASSERT(false);    \
}
#define Approx(x) x;
#endif
    
    void TestMaths()
    {
#if ENABLE_MATHS_TEST
        LINFO("Running Maths Tests");
        
        {
            {
                float const R(1.0f);
                float const S(2.0f);
                float const T(3.0f);
                float const U(4.0f);
                Vector4 const O(1.0f, 2.0f, 3.0f, 4.0f);
                
                Vector4 const A(R);
                Vector4 const B(1.0f);
                REQUIRE(A == B);
                
                Vector4 const C(R, S, T, U);
                REQUIRE(C == O);
                
                Vector4 const D(R, 2.0f, 3.0f, 4.0f);
                REQUIRE(D == O);
                
                Vector4 const E(1.0f, S, 3.0f, 4.0f);
                REQUIRE(E == O);
                
                Vector4 const F(R, S, 3.0f, 4.0f);
                REQUIRE(F == O);
                
                Vector4 const G(1.0f, 2.0f, T, 4.0f);
                REQUIRE(G == O);
                
                Vector4 const H(R, 2.0f, T, 4.0f);
                REQUIRE(H == O);
                
                Vector4 const I(1.0f, S, T, 4.0f);
                REQUIRE(I == O);
                
                Vector4 const J(R, S, T, 4.0f);
                REQUIRE(J == O);
                
                Vector4 const K(R, 2.0f, 3.0f, U);
                REQUIRE(K == O);
                
                Vector4 const L(1.0f, S, 3.0f, U);
                REQUIRE(L == O);
                
                Vector4 const M(R, S, 3.0f, U);
                REQUIRE(M == O);
                
                Vector4 const N(1.0f, 2.0f, T, U);
                REQUIRE(N == O);
                
                Vector4 const P(R, 2.0f, T, U);
                REQUIRE(P == O);
                
                Vector4 const Q(1.0f, S, T, U);
                REQUIRE(Q == O);
                
                Vector4 const V(R, S, T, U);
                REQUIRE(V == O);
            }
            
            {
                float const R(1.0f);
                double const S(2.0);
                float const T(3.0);
                double const U(4.0);
                Vector4 const O(1.0f, 2.0, 3.0f, 4.0);
                
                Vector4 const A(R);
                Vector4 const B(1.0);
                REQUIRE(A == B);
                
                Vector4 const C(R, S, T, U);
                REQUIRE(C == O);
                
                Vector4 const D(R, 2.0f, 3.0, 4.0f);
                REQUIRE(D == O);
                
                Vector4 const E(1.0, S, 3.0f, 4.0);
                REQUIRE(E == O);
                
                Vector4 const F(R, S, 3.0, 4.0f);
                REQUIRE(F == O);
                
                Vector4 const G(1.0f, 2.0, T, 4.0);
                REQUIRE(G == O);
                
                Vector4 const H(R, 2.0, T, 4.0);
                REQUIRE(H == O);
                
                Vector4 const I(1.0, S, T, 4.0f);
                REQUIRE(I == O);
                
                Vector4 const J(R, S, T, 4.0f);
                REQUIRE(J == O);
                
                Vector4 const K(R, 2.0f, 3.0, U);
                REQUIRE(K == O);
                
                Vector4 const L(1.0f, S, 3.0, U);
                REQUIRE(L == O);
                
                Vector4 const M(R, S, 3.0, U);
                REQUIRE(M == O);
                
                Vector4 const N(1.0f, 2.0, T, U);
                REQUIRE(N == O);
                
                Vector4 const P(R, 2.0, T, U);
                REQUIRE(P == O);
                
                Vector4 const Q(1.0f, S, T, U);
                REQUIRE(Q == O);
                
                Vector4 const V(R, S, T, U);
                REQUIRE(V == O);
            }
            
            {
                float const v1_0(1.0f);
                float const v1_1(2.0f);
                float const v1_2(3.0f);
                float const v1_3(4.0f);
                
                Vector2 const v2_0(1.0f, 2.0f);
                Vector2 const v2_1(2.0f, 3.0f);
                Vector2 const v2_2(3.0f, 4.0f);
                
                Vector3 const v3_0(1.0f, 2.0f, 3.0f);
                Vector3 const v3_1(2.0f, 3.0f, 4.0f);
                
                Vector4 const O(1.0f, 2.0f, 3.0f, 4.0f);
                
                Vector4 const A(v1_0, v1_1, v2_2);
                REQUIRE(A == O);
                
                Vector4 const B(1.0f, 2.0f, v2_2);
                REQUIRE(B == O);
                
                Vector4 const C(v1_0, 2.0f, v2_2);
                REQUIRE(C == O);
                
                Vector4 const D(1.0f, v1_1, v2_2);
                REQUIRE(D == O);
                
                Vector4 const E(v2_0, v1_2, v1_3);
                REQUIRE(E == O);
                
                Vector4 const F(v2_0, 3.0, v1_3);
                REQUIRE(F == O);
                
                Vector4 const G(v2_0, v1_2, 4.0);
                REQUIRE(G == O);
                
                Vector4 const H(v2_0, 3.0f, 4.0);
                REQUIRE(H == O);
            }
            
            {
                float const v1_0(1.0f);
                float const v1_1(2.0f);
                float const v1_2(3.0f);
                float const v1_3(4.0f);
                
                Vector2 const v2(2.0f, 3.0f);
                
                Vector4 const O(1.0f, 2.0, 3.0f, 4.0);
                
                Vector4 const A(v1_0, v2, v1_3);
                REQUIRE(A == O);
                
                Vector4 const B(v1_0, v2, 4.0);
                REQUIRE(B == O);
                
                Vector4 const C(1.0, v2, v1_3);
                REQUIRE(C == O);
                
                Vector4 const D(1.0f, v2, 4.0);
                REQUIRE(D == O);
                
                Vector4 const E(1.0, v2, 4.0f);
                REQUIRE(E == O);
            }
            
            {
                Vector4 const A(1.0f, 2.0f, 3.0f, 4.0f);
                Vector4 const B(4.0f, 5.0f, 6.0f, 7.0f);
                
                Vector4 const C = A + B;
                Vector4 const D = B - A;
                Vector4 const E = A * B;
                Vector4 const F = B / A;
                Vector4 const G = A + 1.0f;
                Vector4 const H = B - 1.0f;
                Vector4 const I = A * 2.0f;
                Vector4 const J = B / 2.0f;
                Vector4 const K = 1.0f + A;
                Vector4 const L = 1.0f - B;
                Vector4 const M = 2.0f * A;
                Vector4 const N = 2.0f / B;
                
                REQUIRE(C == Vector4(5, 7, 9, 11));
                REQUIRE(D == Vector4(3, 3, 3, 3));
                REQUIRE(E == Vector4(4, 10, 18, 28));
                REQUIRE(F == Vector4(4, 2.5, 2, 7.0f / 4.0f));
                REQUIRE(G == Vector4(2, 3, 4, 5));
                REQUIRE(H == Vector4(3, 4, 5, 6));
                REQUIRE(I == Vector4(2, 4, 6, 8));
                REQUIRE(J == Vector4(2, 2.5, 3, 3.5));
                REQUIRE(K == Vector4(2, 3, 4, 5));
                REQUIRE(L == Vector4(-3, -4, -5, -6));
                REQUIRE(M == Vector4(2, 4, 6, 8));
                REQUIRE(N == Vector4(0.5f, 2.0f / 5.0f, 2.0f / 6.0f, 2.0f / 7.0f));
            }
        }
        
        GIVEN("Default quaternion")
        {
            WHEN("We convert to a Matrix4")
            {
                Maths::Quaternion q;
                Matrix4 m = q.ToMatrix4();
                THEN("It should be an identity Matrix")
                {
                    REQUIRE(m == Matrix4());
                }
            }
        }
        
        {
            Maths::Quaternion firstQuaternion = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), 180.f);
            Maths::Quaternion secondQuaternion(1.f, 0.f, 0.f, 0.f);
            secondQuaternion.GenerateW();
            
            {
                REQUIRE(Maths::Equals(firstQuaternion, secondQuaternion));
                REQUIRE(Maths::Equals(firstQuaternion, secondQuaternion.Normalised()));
                REQUIRE(Maths::Equals(firstQuaternion.Conjugate(), secondQuaternion.Inverse()));
                REQUIRE(Maths::Equals(firstQuaternion.Dot(secondQuaternion), 1.f));
            }
        }
        
        GIVEN("The four unit quaternions")
        {
            Maths::Quaternion w(0.f, 0.f, 0.f, 1.f);
            Maths::Quaternion x(1.f, 0.f, 0.f, 0.f);
            Maths::Quaternion y(0.f, 1.f, 0.f, 0.f);
            Maths::Quaternion z(0.f, 0.f, 1.f, 0.f);
            
            Maths::Quaternion xyzw = x * y * z * w;
            
            WHEN("We ask for the norm")
            {
                THEN("They are all equal to 1")
                {
                    REQUIRE(Maths::Equals(w.Magnitude(), 1.0f));
                    REQUIRE(Maths::Equals(x.Magnitude(), 1.0f));
                    REQUIRE(Maths::Equals(y.Magnitude(), 1.0f));
                    REQUIRE(Maths::Equals(z.Magnitude(), 1.0f));
                    REQUIRE(Maths::Equals(xyzw.Magnitude(), 1.0f));
                }
            }
            
            WHEN("We multiply them")
            {
                THEN("Results should follow")
                {
                    Maths::Quaternion oppositeOfW(0.f, 0.f, 0.f, -1.f);
                    Maths::Quaternion oppositeOfX = x.Conjugate();
                    Maths::Quaternion oppositeOfY = y.Conjugate();
                    Maths::Quaternion oppositeOfZ = z.Conjugate();
                    
                    REQUIRE(Maths::Equals((x * x), (oppositeOfW)));
                    REQUIRE(Maths::Equals((y * y), (oppositeOfW)));
                    REQUIRE(Maths::Equals((z * z), (oppositeOfW)));
                    REQUIRE(Maths::Equals((x * y * z), (oppositeOfW)));
                    
                    REQUIRE(Maths::Equals((x * y), z));
                    REQUIRE(Maths::Equals((y * x), oppositeOfZ));
                    REQUIRE(Maths::Equals((y * z), x));
                    REQUIRE(Maths::Equals((z * y), oppositeOfX));
                    REQUIRE(Maths::Equals((z * x), y));
                    REQUIRE(Maths::Equals((x * z), oppositeOfY));
                }
            }
        }
        
        GIVEN("Two different quaternions (10, (1, 0, 0) and (20, (1, 0, 0))")
        {
            Maths::Quaternion x10 = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), (10.f));
            Maths::Quaternion x20 = x10 * x10;
            
            Maths::Quaternion x30a = x10 * x20;
            Maths::Quaternion x30b = x20 * x10;
            
            WHEN("We multiply them")
            {
                THEN("These results are expected")
                {
                    REQUIRE(Maths::Equals(x20, Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), (20.f))));
                    REQUIRE(Maths::Equals(x30a, x30b));
                }
            }
            
            WHEN("Convert euler to quaternion")
            {
                Maths::Quaternion X45(Vector3((45.f), 0.f, 0.f));
                Maths::Quaternion Y45(Vector3(0.f, (45.f), 0.f));
                Maths::Quaternion Z45(Vector3(0.f, 0.f, (45.f)));
                
                THEN("They must be equal")
                {
                    REQUIRE(Maths::Equals(X45, Maths::Quaternion(0.38268346f, 0.f, 0.f, 0.9238795f)));
                    REQUIRE(Maths::Equals(Y45, Maths::Quaternion(0.f, 0.38268346f, 0.f, 0.9238795f)));
                    REQUIRE(Maths::Equals(Z45, Maths::Quaternion(0.f, 0.f, 0.38268346f, 0.9238795f)));
                }
            }
            
            WHEN("We convert to euler angles and then to quaternions")
            {
                THEN("These results are expected")
                {
                    REQUIRE(Maths::Equals(x30a.ToEuler(), x30b.ToEuler()));
                    REQUIRE(Maths::Equals(Maths::Quaternion(x30a.ToEuler()), Maths::Quaternion(x30b.ToEuler())));
                    
                    Maths::Quaternion tmp(1.f, 1.f, 0.f, 0.f);
                    tmp.Normalise();
                    REQUIRE(Maths::Equals(tmp, Maths::Quaternion(tmp.ToEuler())));
                }
            }
            
            WHEN("We slerp")
            {
                THEN("The half of 10 and 30 is 20")
                {
                    Maths::Quaternion slerpx10x30a = Maths::Quaternion::Slerp(x10, x30a, 0.5f);
                    /*		REQUIRE(slerpx10x30a.w == Approx(x20.w));
                                    REQUIRE(slerpx10x30a.x == Approx(x20.x));
                                    REQUIRE(slerpx10x30a.y == Approx(x20.y));
                                    REQUIRE(slerpx10x30a.z == Approx(x20.z));*/
                    Maths::Quaternion slerpx10x30b = Maths::Quaternion::Slerp(x10, x30b, 0.5f);
                    /*		REQUIRE(slerpx10x30b.w == Approx(x20.w));
                                    REQUIRE(slerpx10x30b.x == Approx(x20.x));
                                    REQUIRE(slerpx10x30b.y == Approx(x20.y));
                                    REQUIRE(slerpx10x30b.z == Approx(x20.z));*/
                    REQUIRE(Maths::Equals(Maths::Quaternion::Slerp(x10, x30a, 0.f), x10));
                    REQUIRE(Maths::Equals(Maths::Quaternion::Slerp(x10, x30a, 1.f), x30a));
                }
                
                AND_THEN("The half of 45 is 22.5")
                {
                    Maths::Quaternion quaternionA = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), (0.f));
                    Maths::Quaternion quaternionB = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), (45.f));
                    Maths::Quaternion quaternionC = Maths::Quaternion::Slerp(quaternionA, quaternionB, 0.5f);
                    
                    Maths::Quaternion unitZ225 = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), (22.5f));
                    REQUIRE(Maths::Equals(quaternionC, unitZ225));
                }
            }
            
            WHEN("We get the rotation between two vectors")
            {
                THEN("The rotation in right-handed is 90 degree on z")
                {
                    Maths::Quaternion rotationBetweenXY = Maths::Quaternion::FromVectors(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
                    Maths::Quaternion rotation90Z       = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, -1.0f), (-90.f));
                    rotationBetweenXY.Normalise();
                    rotation90Z.Normalise();
                    REQUIRE(Maths::Equals(rotation90Z, rotationBetweenXY));
                }
                
                THEN("The rotation in right-handed is 90 degree on y")
                {
                    Maths::Quaternion rotationBetweenXZ = Maths::Quaternion::FromVectors(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
                    Maths::Quaternion rotation90Y       = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), (90.f));
                    REQUIRE(Maths::Equals(rotation90Y, rotationBetweenXZ));
                }
                
                THEN("The rotation in right-handed is 90 degree on x")
                {
                    Maths::Quaternion rotationBetweenYZ = Maths::Quaternion::FromVectors(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
                    Maths::Quaternion rotation90X       = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), -(90.f));
                    REQUIRE(Maths::Equals(rotation90X, rotationBetweenYZ));
                }
                
                THEN("The rotation in right-handed is 90 degree on y with non-unit vectors")
                {
                    Vector3 origin(1.f, 1.f, 0.f);
                    Vector3 extremity(-1.f, 1.f, 0.f);
                    Maths::Quaternion rotation = Maths::Quaternion::FromVectors(origin, extremity);
                    
                    Maths::Quaternion::RotatePointByQuaternion(rotation, origin);
                    REQUIRE(Maths::Equals(origin, extremity));
                }
            }
        }
        
        GIVEN("Different angles")
        {
            Maths::Quaternion rotation90X(0.707f, 0.f, 0.f, 0.707f);
            Maths::Quaternion rotation90Y(0.f, 0.707f, 0.f, 0.707f);
            Maths::Quaternion rotation90Z(0.f, 0.f, 0.707f, 0.707f);
            
            Maths::Quaternion rotation180X(1.f, 0.f, 0.f, 0.f);
            Maths::Quaternion rotation180Y(0.f, 1.f, 0.f, 0.f);
            Maths::Quaternion rotation180Z(0.f, 0.f, 1.f, 0.f);
            
            Maths::Quaternion rotation270X(0.707f, 0.f, 0.f, -0.707f);
            Maths::Quaternion rotation270Y(0.f, 0.707f, 0.f, -0.707f);
            Maths::Quaternion rotation270Z(0.f, 0.f, 0.707f, -0.707f);
            
            Maths::Quaternion special(0.006f, 0.006f, 0.707f, 0.707f);
            
            WHEN("We convert them to euler angles")
            {
                THEN("Those are equal to")
                {
                    CHECK(Maths::Equals(rotation90X.ToEuler().x, (90.f), 1.0f));
                    CHECK(Maths::Equals(rotation90Y.ToEuler().y, (90.f), 2.0f));
                    CHECK(Maths::Equals(rotation90Z.ToEuler().z, (90.f), 1.0f));
                    
                    CHECK(Maths::Equals(rotation180X.ToEuler(), (Vector3(180.f, 0.f, 0.f))));
                    CHECK(Maths::Equals(rotation180Y.ToEuler(), (Vector3(180.f, 0.f, 180.f))));
                    CHECK(Maths::Equals(rotation180Z.ToEuler(), (Vector3(0.f, 0.f, 180.f))));
                    
                    CHECK(Maths::Equals(rotation270X.ToEuler().x, (-90.f), 1.0f));
                    CHECK(Maths::Equals(rotation270Y.ToEuler().y, (-90.f), 2.0f));
                    CHECK(Maths::Equals(rotation270Z.ToEuler().z, (-90.f), 1.0f));
                    
                    CHECK(Maths::Equals(special.ToEuler().x, (0.f), 1.0f));
                    CHECK(Maths::Equals(special.ToEuler().y, (1.f), 2.0f));
                    CHECK(Maths::Equals(special.ToEuler().z, (90.f), 1.0f));
                }
            }
        }
        
        {
            Matrix3 a, b, c;
            a.RotationX(Maths::M_PI / 2.0f * Maths::M_RADTODEG);
            b.RotationZ(50);
            c = a * b;
            
            Vector3 u = Vector3(1.0f);
            Vector3 v = c * u;
            
            Maths::Quaternion q = Maths::Quaternion::FromMatrix(c);
            Matrix3 m           = q.ToMatrix3();
            
            Vector3 w = m * u;
            
            REQUIRE(v == w);
        }
        
        {
            Matrix4 a, b, c;
            a.RotationY(83);
            b.RotationX(122);
            c = a * b;
            
            Vector4 u = Vector4(1.0f);
            Vector4 v = c * u;
            
            Maths::Quaternion q = Maths::Quaternion::FromMatrix(c);
            Matrix4 m           = q.ToMatrix4();
            
            Vector4 w = m * u;
            
            REQUIRE(w == v);
        }
        
        {
            Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), Maths::M_PI * Maths::M_RADTODEG);
            
            Matrix4 m = q.ToMatrix4();
            
            Vector4 v = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
            v         = m * v;
            
            REQUIRE(v.Equals(Vector4(0.0f, -1.0f, 0.0f, 1.0f)));
        }
        
        {
            Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), Maths::M_PI * Maths::M_RADTODEG);
            
            Matrix4 m = q.ToMatrix4();
            
            Vector4 v = Vector4(0.0f, 0.0f, -1.0f, 1.0f); // Forward
            v         = m * v;
            
            REQUIRE(v.Equals(Vector4(0.0f, 0.0f, 1.0f, 1.0f)));
        }
        
        {
            Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, -1.0f), Maths::M_PI * Maths::M_RADTODEG);
            
            Matrix4 m = q.ToMatrix4();
            
            Vector4 v = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            v         = m * v;
            
            REQUIRE(v.Equals(Vector4(-1.0f, 0.0f, 0.0f, 1.0f)));
        }
        
        {
            Matrix3 m           = Matrix3::RotationX(Maths::M_PI / 2.0f * Maths::M_RADTODEG);
            Maths::Quaternion q = Maths::Quaternion::FromMatrix(m);
            
            Vector3 u = Vector3(1.0f);
            Vector3 v = m * u;
            
            Matrix3 n = q.ToMatrix3();
            
            Vector3 w = n * u;
            
            REQUIRE(Maths::Equals(v, w));
        }
        
        {
            Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), -Maths::M_PI / 2.0f * Maths::M_RADTODEG);
            Maths::Quaternion r = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), -40.0f);
            Maths::Quaternion t = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, -1.0f), -310.0f);
            
            Maths::Quaternion k = q * r * t;
            
            Vector4 v = Vector4(1.0f);
            Matrix4 m = k.ToMatrix4();
            v         = m * v;
            
            REQUIRE(v.Equals(Vector4(0.436440f, 1.671624f, 0.123257f, 1.0f)));
        }
        
#endif
    }
    
    float SineOut(float time)
    {
        return Maths::Sin(time * 90.0f);
    }
    float SineIn(float time)
    {
        return -1.0f * Maths::Cos(time * 90.0f) + 1.0f;
    }
    float SineInOut(float time)
    {
        return -0.5f * (Maths::Cos(180.0f * time) - 1.0f);
    }
    
    float ExponentialOut(float time)
    {
        return time == 1.0f ? 1.0f : -Maths::Pow<float>(2.0f, -10.0f * time / 1.0f) + 1.0f;
    }
    
    float ExponentialIn(float time)
    {
        return time == 0.0f ? 0.0f : Pow(2.0f, 10.0f * (time / 1.0f - 1.0f)) - 1.0f * 0.001f;
    }
    
    float ExponentialInOut(float time)
    {
        time /= 0.5f;
        if(time < 1)
            return 0.5f * (float)Pow(2.0f, 10.0f * (time - 1.0f));
        return 0.5f * (-(float)Pow(2.0f, -10.0f * (time - 1.0f)) + 2.0f);
    }
    
    float ElasticIn(float time, float period)
    {
        if(time == 0 || time == 1)
            return time;
        
        auto s = period / 4.0f;
        time   = time - 1;
        return -(Maths::Pow<float>(2, 10 * time) * Sin((time - s) * 180.0f * 2.0f / period));
    }
    
    float ElasticOut(float time, float period)
    {
        if(time == 0 || time == 1)
            return time;
        
        auto s = period * 0.25f;
        return (Maths::Pow<float>(2, -10 * time) * Maths::Sin((time - s) * 180.0f * 2.0f / period) + 1);
    }
    
    float ElasticInOut(float time, float period)
    {
        if(time == 0 || time == 1)
            return time;
        
        time = time * 2;
        if(period == 0)
            period = 0.3f * 1.5f;
        
        auto s = period / 4.0f;
        
        time = time - 1;
        if(time < 0)
            return (float)(-0.5f * Maths::Pow<float>(2, 10 * time) * Maths::Sin((time - s) * 360.0f / period));
        return (float)(Maths::Pow<float>(2, -10 * time) * Maths::Sin((time - s) * 360.0f / period) * 0.5f + 1);
    }
    
    bool AnimateToTarget(float* value, float target, float delta_t, float rate)
    {
        *value += (target - *value) * (1.0f - Pow(2.0f, -rate * delta_t));
        if(Equals(*value, target))
        {
            *value = target;
            return true; // reached
        }
        
        return false;
    }
    
    void Print(const Vector3& vec)
    {
        LINFO("%.2f, %.2f, %.2f", vec.x, vec.y, vec.z);
    }
    
    void Print(const Vector4& vec)
    {
        LINFO("%.2f, %.2f, %.2f, %.2f", vec.x, vec.y, vec.z, vec.w);
    }
    
    void Print(const Quaternion& vec)
    {
        LINFO("%.2f, %.2f, %.2f, %.2f", vec.x, vec.y, vec.z, vec.w);
    }
    
    void Print(const Matrix4& mat)
    {
        LINFO("------------------------");
        LINFO("(%.2f, %.2f, %.2f, %.2f)", mat[0], mat[4], mat[8], mat[12]);
        LINFO("(%.2f, %.2f, %.2f, %.2f)", mat[1], mat[5], mat[9], mat[13]);
        LINFO("(%.2f, %.2f, %.2f, %.2f)", mat[2], mat[6], mat[10], mat[14]);
        LINFO("(%.2f, %.2f, %.2f, %.2f)", mat[3], mat[7], mat[11], mat[15]);
        LINFO("------------------------");
    }
}
