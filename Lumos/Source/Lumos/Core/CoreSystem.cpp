#include "Precompiled.h"
#include "CoreSystem.h"
#include "VFS.h"
#include "JobSystem.h"
#include "Scripting/Lua/LuaManager.h"
#include "Core/Version.h"
#include "Core/CommandLine.h"

#include "Core/OS/MemoryManager.h"

namespace Lumos
{
    namespace Internal
    {
        bool CoreSystem::Init(int argc, char** argv)
        {
            Debug::Log::OnInit();

            LUMOS_LOG_INFO("Lumos Engine - Version {0}.{1}.{2}", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

            auto& args = CommandLine::Get();

            // This variables can be set via the command line.
            std::string oString = "Default Value";
            int32_t oInteger = -1;
            uint32_t oUnsigned = 0;
            double oDouble = 0.0;
            float oFloat = 0.f;
            bool oBool = false;
            bool oPrintHelp = false;

            // First configure all possible command line options.
            args.AddArgument({ "-s", "--string" }, &oString, "A string value");
            args.AddArgument({ "-i", "--integer" }, &oInteger, "A integer value");
            args.AddArgument({ "-u", "--unsigned" }, &oUnsigned, "A unsigned value");
            args.AddArgument({ "-d", "--double" }, &oDouble, "A float value");
            args.AddArgument({ "-f", "--float" }, &oFloat, "A double value");
            args.AddArgument({ "-b", "--bool" }, &oBool, "A bool value");
            args.AddArgument({ "-h", "--help" }, &oPrintHelp,
                "Print this help. This help message is actually so long "
                "that it requires a line break!");

            args.Parse(argc, argv);

            if(oPrintHelp)
            {
                args.PrintHelp();
                return false;
            }

            System::JobSystem::OnInit();
            LUMOS_LOG_INFO("Initialising System");
            VFS::OnInit();
            LuaManager::Get().OnInit();

            return true;
        }

        void CoreSystem::Shutdown()
        {
            LUMOS_LOG_INFO("Shutting down System");
            LuaManager::Release();
            VFS::OnShutdown();
            Lumos::Memory::LogMemoryInformation();

            Debug::Log::OnRelease();

            MemoryManager::OnShutdown();
        }
    }
}
