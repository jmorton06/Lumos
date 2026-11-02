#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Reads + decodes images on worker threads; the caller turns the pixels into a texture on the
    // main thread. Newest requests are served first, so whatever just scrolled into view wins.
    namespace AsyncImageLoader
    {
        struct Source
        {
            enum Kind : u8
            {
                File,  // an image file
                Audio, // cover embedded in an audio file's tags
                Bytes, // encoded image held in memory
            };
            Kind kind = File;
            std::string data; // path, or the bytes
        };

        struct Result
        {
            bool ok = false;
            u32 width = 0, height = 0;
            TDArray<u8> pixels; // RGBA8
            u32 sourceIndex = 0; // which source produced it
        };

        // Tries the sources in order until one decodes. Returns a job id (never 0).
        // square: crops to the middle (tall images keep more of the top, where faces are)
        LUMOS_EXPORT u32 Queue(const TDArray<Source>& sources, u32 maxSize, bool square = false);
        // true once done (result.ok false when nothing decoded)
        LUMOS_EXPORT bool Poll(u32 id, Result& result);
        LUMOS_EXPORT void Cancel(u32 id);
        LUMOS_EXPORT u32 Pending();
        LUMOS_EXPORT void Shutdown();
    }
}
