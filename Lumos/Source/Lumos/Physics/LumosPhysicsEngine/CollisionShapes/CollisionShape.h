#pragma once

#include "Maths/Maths.h"
#include <list>
#include <vector>

namespace Lumos
{
    class RigidBody3D;

    struct ReferencePolygon
    {
        glm::vec3 Faces[8];
        Plane AdjacentPlanes[8];
        glm::vec3 Normal;
        uint32_t FaceCount = 0;
        uint32_t PlaneCount = 0;
    };

    struct LUMOS_EXPORT CollisionEdge
    {
        CollisionEdge(const glm::vec3& a = glm::vec3(0.0f), const glm::vec3& b = glm::vec3(0.0f))
            : posA(a)
            , posB(b)
        {
        }

        glm::vec3 posA;
        glm::vec3 posB;
    };

    enum CollisionShapeType : unsigned int
    {
        CollisionCuboid = 1,
        CollisionSphere = 2,
        CollisionPyramid = 4,
        CollisionCapsule = 8,
        CollisionHull = 16,
        CollisionShapeTypeMax = 32
    };

    class LUMOS_EXPORT CollisionShape
    {
    public:
        CollisionShape()
            : m_Type()
        {
            m_LocalTransform = glm::mat4(1.0f);
        }
        virtual ~CollisionShape()
        {
        }

        // Constructs an inverse inertia matrix of the given collision volume. This is the equivilant of the inverse mass of an object for rotation,
        //   a good source for non-inverse inertia matricies can be found here: https://en.wikipedia.org/wiki/List_of_moments_of_inertia
        virtual glm::mat3 BuildInverseInertia(float invMass) const = 0;
        virtual float GetSize() const = 0;

        // Draws this collision shape to the debug renderer
        virtual void DebugDraw(const RigidBody3D* currentObject) const = 0;

        //<----- USED BY COLLISION DETECTION ----->
        // Get all possible collision axes
        //	- This is a list of all the face normals ignoring any duplicates and parallel vectors.
        virtual std::vector<glm::vec3>& GetCollisionAxes(const RigidBody3D* currentObject) = 0;

        // Get all shape Edges
        //	- Returns a list of all edges AB that form the convex hull of the collision shape. These are
        //    used to check edge/edge collisions aswell as finding the closest point to a sphere. */
        virtual std::vector<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) = 0;

        // Get the min/max vertices along a given axis
        virtual void GetMinMaxVertexOnAxis(
            const RigidBody3D* currentObject,
            const glm::vec3& axis,
            glm::vec3* out_min,
            glm::vec3* out_max) const = 0;

        // Get all data needed to build manifold
        //	- Computes the face that is closest to parallel to that of the given axis,
        //    returning the face (as a list of vertices), face normal and the planes
        //    of all adjacent faces in order to clip against.
        virtual void GetIncidentReferencePolygon(const RigidBody3D* currentObject,
            const glm::vec3& axis,
            ReferencePolygon& refPolygon) const = 0;

        void SetLocalTransform(const glm::mat4& transform)
        {
            m_LocalTransform = transform;
        }

        inline CollisionShapeType GetType() const
        {
            return m_Type;
        }

        template <class Archive>
        void load(Archive& archive)
        {
            LUMOS_LOG_ERROR("Loading abstract CollisionShape");
        }

        template <class Archive>
        void save(Archive& archive) const
        {
            LUMOS_LOG_ERROR("Serialising abstract CollisionShape");
        }

    protected:
        CollisionShapeType m_Type;
        glm::mat4 m_LocalTransform;
        std::vector<CollisionEdge> m_Edges;
        std::vector<glm::vec3> m_Axes;
    };
}
