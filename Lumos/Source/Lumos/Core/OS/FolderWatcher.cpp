#include "Precompiled.h"
#include "FolderWatcher.h"

#include <mutex>

#ifdef LUMOS_PLATFORM_MACOS
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#endif

namespace Lumos
{
    namespace FolderWatcher
    {
        namespace
        {
            std::mutex s_Lock;
            TDArray<std::string> s_Changed;
            constexpr u32 MaxQueued = 4096;

            void Push(const char* path)
            {
                std::lock_guard<std::mutex> lock(s_Lock);
                for(u32 i = 0; i < s_Changed.Size(); i++)
                    if(s_Changed[i] == path)
                        return;
                if(s_Changed.Size() < MaxQueued)
                    s_Changed.PushBack(path);
            }

#ifdef LUMOS_PLATFORM_MACOS
            FSEventStreamRef s_Stream = nullptr;
            dispatch_queue_t s_Queue  = nullptr;

            void OnEvents(ConstFSEventStreamRef, void*, size_t count, void* paths, const FSEventStreamEventFlags*, const FSEventStreamEventId*)
            {
                char** p = (char**)paths;
                for(size_t i = 0; i < count; i++)
                    Push(p[i]);
            }

            // stop the stream before statics go away
            struct Guard
            {
                ~Guard() { Stop(); }
            } s_Guard;
#endif
        }

#ifdef LUMOS_PLATFORM_MACOS
        bool IsSupported() { return true; }

        void Stop()
        {
            if(s_Stream)
            {
                FSEventStreamStop(s_Stream);
                FSEventStreamInvalidate(s_Stream);
                FSEventStreamRelease(s_Stream);
                s_Stream = nullptr;
            }
        }

        void Watch(const TDArray<std::string>& folders)
        {
            Stop();
            if(folders.Empty())
                return;
            if(!s_Queue)
                s_Queue = dispatch_queue_create("lumos.folderwatcher", DISPATCH_QUEUE_SERIAL);

            CFMutableArrayRef list = CFArrayCreateMutable(nullptr, (CFIndex)folders.Size(), &kCFTypeArrayCallBacks);
            for(u32 i = 0; i < folders.Size(); i++)
            {
                CFStringRef s = CFStringCreateWithCString(nullptr, folders[i].c_str(), kCFStringEncodingUTF8);
                if(s)
                {
                    CFArrayAppendValue(list, s);
                    CFRelease(s);
                }
            }
            FSEventStreamContext ctx = {};
            // WatchRoot: also hear about the folder itself going away / coming back (drives)
            s_Stream = FSEventStreamCreate(nullptr, &OnEvents, &ctx, list, kFSEventStreamEventIdSinceNow, 0.5,
                                           kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagWatchRoot | kFSEventStreamCreateFlagFileEvents);
            CFRelease(list);
            if(!s_Stream)
            {
                LWARN("FolderWatcher: couldn't create an FSEvents stream");
                return;
            }
            FSEventStreamSetDispatchQueue(s_Stream, s_Queue);
            if(!FSEventStreamStart(s_Stream))
            {
                LWARN("FolderWatcher: couldn't start the FSEvents stream");
                Stop();
            }
        }
#else
        // TODO: inotify (Linux), ReadDirectoryChangesW (Windows)
        bool IsSupported() { return false; }
        void Stop() { }
        void Watch(const TDArray<std::string>&) { }
#endif

        u32 Poll(TDArray<std::string>& out)
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            u32 n = (u32)s_Changed.Size();
            for(u32 i = 0; i < n; i++)
                out.PushBack(s_Changed[i]);
            s_Changed.Clear();
            return n;
        }
    }
}
