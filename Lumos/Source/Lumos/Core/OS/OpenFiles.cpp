#include "Precompiled.h"
#include "OpenFiles.h"
#include <mutex>

namespace Lumos
{
    namespace OpenFiles
    {
        namespace
        {
            std::mutex s_Lock;
            TDArray<std::string> s_Queue;
        }

        void Push(const std::string& path)
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            if(s_Queue.Size() < 1024)
                s_Queue.PushBack(path);
        }

        void Pop(TDArray<std::string>& out)
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            for(u32 i = 0; i < s_Queue.Size(); i++)
                out.PushBack(s_Queue[i]);
            s_Queue.Clear();
        }

#ifndef LUMOS_PLATFORM_MACOS
        // TODO: Windows / Linux pass files on the command line
        void Install() { }
#endif
    }
}
