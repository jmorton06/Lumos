#pragma once
#include "Maths/Vector3.h"
namespace Lumos
{
    namespace Graphics
    {
        enum class PrimitiveType : int
        {
            Plane    = 0,
            Quad     = 1,
            Cube     = 2,
            Pyramid  = 3,
            Sphere   = 4,
            Capsule  = 5,
            Cylinder = 6,
            Terrain  = 7,
            File     = 8,
            None     = 9
        };

        class Material;
        class Mesh;

        LUMOS_EXPORT Mesh* CreatePrimative(PrimitiveType type);

        Mesh* CreateQuad();
        Mesh* CreateQuad(float x, float y, float width, float height);
        Mesh* CreateQuad(const Vec2& position, const Vec2& size);
        Mesh* CreateCube();
        Mesh* CreatePyramid();
        Mesh* CreateSphere(uint32_t xSegments = 32, uint32_t ySegments = 32);
        Mesh* CreateCapsule(float radius = 0.5f, float midHeight = 2.0f, int radialSegments = 64, int rings = 8);
        Mesh* CreatePlane(float width, float height, const Vec3& normal);
        Mesh* CreateCylinder(float bottomRadius = 0.5f, float topRadius = 0.5f, float height = 1.0f, int radialSegments = 64, int rings = 8);
        Mesh* CreateTerrain();
    }
}
