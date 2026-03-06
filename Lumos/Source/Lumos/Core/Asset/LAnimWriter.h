#pragma once
#include "LAnimFormat.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/String.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    namespace Graphics
    {
        class Skeleton;
        class Animation;
    }

    struct LAnimWriteData
    {
        SharedPtr<Graphics::Skeleton> Skeleton;
        TDArray<SharedPtr<Graphics::Animation>> Animations;
        TDArray<Mat4> BindPoses;
        u64 AssetUUID = 0;
    };

    class LAnimWriter
    {
    public:
        static bool Write(const String8& outputPath, const LAnimWriteData& data);
    };
}
