#include "lmpch.h"

#include "Maths/Frustum.h"
#include "Maths/Polyhedron.h"

#ifdef _MSC_VER
#pragma warning(disable:6293)
#endif

namespace Lumos::Maths
{
    void Polyhedron::Define(const BoundingBox& box)
    {
        Vector3 vertices[8];
        vertices[0] = box.min_;
        vertices[1] = Vector3(box.max_.x, box.min_.y, box.min_.z);
        vertices[2] = Vector3(box.min_.x, box.max_.y, box.min_.z);
        vertices[3] = Vector3(box.max_.x, box.max_.y, box.min_.z);
        vertices[4] = Vector3(box.min_.x, box.min_.y, box.max_.z);
        vertices[5] = Vector3(box.max_.x, box.min_.y, box.max_.z);
        vertices[6] = Vector3(box.min_.x, box.max_.y, box.max_.z);
        vertices[7] = box.max_;

        faces_.resize(6);
        SetFace(0, vertices[3], vertices[7], vertices[5], vertices[1]);
        SetFace(1, vertices[6], vertices[2], vertices[0], vertices[4]);
        SetFace(2, vertices[6], vertices[7], vertices[3], vertices[2]);
        SetFace(3, vertices[1], vertices[5], vertices[4], vertices[0]);
        SetFace(4, vertices[7], vertices[6], vertices[4], vertices[5]);
        SetFace(5, vertices[2], vertices[3], vertices[1], vertices[0]);
    }

    void Polyhedron::Define(const Frustum& frustum)
    {
        const Vector3* vertices = frustum.vertices_;

        faces_.resize(6);
        SetFace(0, vertices[0], vertices[4], vertices[5], vertices[1]);
        SetFace(1, vertices[7], vertices[3], vertices[2], vertices[6]);
        SetFace(2, vertices[7], vertices[4], vertices[0], vertices[3]);
        SetFace(3, vertices[1], vertices[5], vertices[6], vertices[2]);
        SetFace(4, vertices[4], vertices[7], vertices[6], vertices[5]);
        SetFace(5, vertices[3], vertices[0], vertices[1], vertices[2]);
    }

    void Polyhedron::AddFace(const Vector3& v0, const Vector3& v1, const Vector3& v2)
    {
        faces_.resize(faces_.size() + 1);
        std::vector<Vector3>& face = faces_[faces_.size() - 1];
        face.resize(3);
        face[0] = v0;
        face[1] = v1;
        face[2] = v2;
    }

    void Polyhedron::AddFace(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3)
    {
        faces_.resize(faces_.size() + 1);
        std::vector<Vector3>& face = faces_[faces_.size() - 1];
        face.resize(4);
        face[0] = v0;
        face[1] = v1;
        face[2] = v2;
        face[3] = v3;
    }

    void Polyhedron::AddFace(const std::vector<Vector3>& face)
    {
        faces_.push_back(face);
    }

    void Polyhedron::Clip(const Plane& plane)
    {
        clippedVertices_.clear();

        for (unsigned i = 0; i < faces_.size(); ++i)
        {
            std::vector<Vector3>& face = faces_[i];
            Vector3 lastVertex;
            float lastDistance = 0.0f;

            outFace_.clear();

            for (unsigned j = 0; j < face.size(); ++j)
            {
                float distance = plane.Distance(face[j]);
                if (distance >= 0.0f)
                {
                    if (lastDistance < 0.0f)
                    {
                        float t = lastDistance / (lastDistance - distance);
                        Vector3 clippedVertex = lastVertex + t * (face[j] - lastVertex);
                        outFace_.push_back(clippedVertex);
                        clippedVertices_.push_back(clippedVertex);
                    }

                    outFace_.push_back(face[j]);
                }
                else
                {
                    if (lastDistance >= 0.0f && j != 0)
                    {
                        float t = lastDistance / (lastDistance - distance);
                        Vector3 clippedVertex = lastVertex + t * (face[j] - lastVertex);
                        outFace_.push_back(clippedVertex);
                        clippedVertices_.push_back(clippedVertex);
                    }
                }

                lastVertex = face[j];
                lastDistance = distance;
            }

            // Recheck the distances of the last and first vertices and add the final clipped vertex if applicable
            float distance = plane.Distance(face[0]);
            if ((lastDistance < 0.0f && distance >= 0.0f) || (lastDistance >= 0.0f && distance < 0.0f))
            {
                float t = lastDistance / (lastDistance - distance);
                Vector3 clippedVertex = lastVertex + t * (face[0] - lastVertex);
                outFace_.push_back(clippedVertex);
                clippedVertices_.push_back(clippedVertex);
            }

            // Do not keep faces which are less than triangles
            if (outFace_.size() < 3)
                outFace_.clear();

            face = outFace_;
        }

        if (clippedVertices_.size() > 3)
        {
            outFace_.clear();

            // Start with the first vertex
            outFace_.push_back(clippedVertices_.front());    }
    }

