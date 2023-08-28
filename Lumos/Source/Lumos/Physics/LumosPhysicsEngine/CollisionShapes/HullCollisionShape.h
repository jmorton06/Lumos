#pragma once

#include "CollisionShape.h"
#include "Hull.h"
#include "Maths/BoundingBox.h"

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>

namespace Lumos
{
    namespace Graphics
    {
        class Mesh;
    }

    class LUMOS_EXPORT HullCollisionShape : public CollisionShape
    {
    public:
        HullCollisionShape();
        ~HullCollisionShape();

        void BuildFromMesh(Graphics::Mesh* mesh);

        // Collision Shape Functionality
        virtual glm::mat3 BuildInverseInertia(float invMass) const override;

        virtual std::vector<glm::vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        virtual std::vector<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;

        virtual void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const override;
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                 const glm::vec3& axis,
                                                 ReferencePolygon& refPolygon) const override;

        virtual void DebugDraw(const RigidBody3D* currentObject) const override;

        // Set Cuboid Dimensions
        void SetHalfWidth(float half_width)
        {
            m_HalfDimensions.x = fabs(half_width);
            m_LocalTransform   = glm::scale(glm::mat4(1.0), m_HalfDimensions);
        }
        void SetHalfHeight(float half_height)
        {
            m_HalfDimensions.y = fabs(half_height);
            m_LocalTransform   = glm::scale(glm::mat4(1.0), m_HalfDimensions);
        }
        void SetHalfDepth(float half_depth)
        {
            m_HalfDimensions.z = fabs(half_depth);
            m_LocalTransform   = glm::scale(glm::mat4(1.0), m_HalfDimensions);
        }

        float GetHalfWidth() const
        {
            return m_HalfDimensions.x;
        }
        float GetHalfHeight() const
        {
            return m_BoundingBox.Size().y;
        }
        float GetHalfDepth() const
        {
            return m_BoundingBox.Size().z;
        }

        glm::vec3 GetHalfDimensions() const
        {
            return m_BoundingBox.Size();
        }
        void SetHalfDimensions(const glm::vec3& dims)
        {
            m_HalfDimensions = dims;
            m_LocalTransform = glm::scale(glm::mat4(1.0), m_HalfDimensions);
        }

        virtual float GetSize() const override
        {
            return m_BoundingBox.Size().x;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_HalfDimensions);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_HalfDimensions);

            m_LocalTransform = glm::scale(glm::mat4(1.0), m_HalfDimensions);
            m_Type           = CollisionShapeType::CollisionHull;
        }

    protected:
        glm::vec3 m_HalfDimensions;
        Maths::BoundingBox m_BoundingBox;

        SharedPtr<Hull> m_Hull;
    };

}
