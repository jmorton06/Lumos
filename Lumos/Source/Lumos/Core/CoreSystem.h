#pragma once

#include "Core/Core.h"

namespace Lumos
{

    struct CommandLine;

    namespace Internal
    {
        // Low-level System operations
        namespace CoreSystem
        {
            bool Init(int argc = 0, char** argv = nullptr);
            void Shutdown();

            CommandLine* GetCmdLine();
        };

    }

};
