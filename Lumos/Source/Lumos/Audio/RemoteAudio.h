#pragma once
#include "Core/Core.h"
#include "Core/OS/Http.h"
#include <string>

namespace Lumos
{
    class AudioDecoder;

    // Songs on a media server, played while they download. Each URL is fetched (Range requests)
    // into a cache file that is kept once whole; reads that get ahead of the download wait on it
    // on a worker thread, so the audio side never blocks - it hears silence and IsBuffering.
    // AudioDecoder::Open / MusicStream::Create take these URLs like paths.
    namespace RemoteAudio
    {
        bool IsUrl(const std::string& path);

        // Starts fetching `url` into `cachePath` (its extension says what kind of file it is) and,
        // with `decode`, opening it so a stream made from the URL starts at once. Again with the same
        // URL does nothing new.
        void Prepare(const std::string& url, const std::string& cachePath, const Http::Headers& headers = {}, bool decode = true);

        struct Status
        {
            const char* state = "none"; // none | opening | ready | downloading | complete | failed
            std::string error;
            u64 have = 0, total = 0;
            bool complete = false; // every byte in cachePath
        };
        Status GetStatus(const std::string& url);

        // Forgets the URL. Streams already playing it keep their copy of the download.
        void Release(const std::string& url);

        // The prepared decoder, or a new one (waits for it to open). nullptr on failure.
        AudioDecoder* Open(const std::string& url, bool precise);

        void ShutdownAll();
    }
}
