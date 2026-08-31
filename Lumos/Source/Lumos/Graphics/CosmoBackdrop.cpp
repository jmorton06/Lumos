#include "Precompiled.h"
#include "CosmoBackdrop.h"

namespace Lumos
{
    namespace Graphics
    {
        namespace CosmoBackdrop
        {
            static State s_State;

            State& Get()
            {
                return s_State;
            }
        }
    }
}
