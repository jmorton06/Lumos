#pragma once
#include "LAnimFormat.h"
#include "Core/String.h"

namespace Lumos
{
    namespace Graphics
    {
        class Model;
    }

    class LAnimReader
    {
    public:
        static bool Read(Arena* arena, const String8& path, Graphics::Model& outModel);
    };
}
