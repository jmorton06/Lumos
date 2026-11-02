#include "Precompiled.h"
#include "Process.h"

#if defined(LUMOS_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_LINUX)
#define LUMOS_PROCESS_POSIX
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
extern char** environ;
#endif

namespace Lumos
{
    Process::~Process()
    {
        if(m_Running)
            Kill();
        Close();
    }

    std::string Process::Describe(const TDArray<std::string>& args)
    {
        std::string s;
        for(u32 i = 0; i < args.Size(); i++)
        {
            if(i)
                s += ' ';
            bool quote = args[i].find_first_of(" \t\"'") != std::string::npos || args[i].empty();
            s += quote ? ("\"" + args[i] + "\"") : args[i];
        }
        return s;
    }

    int Process::Run(const TDArray<std::string>& args, std::string* out, const std::string& workingDir)
    {
        Process p;
        if(!p.Start(args, workingDir))
            return -1;
        return p.Wait(out);
    }

#if defined(LUMOS_PROCESS_POSIX)

    bool Process::Start(const TDArray<std::string>& args, const std::string& workingDir)
    {
        if(args.Empty() || m_Running)
            return false;
        int fds[2];
        if(pipe(fds) != 0)
            return false;

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, fds[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, fds[0]);
        posix_spawn_file_actions_addclose(&actions, fds[1]);
        posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);

        // own process group so Kill reaches grandchildren too
        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
        posix_spawnattr_setpgroup(&attr, 0);

        TDArray<char*> argv;
        for(auto& a : args)
            argv.PushBack(const_cast<char*>(a.c_str()));
        argv.PushBack(nullptr);

        // posix_spawn has no portable chdir: hop through sh when a working dir is wanted
        TDArray<std::string> shArgs;
        if(!workingDir.empty())
        {
            shArgs.PushBack("/bin/sh");
            shArgs.PushBack("-c");
            shArgs.PushBack("cd \"$0\" && exec \"$@\"");
            shArgs.PushBack(workingDir);
            for(auto& a : args)
                shArgs.PushBack(a);
            argv.Clear();
            for(auto& a : shArgs)
                argv.PushBack(const_cast<char*>(a.c_str()));
            argv.PushBack(nullptr);
        }

        pid_t pid = -1;
        int err   = posix_spawnp(&pid, argv[0], &actions, &attr, argv.Data(), environ);
        posix_spawn_file_actions_destroy(&actions);
        posix_spawnattr_destroy(&attr);
        close(fds[1]);
        if(err != 0)
        {
            close(fds[0]);
            LERROR("Process: couldn't start %s (%s)", args[0].c_str(), strerror(err));
            return false;
        }
        fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
        m_Pid      = pid;
        m_Read     = fds[0];
        m_Running  = true;
        m_ExitCode = -1;
        return true;
    }

    bool Process::ReadOutput(std::string& out)
    {
        if(m_Read < 0)
            return false;
        char buf[4096];
        for(;;)
        {
            ssize_t n = read(m_Read, buf, sizeof(buf));
            if(n > 0)
            {
                out.append(buf, (size_t)n);
                continue;
            }
            if(n == 0)
            {
                close(m_Read);
                m_Read = -1;
                return false;
            }
            return errno == EAGAIN || errno == EINTR;
        }
    }

    bool Process::IsRunning()
    {
        if(!m_Running)
            return false;
        int status = 0;
        pid_t r    = waitpid(m_Pid, &status, WNOHANG);
        if(r == m_Pid)
        {
            m_Running  = false;
            m_ExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
        }
        else if(r < 0)
            m_Running = false;
        return m_Running;
    }

    int Process::Wait(std::string* out)
    {
        std::string sink;
        while(IsRunning())
        {
            ReadOutput(out ? *out : sink);
            usleep(5000);
        }
        // whatever is still buffered after exit
        if(m_Read >= 0)
        {
            fcntl(m_Read, F_SETFL, fcntl(m_Read, F_GETFL) & ~O_NONBLOCK);
            while(ReadOutput(out ? *out : sink)) { }
        }
        return m_ExitCode;
    }

    void Process::Kill()
    {
        if(m_Running && m_Pid > 0)
        {
            kill(-m_Pid, SIGTERM);
            for(int i = 0; i < 40 && IsRunning(); i++)
                usleep(25000);
            if(IsRunning())
                kill(-m_Pid, SIGKILL);
            Wait();
        }
    }

    void Process::Close()
    {
        if(m_Read >= 0)
            close(m_Read);
        m_Read = -1;
    }

