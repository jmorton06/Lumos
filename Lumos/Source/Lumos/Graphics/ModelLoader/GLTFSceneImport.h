#pragma once

#include "Graphics/Light.h"
#include "Maths/Vector3.h"
#include "Maths/Quaternion.h"
#include "Maths/Matrix4.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/Reference.h"
#include <string>

namespace Lumos::Graphics
{
    class Model;

    struct GLTFImportNode
    {
        std::string name;

        // Local transform — either an explicit matrix or TRS (glTF allows both).
        bool hasMatrix = false;
        Mat4 matrix    = Mat4(1.0f);
        Vec3 translation = Vec3(0.0f);
        Quat rotation;                 // identity by default
        Vec3 scale       = Vec3(1.0f);

        SharedPtr<Model> model; // null when the node carries no mesh

        bool hasLight = false;
        Light light;

        bool hasCamera         = false;
        float cameraFovDegrees = 45.0f;
        float cameraNear       = 0.01f;
        float cameraFar        = 1000.0f;

        TDArray<GLTFImportNode> children;
    };

    bool LoadGLTFSceneTree(const std::string& path, GLTFImportNode& outRoot);
}