    void Polyhedron::Clip(const Frustum& frustum)
    {
        for (const auto& plane : frustum.planes_)
            Clip(plane);
    }

    void Polyhedron::Clip(const BoundingBox& box)
    {
        Vector3 vertices[8];
        vertices[0] = box.min_;
        vertices[1] = Vector3(box.max_.x, box.min_.y, box.min_.z);
        vertices[2] = Vector3(box.min_.x, box.max_.y, box.min_.z);
        vertices[3] = Vector3(box.max_.x, box.max_.y, box.min_.z);
        vertices[4] = Vector3(box.min_.x, box.min_.y, box.max_.z);
        vertices[5] = Vector3(box.max_.x, box.min_.y, box.max_.z);
        vertices[6] = Vector3(box.min_.x, box.max_.y, box.max_.z);
        vertices[7] = box.max_;

        Clip(Plane(vertices[5], vertices[7], vertices[3]));
        Clip(Plane(vertices[0], vertices[2], vertices[6]));
        Clip(Plane(vertices[3], vertices[7], vertices[6]));
        Clip(Plane(vertices[4], vertices[5], vertices[1]));
        Clip(Plane(vertices[4], vertices[6], vertices[7]));
        Clip(Plane(vertices[1], vertices[3], vertices[2]));
    }

    void Polyhedron::Clear()
    {
        faces_.clear();
    }

    void Polyhedron::Transform(const Matrix3& transform)
    {
        for (unsigned i = 0; i < faces_.size(); ++i)
        {
            std::vector<Vector3>& face = faces_[i];
            for (unsigned j = 0; j < face.size(); ++j)
                face[j] = transform * face[j];
        }
    }

    void Polyhedron::Transform(const Matrix3x4& transform)
    {
        for (unsigned i = 0; i < faces_.size(); ++i)
        {
            std::vector<Vector3>& face = faces_[i];
            for (unsigned j = 0; j < face.size(); ++j)
                face[j] = transform * face[j];
        }
    }

    Polyhedron Polyhedron::Transformed(const Matrix3& transform) const
    {
        Polyhedron ret;
        ret.faces_.resize(faces_.size());

        for (unsigned i = 0; i < faces_.size(); ++i)
        {
            const std::vector<Vector3>& face = faces_[i];
            std::vector<Vector3>& newFace = ret.faces_[i];
            newFace.resize(face.size());

            for (unsigned j = 0; j < face.size(); ++j)
                newFace[j] = transform * face[j];
        }

        return ret;
    }

    Polyhedron Polyhedron::Transformed(const Matrix3x4& transform) const
    {
        Polyhedron ret;
        ret.faces_.resize(faces_.size());

        for (unsigned i = 0; i < faces_.size(); ++i)
        {
            const std::vector<Vector3>& face = faces_[i];
            std::vector<Vector3>& newFace = ret.faces_[i];
            newFace.resize(face.size());

            for (unsigned j = 0; j < face.size(); ++j)
                newFace[j] = transform * face[j];
        }

        return ret;
    }

    void Polyhedron::SetFace(unsigned index, const Vector3& v0, const Vector3& v1, const Vector3& v2)
    {
        std::vector<Vector3>& face = faces_[index];
        face.resize(3);
        face[0] = v0;
        face[1] = v1;
        face[2] = v2;
    }

    void Polyhedron::SetFace(unsigned index, const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3)
    {
        std::vector<Vector3>& face = faces_[index];
        face.resize(4);
        face[0] = v0;
        face[1] = v1;
        face[2] = v2;
        face[3] = v3;
    }
}
