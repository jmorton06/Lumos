#pragma once
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    namespace Graphics
    {
        struct WorldLabel
        {
            Vec3 Position;
            Vec4 Colour;
            Vec4 Background; // alpha 0 = none; else a solid backing quad under the text
            float Size;
            bool Local; // authored in the local "system frame" (see PlanetField)
            char Text[64];
        };

        TDArray<WorldLabel>& GetActiveWorldLabels();
        void AddWorldLabel(const Vec3& pos, const char* text, float size, const Vec4& colour, bool local = false, const Vec4& background = Vec4(0.0f));
        void ClearWorldLabels();
    }
}
