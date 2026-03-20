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

            LINFO("Lumos Engine - Version %i.%i.%i", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);
#if LUMOS_PROFILE
            String8 versionString = PushStr8F(s_Arena, "Version : %i.%i.%i", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);
            TracyAppInfo((const char*)versionString.str, versionString.size);
#endif

            String8List argsList = {};
            for(uint64_t argumentIndex = 1; argumentIndex < argc; argumentIndex += 1)
            {
                Str8ListPush(s_Arena, &argsList, Str8C(argv[argumentIndex]));
            }
            s_CommandLine = {};
            s_CommandLine.Init(s_Arena, argsList);

            if(s_CommandLine.OptionBool(Str8Lit("help")))
            {
                LINFO(
                    "Available command-line options:\n"
                    "  --EnableVulkanValidation     Enable Vulkan API validation layers\n"
                    "  --CleanEditorIni             Delete the editor's INI file on startup\n"
                    "  --help                       Show this help message\n"
                    "  --disable-splash             Disable Splash Screen\n"
                    "  --test-maths                 Test Maths Library\n"
                    "  --screenshot=<path>          Take screenshot on first frame and save to path\n"
                    "  --screenshot-no-close        Don't close app after taking screenshot\n"
                    "  --force-embedded-shaders     Force use of embedded shaders (skip disk loading)\n"
                    "  --test-ui                    Show test UI on startup\n"
                    "  --disable-test-ui            Hide test UI on startup\n"
                    "  --headless                   Run without rendering (no window)\n"
                    "  --scene=<name>               Load specific scene on startup\n"
                    "  --benchmark=<frames>         Run for N frames, log performance, then exit\n");
            }

            // Init Jobsystem. Reserve 2 threads for main and render threads
            System::JobSystem::OnInit(2);
            LINFO("Initialising System");
            FileSystem::Get();

            return true;
        }

        void CoreSystem::Shutdown()
        {
            LINFO("Shutting down System");
            FileSystem::Release();

            Debug::Log::OnRelease();
            System::JobSystem::Release();

            ArenaClear(s_Arena);

            MemoryManager::Release();
            ThreadContextRelease(GetThreadContext());
        }

        CommandLine* CoreSystem::GetCmdLine()
        {
            return &s_CommandLine;
        }
    }
}
