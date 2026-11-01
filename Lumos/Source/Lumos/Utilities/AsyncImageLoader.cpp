#include "Precompiled.h"
#include "AsyncImageLoader.h"
#include "LoadImage.h"
#include "Audio/AudioTags.h"
#include "Core/Mutex.h"

#include <atomic>
#include <thread>
#include <cstdio>
#include <unordered_map>

namespace Lumos
{
    namespace AsyncImageLoader
    {
        namespace
        {
            struct Job
            {
                u32 id = 0;
                TDArray<Source> sources;
                u32 maxSize = 0;
                bool square = false;
            };

            struct Loader
            {
                Mutex Lock;
                TDArray<Job> Queue; // served from the back
                std::unordered_map<u32, Result> Done;
                TDArray<u32> Cancelled;
                std::thread Threads[2];
                std::atomic<bool> Running { false };
                std::atomic<u32> Busy { 0 };
                u32 NextId = 1;
                bool Initialised = false;
            };

            Loader& Get()
            {
                static Loader s_Loader;
                if(!s_Loader.Initialised)
                {
                    MutexInit(&s_Loader.Lock);
                    s_Loader.Initialised = true;
                }
                return s_Loader;
            }

            bool ReadWholeFile(const std::string& path, TDArray<u8>& out)
            {
                FILE* f = fopen(path.c_str(), "rb");
                if(!f)
                    return false;
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                if(size <= 0 || size > 64 * 1024 * 1024)
                {
                    fclose(f);
                    return false;
                }
                out.Resize((size_t)size);
                bool ok = fread(out.Data(), 1, (size_t)size, f) == (size_t)size;
                fclose(f);
                return ok;
            }

            bool Decode(const u8* data, size_t size, u32 maxSize, Result& r)
            {
                u32 w = 0, h = 0;
                u8* pixels = LoadImageFromMemory(data, size, &w, &h, maxSize);
                if(!pixels)
                    return false;
                r.width  = w;
                r.height = h;
                r.pixels.Resize((size_t)w * h * 4);
                memcpy(r.pixels.Data(), pixels, (size_t)w * h * 4);
                FreeImageMemory(pixels);
                r.ok = true;
                return true;
            }

            void CropSquare(Result& r)
            {
                const u32 side = r.width < r.height ? r.width : r.height;
                if(side == 0 || r.width == r.height)
                    return;
                const u32 x0 = (r.width - side) / 2;
                const u32 y0 = (r.height - side) / 5;
                TDArray<u8> out;
                out.Resize((size_t)side * side * 4);
                for(u32 y = 0; y < side; y++)
                    memcpy(out.Data() + (size_t)y * side * 4, r.pixels.Data() + ((size_t)(y + y0) * r.width + x0) * 4, (size_t)side * 4);
                r.pixels = std::move(out);
                r.width = r.height = side;
            }

            void Run(const Job& job, Result& r)
            {
                for(u32 i = 0; i < job.sources.Size(); i++)
                {
                    const Source& s = job.sources[i];
                    bool ok         = false;
                    if(s.kind == Source::Bytes)
                        ok = Decode((const u8*)s.data.data(), s.data.size(), job.maxSize, r);
                    else if(s.kind == Source::File)
                    {
                        TDArray<u8> bytes;
                        ok = ReadWholeFile(s.data, bytes) && Decode(bytes.Data(), bytes.Size(), job.maxSize, r);
                    }
                    else
                    {
                        AudioTags tags;
                        ok = ReadAudioTags(s.data, tags, true) && !tags.Artwork.Empty() && Decode(tags.Artwork.Data(), tags.Artwork.Size(), job.maxSize, r);
                    }
                    if(ok)
                    {
                        if(job.square)
                            CropSquare(r);
                        r.sourceIndex = i;
                        return;
                    }
                }
            }

            void Worker()
            {
                Loader& l = Get();
                while(l.Running.load())
                {
                    Job job;
                    {
                        ScopedMutex lock(&l.Lock);
                        if(!l.Queue.Empty())
                        {
                            job = std::move(l.Queue.Back());
                            l.Queue.PopBack();
                            l.Busy++;
                        }
                    }
                    if(job.id == 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(4));
                        continue;
                    }
                    Result r;
                    Run(job, r);
                    ScopedMutex lock(&l.Lock);
                    l.Busy--;
                    bool cancelled = false;
                    for(u32 i = 0; i < l.Cancelled.Size(); i++)
                        if(l.Cancelled[i] == job.id)
                        {
                            l.Cancelled[i] = l.Cancelled.Back();
                            l.Cancelled.PopBack();
                            cancelled = true;
                            break;
                        }
                    if(!cancelled)
                        l.Done[job.id] = std::move(r);
                }
            }
        }

        u32 Queue(const TDArray<Source>& sources, u32 maxSize, bool square)
        {
            Loader& l = Get();
            u32 id;
            {
                ScopedMutex lock(&l.Lock);
                id = l.NextId++;
                if(l.NextId == 0)
                    l.NextId = 1;
                Job job;
                job.id      = id;
                job.sources = sources;
                job.maxSize = maxSize;
                job.square  = square;
                l.Queue.PushBack(std::move(job));
            }
            if(!l.Running.load())
            {
                l.Running = true;
                for(auto& t : l.Threads)
                    t = std::thread(Worker);
            }
            return id;
        }

        bool Poll(u32 id, Result& result)
        {
            Loader& l = Get();
            ScopedMutex lock(&l.Lock);
            auto it = l.Done.find(id);
            if(it == l.Done.end())
                return false;
            result = std::move(it->second);
            l.Done.erase(it);
            return true;
        }

        void Cancel(u32 id)
        {
            Loader& l = Get();
            ScopedMutex lock(&l.Lock);
            for(u32 i = 0; i < l.Queue.Size(); i++)
                if(l.Queue[i].id == id)
                {
                    for(u32 j = i + 1; j < l.Queue.Size(); j++)
                        l.Queue[j - 1] = std::move(l.Queue[j]);
                    l.Queue.PopBack();
                    return;
                }
            if(l.Done.erase(id) == 0)
                l.Cancelled.PushBack(id); // running now: drop the result when it lands
        }

        u32 Pending()
        {
            Loader& l = Get();
            ScopedMutex lock(&l.Lock);
            return (u32)l.Queue.Size() + l.Busy.load();
        }

        void Shutdown()
        {
            Loader& l = Get();
            if(!l.Running.load())
                return;
            l.Running = false;
            for(auto& t : l.Threads)
                if(t.joinable())
                    t.join();
            ScopedMutex lock(&l.Lock);
            l.Queue.Clear();
            l.Done.clear();
        }
    }
}
