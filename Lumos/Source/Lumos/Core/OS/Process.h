#pragma once
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Child process with stdout + stderr merged into one pipe. Poll ReadOutput from any thread
    // (one reader at a time); Kill takes the whole process group down (xcodebuild spawns children).
    // Desktop only: Start returns false on iOS / Android.
    class Process
    {
    public:
        Process() = default;
        ~Process();
        Process(const Process&)            = delete;
        Process& operator=(const Process&) = delete;

        // args[0] is looked up on PATH. workingDir empty = inherit.
        bool Start(const TDArray<std::string>& args, const std::string& workingDir = "");

        // appends whatever output arrived since the last call; false once the pipe is closed and drained
        bool ReadOutput(std::string& out);
        bool IsRunning();
        // blocks until exit, draining output into `out` if given
        int Wait(std::string* out = nullptr);
        void Kill();
        int ExitCode() const { return m_ExitCode; }

        // run to completion; output (stdout + stderr) into `out` if given. -1 if it couldn't start.
        static int Run(const TDArray<std::string>& args, std::string* out = nullptr, const std::string& workingDir = "");
        // "a b" -> "'a b'" style quoting for logs
        static std::string Describe(const TDArray<std::string>& args);

    private:
        void Close();

        int m_ExitCode  = -1;
        bool m_Running  = false;
#ifdef LUMOS_PLATFORM_WINDOWS
        void* m_Process = nullptr;
        void* m_Job     = nullptr;
        void* m_Read    = nullptr;
#else
        int m_Pid  = -1;
        int m_Read = -1;
#endif
    };
}
