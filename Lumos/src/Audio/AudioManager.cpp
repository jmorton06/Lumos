#include "LM.h"

#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALManager.h"
#endif

namespace Lumos
{
    AudioManager* AudioManager::Create()
    {
        #ifdef LUMOS_OPENAL
        return new Audio::ALManager();
        #else
        return nullptr;
        #endif
    }
}