#pragma once

#include "Core/Core.h"

namespace Lumos
{
    namespace Internal
    {
        // Low-level System operations
        class LUMOS_EXPORT CoreSystem
        {
        public:
            static bool Init(int argc = 0, char** argv = nullptr);
            static void Shutdown();
        };

    }

};
