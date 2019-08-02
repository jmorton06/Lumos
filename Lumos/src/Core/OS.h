#pragma once
#include "LM.h"

namespace Lumos
{
    class LUMOS_EXPORT OS
    {
    public:
        OS() {}
        virtual ~OS() {}

        virtual void Run() = 0;
    
        static void Create();
        static void Release();

        static OS* Instance() { return s_Instance; }

    private:

        static OS* s_Instance;
	};
}
