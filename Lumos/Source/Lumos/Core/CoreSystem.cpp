#include "Precompiled.h"
#include "CoreSystem.h"
#include "OS/FileSystem.h"
#include "JobSystem.h"
#include "Scripting/Lua/LuaManager.h"
#include "Core/Version.h"
#include "Core/CommandLine.h"
#include "Core/Thread.h"
#include "Core/OS/MemoryManager.h"

namespace Lumos
{
    namespace Internal
    {
        static Arena* s_Arena;
        static CommandLine s_CommandLine;

        bool CoreSystem::Init(int argc, char** argv)
        {
            Debug::Log::OnInit();
            ThreadContext& mainThread = *GetThreadContext();
            mainThread                = ThreadContextAlloc();
            SetThreadName(Str8Lit("Main"));

            s_Arena = ArenaAlloc(Megabytes(2));

            LUMOS_LOG_INFO("Lumos Engine - Version {0}.{1}.{2}", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

            String8List argsList = {};
            for(uint64_t argumentIndex = 1; argumentIndex < argc; argumentIndex += 1)
            {
                Str8ListPush(s_Arena, &argsList, Str8C(argv[argumentIndex]));
            }
            s_CommandLine = {};
            s_CommandLine.Init(s_Arena, argsList);

            if(s_CommandLine.OptionBool(Str8Lit("help")))
            {
                LUMOS_LOG_INFO("Print this help. This help message is actually so long "
                               "that it requires a line break!");
            }

            // Init Jobsystem. Reserve 2 threads for main and render threads
            System::JobSystem::OnInit(2);
            LUMOS_LOG_INFO("Initialising System");
            FileSystem::Get();

            return true;
        }

        void CoreSystem::Shutdown()
        {
            LUMOS_LOG_INFO("Shutting down System");
            FileSystem::Release();
            Lumos::Memory::LogMemoryInformation();

            Debug::Log::OnRelease();
            System::JobSystem::Release();

            ArenaClear(s_Arena);

            MemoryManager::OnShutdown();
            ThreadContextRelease(GetThreadContext());
        }

        CommandLine* CoreSystem::GetCmdLine()
        {
            return &s_CommandLine;
        }
    }
}
