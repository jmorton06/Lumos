#pragma once
#include "LMeshFormat.h"
#include "Core/String.h"

namespace Lumos
{
    namespace Graphics
    {
        class Model;
    }

    class LMeshReader
    {
    public:
        static bool Read(Arena* arena, const String8& path, Graphics::Model& outModel);
    };
}
