#pragma once

#include "JM.h"

namespace jm
{
    class Mesh;
    class Material;

    namespace maths
    {
        class Vector2;
        class Vector3;
    }

    namespace MeshFactory
    {
        JM_EXPORT Mesh *CreateQuad();
        JM_EXPORT Mesh *CreateQuad(float x, float y, float width, float height, std::shared_ptr<Material> material);
        JM_EXPORT Mesh *CreateQuad(const maths::Vector2 &position, const maths::Vector2 &size, std::shared_ptr<Material> material);
        JM_EXPORT Mesh *CreateCube(float size, std::shared_ptr<Material> material);
        JM_EXPORT Mesh *CreatePlane(float width, float height, const maths::Vector3 &normal, std::shared_ptr<Material> material);
    }
}
