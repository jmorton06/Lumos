//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "lmpch.h"

#include "../Math/Frustum.h"
#include "../Math/Polyhedron.h"



namespace Urho3D
{

void Sphere::Define(const Vector3* vertices, unsigned count)
{
    if (!count)
        return;

    Clear();
    Merge(vertices, count);
}

void Sphere::Define(const BoundingBox& box)
{
    const Vector3& min = box.min_;
    const Vector3& max = box.max_;

    Clear();
    Merge(min);
    Merge(Vector3(max.x, min.y, min.z));
    Merge(Vector3(min.x, max.y, min.z));
    Merge(Vector3(max.x, max.y, min.z));
    Merge(Vector3(min.x, min.y, max.z));
    Merge(Vector3(max.x, min.y, max.z));
    Merge(Vector3(min.x, max.y, max.z));
    Merge(max);
}

void Sphere::Define(const Frustum& frustum)
{
    Define(frustum.vertices_, NUM_FRUSTUM_VERTICES);
}

void Sphere::Define(const Polyhedron& poly)
{
    Clear();
    Merge(poly);
}

void Sphere::Merge(const Vector3* vertices, unsigned count)
{
    while (count--)
        Merge(*vertices++);
}

void Sphere::Merge(const BoundingBox& box)
{
    const Vector3& min = box.min_;
    const Vector3& max = box.max_;

    Merge(min);
    Merge(Vector3(max.x, min.y, min.z));
    Merge(Vector3(min.x, max.y, min.z));
    Merge(Vector3(max.x, max.y, min.z));
    Merge(Vector3(min.x, min.y, max.z));
    Merge(Vector3(max.x, min.y, max.z));
    Merge(Vector3(min.x, max.y, max.z));
    Merge(max);
}

void Sphere::Merge(const Frustum& frustum)
{
    const Vector3* vertices = frustum.vertices_;
    Merge(vertices, NUM_FRUSTUM_VERTICES);
}

void Sphere::Merge(const Polyhedron& poly)
{
    for (unsigned i = 0; i < poly.faces_.size(); ++i)
    {
        const std::vector<Vector3>& face = poly.faces_[i];
        if (!face.empty())
            Merge(&face[0], face.size());
    }
}

void Sphere::Merge(const Sphere& sphere)
{
    if (radius_ < 0.0f)
    {
        center_ = sphere.center_;
        radius_ = sphere.radius_;
        return;
    }

    Vector3 offset = sphere.center_ - center_;
    float dist = offset.Length();

    // If sphere fits inside, do nothing
    if (dist + sphere.radius_ < radius_)
        return;

    // If we fit inside the other sphere, become it
    if (dist + radius_ < sphere.radius_)
    {
        center_ = sphere.center_;
        radius_ = sphere.radius_;
    }
    else
    {
        Vector3 NormalizedOffset = offset / dist;

        Vector3 min = center_ - radius_ * NormalizedOffset;
        Vector3 max = sphere.center_ + sphere.radius_ * NormalizedOffset;
        center_ = (min + max) * 0.5f;
        radius_ = (max - center_).Length();
    }
}

Intersection Sphere::IsInside(const BoundingBox& box) const
{
    float radiusSquared = radius_ * radius_;
    float distSquared = 0;
    float temp;
    Vector3 min = box.min_;
    Vector3 max = box.max_;

    if (center_.x < min.x)
    {
        temp = center_.x - min.x;
        distSquared += temp * temp;
    }
    else if (center_.x > max.x)
    {
        temp = center_.x - max.x;
        distSquared += temp * temp;
    }
    if (center_.y < min.y)
    {
        temp = center_.y - min.y;
        distSquared += temp * temp;
    }
    else if (center_.y > max.y)
    {
        temp = center_.y - max.y;
        distSquared += temp * temp;
    }
    if (center_.z < min.z)
    {
        temp = center_.z - min.z;
        distSquared += temp * temp;
    }
    else if (center_.z > max.z)
    {
        temp = center_.z - max.z;
        distSquared += temp * temp;
    }

    if (distSquared >= radiusSquared)
        return OUTSIDE;

    min -= center_;
    max -= center_;

    Vector3 tempVec = min; // - - -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x = max.x; // + - -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y = max.y; // + + -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x = min.x; // - + -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.z = max.z; // - + +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y = min.y; // - - +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x = max.x; // + - +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y = max.y; // + + +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;

    return INSIDE;
}

Intersection Sphere::IsInsideFast(const BoundingBox& box) const
{
    float radiusSquared = radius_ * radius_;
    float distSquared = 0;
    float temp;
    Vector3 min = box.min_;
    Vector3 max = box.max_;

    if (center_.x < min.x)
    {
        temp = center_.x - min.x;
        distSquared += temp * temp;
    }
    else if (center_.x > max.x)
    {
        temp = center_.x - max.x;
        distSquared += temp * temp;
    }
    if (center_.y < min.y)
    {
        temp = center_.y - min.y;
        distSquared += temp * temp;
    }
    else if (center_.y > max.y)
    {
        temp = center_.y - max.y;
        distSquared += temp * temp;
    }
    if (center_.z < min.z)
    {
        temp = center_.z - min.z;
        distSquared += temp * temp;
    }
    else if (center_.z > max.z)
    {
        temp = center_.z - max.z;
        distSquared += temp * temp;
    }

    if (distSquared >= radiusSquared)
        return OUTSIDE;
    else
        return INSIDE;
}

Vector3 Sphere::GetLocalPoint(float theta, float phi) const
{
    return Vector3(
        radius_ * Sin(theta) * Sin(phi),
        radius_ * Cos(phi),
        radius_ * Cos(theta) * Sin(phi)
    );
}

}
