#pragma once

#include "CollisionShape.h"
#include "Hull.h"

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>

namespace Lumos
{

    class LUMOS_EXPORT PyramidCollisionShape : public CollisionShape
    {
    public:
        PyramidCollisionShape();
        PyramidCollisionShape(const glm::vec3& halfdims);
        ~PyramidCollisionShape();

        // Collision Shape Functionality
        virtual glm::mat3 BuildInverseInertia(float invMass) const override;

        virtual std::vector<glm::vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual std::vector<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                 const glm::vec3& axis,
                                                 ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        const glm::vec3& GetHalfDimensions() const
        {
            return m_PyramidHalfDimensions;
        }
        void SetHalfDimensions(const glm::vec3& dims)
        {
            m_PyramidHalfDimensions = dims;

            m_LocalTransform = glm::scale(glm::mat4(1.0), m_PyramidHalfDimensions);
            m_Type           = CollisionShapeType::CollisionPyramid;

            glm::vec3 m_Points[5] = {
                m_LocalTransform * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)
            };

            m_Normals[0] = glm::normalize(glm::cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]));
            m_Normals[1] = glm::normalize(glm::cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]));
            m_Normals[2] = glm::normalize(glm::cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]));
            m_Normals[3] = glm::normalize(glm::cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]));
            m_Normals[4] = glm::vec3(0.0f, -1.0f, 0.0f);

            if(m_PyramidHull->GetNumVertices() == 0)
            {
                ConstructPyramidHull();
            }
        }

        float GetSize() const override
        {
            return m_PyramidHalfDimensions.x;
        };

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_PyramidHalfDimensions);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_PyramidHalfDimensions);

            m_LocalTransform = glm::scale(glm::mat4(1.0), m_PyramidHalfDimensions);
            m_Type           = CollisionShapeType::CollisionPyramid;

            glm::vec3 m_Points[5] = {
                m_LocalTransform * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
                m_LocalTransform * glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
                m_LocalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)
            };

            m_Normals[0] = glm::normalize(glm::cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]));
            m_Normals[1] = glm::normalize(glm::cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]));
            m_Normals[2] = glm::normalize(glm::cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]));
            m_Normals[3] = glm::normalize(glm::cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]));
            m_Normals[4] = glm::vec3(0.0f, -1.0f, 0.0f);

            if(m_PyramidHull->GetNumVertices() == 0)
            {
                ConstructPyramidHull();
            }
        }

    protected:
        // Constructs the static cube hull
        static void ConstructPyramidHull();

    protected:
        glm::vec3 m_PyramidHalfDimensions;
        glm::vec3 m_Normals[5];

        static UniquePtr<Hull> m_PyramidHull;
    };
}
