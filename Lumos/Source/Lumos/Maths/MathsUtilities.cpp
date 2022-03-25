#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include <glm/vec3.hpp>
#include <glm/gtx/norm.hpp>
namespace Lumos::Maths
{
    void SinCos(float angle, float &sin, float &cos)
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
        if (k > n)
            return 0;
        if (k * 2 > n)
            k = n - k;
        if (k == 0)
            return 1;

        uint32_t result = n;
        for (uint32_t i = 2; i <= k; ++i)
        {
            result *= (n - i + 1);
            result /= i;
        }
        return result;
    }

    glm::vec3 ComputeClosestPointOnSegment(const glm::vec3 &segPointA, const glm::vec3 &segPointB, const glm::vec3 &pointC)
    {
        const glm::vec3 ab = segPointB - segPointA;

        float abLengthSquare = glm::length2(ab);

        // If the segment has almost zero length
        if (abLengthSquare < M_EPSILON)
        {
            // Return one end-point of the segment as the closest point
            return segPointA;
        }

        // Project point C onto "AB" line
        float t = glm::dot((pointC - segPointA), ab) / abLengthSquare;

        // If projected point onto the line is outside the segment, clamp it to the segment
        t = Clamp(t, 0.0f, 1.0f);

        // Return the closest point on the segment
        return segPointA + t * ab;
    }

    void ClosestPointBetweenTwoSegments(const glm::vec3 &seg1PointA, const glm::vec3 &seg1PointB,
                                               const glm::vec3 &seg2PointA, const glm::vec3 &seg2PointB,
                                               glm::vec3 &closestPointSeg1, glm::vec3 &closestPointSeg2)
    {

        const glm::vec3 d1 = seg1PointB - seg1PointA;
        const glm::vec3 d2 = seg2PointB - seg2PointA;
        const glm::vec3 r = seg1PointA - seg2PointA;
        float a = glm::length2(d1);
        float e = glm::length2(d2);
        float f = glm::dot(d2, r);
        float s, t;

        // If both segments degenerate into points
        if (a <= M_EPSILON && e <= M_EPSILON)
        {
            closestPointSeg1 = seg1PointA;
            closestPointSeg2 = seg2PointA;
            return;
        }
        if (a <= M_EPSILON)
        { // If first segment degenerates into a point

            s = 0.0f;

            // Compute the closest point on second segment
            t = Maths::Clamp(f / e, 0.0f, 1.0f);
        }
        else
        {

            float c = glm::dot(d1, r);

            // If the second segment degenerates into a point
            if (e <= M_EPSILON)
            {

                t = 0.0f;
                s = Clamp(-c / a, 0.0f, 1.0f);
            }
            else
            {
                float b = glm::dot(d1, d2);
                float denom = a * e - b * b;

                // If the segments are not parallel
                if (denom != 0.0f)
                {

                    // Compute the closest point on line 1 to line 2 and
                    // clamp to first segment.
                    s = Clamp((b * f - c * e) / denom, 0.0f,1.0f);
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
                if (t < 0.0f)
                {
                    t = 0.0f;
                    s = Clamp(-c / a, 0.0f, 1.0f);
                }
                else if (t > 1.0f)
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

    bool AreVectorsParallel(const glm::vec3 &v1, const glm::vec3 &v2)
    {
        return glm::length2(glm::cross(v1, v2)) < M_EPSILON;
    }
}
