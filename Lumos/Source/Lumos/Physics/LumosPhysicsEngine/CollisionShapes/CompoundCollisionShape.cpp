#include "Precompiled.h"
#include "CompoundCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    CompoundCollisionShape::CompoundCollisionShape()
    {
        m_Type = CollisionShapeType::CollisionCompound;
    }

    CompoundCollisionShape::~CompoundCollisionShape()
    {
    }

    void CompoundCollisionShape::AddChild(SharedPtr<CollisionShape> shape, const Mat4& localTransform)
    {
        CompoundChild child;
        child.Shape          = shape;
        child.LocalTransform = localTransform;
        m_Children.PushBack(child);
    }

    void CompoundCollisionShape::RemoveChild(uint32_t index)
    {
        if(index < m_Children.Size())
        {
            // Swap with last and pop
            if(index < m_Children.Size() - 1)
                m_Children[index] = m_Children.Back();
            m_Children.PopBack();
        }
    }

    void CompoundCollisionShape::ClearChildren()
    {
        m_Children.Clear();
    }

    Mat3 CompoundCollisionShape::BuildInverseInertia(float invMass) const
    {
        if(m_Children.Empty())
            return Mat3(1.0f);

        // Simple approximation: use the largest child's inertia
        // (proper compound inertia requires parallel axis theorem)
        float maxSize     = 0.0f;
        int largestChild  = 0;

        for(uint32_t i = 0; i < m_Children.Size(); i++)
        {
            float childSize = m_Children[i].Shape->GetSize();
            if(childSize > maxSize)
            {
                maxSize      = childSize;
                largestChild = (int)i;
            }
        }

        return m_Children[largestChild].Shape->BuildInverseInertia(invMass);
    }

    float CompoundCollisionShape::GetSize() const
    {
        float maxSize = 0.0f;
        for(const auto& child : m_Children)
        {
            float childSize = child.Shape->GetSize();
            if(childSize > maxSize)
                maxSize = childSize;
        }
        return maxSize;
    }

    void CompoundCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        for(const auto& child : m_Children)
        {
            // Temporarily set child transform and draw
            child.Shape->DebugDraw(currentObject);
        }
    }

    TDArray<Vec3>& CompoundCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        m_CachedAxes.Clear();

        for(auto& child : m_Children)
        {
            auto& childAxes = child.Shape->GetCollisionAxes(currentObject);
            for(const auto& axis : childAxes)
            {
                // Transform axis by child's local transform
                Vec3 transformedAxis = Vec3(child.LocalTransform * Vec4(axis, 0.0f)).Normalised();
                m_CachedAxes.PushBack(transformedAxis);
            }
        }

        return m_CachedAxes;
    }

    TDArray<CollisionEdge>& CompoundCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        m_CachedEdges.Clear();

        for(auto& child : m_Children)
        {
            auto& childEdges = child.Shape->GetEdges(currentObject);
            for(const auto& edge : childEdges)
            {
                CollisionEdge transformedEdge;
                transformedEdge.posA = Vec3(child.LocalTransform * Vec4(edge.posA, 1.0f));
                transformedEdge.posB = Vec3(child.LocalTransform * Vec4(edge.posB, 1.0f));
                m_CachedEdges.PushBack(transformedEdge);
            }
        }

        return m_CachedEdges;
    }

    void CompoundCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        if(m_Children.Empty())
        {
            if(out_min) *out_min = Vec3(0.0f);
            if(out_max) *out_max = Vec3(0.0f);
            return;
        }

        Vec3 globalMin, globalMax;
        bool first = true;

        for(const auto& child : m_Children)
        {
            Vec3 childMin, childMax;
            child.Shape->GetMinMaxVertexOnAxis(currentObject, axis, &childMin, &childMax);

            // Transform by child local transform
            childMin = Vec3(child.LocalTransform * Vec4(childMin, 1.0f));
            childMax = Vec3(child.LocalTransform * Vec4(childMax, 1.0f));

            if(first)
            {
                globalMin = childMin;
                globalMax = childMax;
                first     = false;
            }
            else
            {
                float minProj = Maths::Dot(childMin, axis);
                float maxProj = Maths::Dot(childMax, axis);
                float currMinProj = Maths::Dot(globalMin, axis);
                float currMaxProj = Maths::Dot(globalMax, axis);

                if(minProj < currMinProj)
                    globalMin = childMin;
                if(maxProj > currMaxProj)
                    globalMax = childMax;
            }
        }

        if(out_min) *out_min = globalMin;
        if(out_max) *out_max = globalMax;
    }

    void CompoundCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const
    {
        // Find child most aligned with axis
        float bestDot   = -1e30f;
        int bestChildIdx = -1;

        for(uint32_t i = 0; i < m_Children.Size(); i++)
        {
            const auto& child = m_Children[i];
            Vec3 childMin, childMax;
            child.Shape->GetMinMaxVertexOnAxis(currentObject, axis, &childMin, &childMax);

            float dot = Maths::Dot(childMax, axis);
            if(dot > bestDot)
            {
                bestDot      = dot;
                bestChildIdx = (int)i;
            }
        }

        if(bestChildIdx >= 0)
        {
            m_Children[bestChildIdx].Shape->GetIncidentReferencePolygon(currentObject, axis, refPolygon);
            // Transform polygon faces by child's local transform
            for(uint32_t i = 0; i < refPolygon.FaceCount; i++)
            {
                refPolygon.Faces[i] = Vec3(m_Children[bestChildIdx].LocalTransform * Vec4(refPolygon.Faces[i], 1.0f));
            }
            refPolygon.Normal = Vec3(m_Children[bestChildIdx].LocalTransform * Vec4(refPolygon.Normal, 0.0f)).Normalised();
        }
    }
}
