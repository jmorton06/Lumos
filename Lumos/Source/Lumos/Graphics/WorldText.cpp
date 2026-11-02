#include "Precompiled.h"
#include "WorldText.h"

namespace Lumos
{
    namespace Graphics
    {
        static TDArray<WorldLabel> s_ActiveWorldLabels;

        TDArray<WorldLabel>& GetActiveWorldLabels()
        {
            return s_ActiveWorldLabels;
        }

        void AddWorldLabel(const Vec3& pos, const char* text, float size, const Vec4& colour, bool local, const Vec4& background)
        {
            if(!text)
                return;

            WorldLabel label;
            label.Position   = pos;
            label.Colour     = colour;
            label.Background = background;
            label.Size       = size;
            label.Local      = local;

            uint32_t i = 0;
            for(; text[i] && i < sizeof(label.Text) - 1; i++)
                label.Text[i] = text[i];
            label.Text[i] = '\0';

            s_ActiveWorldLabels.PushBack(label);
        }

        void ClearWorldLabels()
        {
            s_ActiveWorldLabels.Clear();
        }
    }
}
