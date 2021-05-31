#pragma once

namespace Lumos
{

    namespace Maths
    {
        class Vector2;
        class Vector3;
    }

    namespace Graphics
    {
        enum class PrimitiveType : int
        {
            Plane = 0,
            Quad = 1,
            Cube = 2,
            Pyramid = 3,
            Sphere = 4,
            Capsule = 5,
            Cylinder = 6,
            Terrain = 7,
            File = 8
        };

        class Material;
        class Mesh;

        LUMOS_EXPORT Mesh* CreatePrimative(PrimitiveType type);

        Mesh* CreateQuad();
        Mesh* CreateScreenQuad();
        Mesh* CreateQuad(float x, float y, float width, float height);
        Mesh* CreateQuad(const Maths::Vector2& position, const Maths::Vector2& size);
        Mesh* CreateCube();
        Mesh* CreatePyramid();
        Mesh* CreateSphere(uint32_t xSegments = 64, uint32_t ySegments = 64);
        Mesh* CreateIcoSphere(uint32_t radius = 1, uint32_t subdivision = 64);
        Mesh* CreateCapsule(float radius = 1.0f, float midHeight = 1.0f, int radialSegments = 64, int rings = 8);
        Mesh* CreatePlane(float width, float height, const Maths::Vector3& normal);
        Mesh* CreateCylinder(float bottomRadius = 1.0f, float topRadius = 1.0f, float height = 1.0f, int radialSegments = 64, int rings = 8);
        Mesh* CreateTerrain();
    }
}