#elif defined(LUMOS_PLATFORM_WINDOWS)

    static std::string QuoteArg(const std::string& a)
    {
        if(!a.empty() && a.find_first_of(" \t\"") == std::string::npos)
            return a;
        std::string q = "\"";
        for(char c : a)
        {
            if(c == '"')
                q += '\\';
            q += c;
        }
        return q + "\"";
    }

    bool Process::Start(const TDArray<std::string>& args, const std::string& workingDir)
    {
        if(args.Empty() || m_Running)
            return false;
        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE readPipe = nullptr, writePipe = nullptr;
        if(!CreatePipe(&readPipe, &writePipe, &sa, 0))
            return false;
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si = {};
        si.cb           = sizeof(si);
        si.dwFlags      = STARTF_USESTDHANDLES;
        si.hStdOutput   = writePipe;
        si.hStdError    = writePipe;
        si.hStdInput    = nullptr;

        std::string cmd;
        for(u32 i = 0; i < args.Size(); i++)
            cmd += (i ? " " : "") + QuoteArg(args[i]);

        PROCESS_INFORMATION pi = {};
        BOOL ok                = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr,
                                                workingDir.empty() ? nullptr : workingDir.c_str(), &si, &pi);
        CloseHandle(writePipe);
        if(!ok)
        {
            CloseHandle(readPipe);
            LERROR("Process: couldn't start %s (%lu)", args[0].c_str(), GetLastError());
            return false;
        }
        // a job object so Kill takes the children with it
        HANDLE job = CreateJobObjectA(nullptr, nullptr);
        if(job)
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
            info.BasicLimitInformation.LimitFlags     = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info));
            AssignProcessToJobObject(job, pi.hProcess);
        }
        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);
        m_Process  = pi.hProcess;
        m_Job      = job;
        m_Read     = readPipe;
        m_Running  = true;
        m_ExitCode = -1;
        return true;
    }

    bool Process::ReadOutput(std::string& out)
    {
        if(!m_Read)
            return false;
        for(;;)
        {
            DWORD avail = 0;
            if(!PeekNamedPipe((HANDLE)m_Read, nullptr, 0, nullptr, &avail, nullptr))
            {
                CloseHandle((HANDLE)m_Read);
                m_Read = nullptr;
                return false;
            }
            if(avail == 0)
                return true;
            char buf[4096];
            DWORD n = 0;
            if(!ReadFile((HANDLE)m_Read, buf, (DWORD)std::min<DWORD>(avail, sizeof(buf)), &n, nullptr) || n == 0)
                return true;
            out.append(buf, n);
        }
    }

    bool Process::IsRunning()
    {
        if(!m_Running)
            return false;
        if(WaitForSingleObject((HANDLE)m_Process, 0) == WAIT_OBJECT_0)
        {
            DWORD code = 0;
            GetExitCodeProcess((HANDLE)m_Process, &code);
            m_ExitCode = (int)code;
            m_Running  = false;
        }
        return m_Running;
    }

    int Process::Wait(std::string* out)
    {
        std::string sink;
        while(IsRunning())
        {
            ReadOutput(out ? *out : sink);
            Sleep(5);
        }
        while(ReadOutput(out ? *out : sink) && m_Read)
        {
            DWORD avail = 0;
            if(!PeekNamedPipe((HANDLE)m_Read, nullptr, 0, nullptr, &avail, nullptr) || avail == 0)
                break;
        }
        return m_ExitCode;
    }

    void Process::Kill()
    {
        if(m_Running)
        {
            if(m_Job)
                TerminateJobObject((HANDLE)m_Job, 1);
            else
                TerminateProcess((HANDLE)m_Process, 1);
            Wait();
        }
    }

    void Process::Close()
    {
        if(m_Read)
            CloseHandle((HANDLE)m_Read);
        if(m_Process)
            CloseHandle((HANDLE)m_Process);
        if(m_Job)
            CloseHandle((HANDLE)m_Job);
        m_Read = m_Process = m_Job = nullptr;
    }

#else

    bool Process::Start(const TDArray<std::string>&, const std::string&) { return false; }
    bool Process::ReadOutput(std::string&) { return false; }
    bool Process::IsRunning() { return false; }
    int Process::Wait(std::string*) { return m_ExitCode; }
    void Process::Kill() { }
    void Process::Close() { }

#endif
}
