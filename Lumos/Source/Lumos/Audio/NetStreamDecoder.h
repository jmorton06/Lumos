#pragma once
#include "AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace Lumos
{
    class HttpStream;

    // MP3 internet radio. Connects, strips ICY metadata and decodes on its own thread into a
    // few seconds of PCM; Read never blocks (it plays silence while the network catches up).
    class LUMOS_EXPORT NetStreamDecoder : public AudioDecoder
    {
    public:
        enum class State
        {
            Connecting,
            Ready, // sample rate known and a little buffered: safe to make a MusicStream
            Failed,
            Ended,
        };

        explicit NetStreamDecoder(const std::string& url);
        ~NetStreamDecoder() override;

        State GetState() const { return m_State.load(); }
        std::string GetError() const;
        std::string GetStationName() const;
        std::string GetStreamTitle() const override;
        bool IsBuffering() const override { return m_Starved.load(); }

        u32 Read(i16* out, u32 frames) override;
        bool Seek(u64) override { return false; }

        // dr_mp3 pulls compressed bytes through here (worker thread)
        size_t ReadCompressed(u8* out, size_t bytes);

    private:
        void Run();
        void Fail(const std::string& error);
        void ParseMetadata(const std::string& block);

        std::string m_Url;
        HttpStream* m_Http = nullptr;
        std::thread m_Thread;
        std::atomic<bool> m_Stop { false };
        std::atomic<State> m_State { State::Connecting };
        std::atomic<bool> m_Starved { false };

        mutable std::mutex m_Lock;
        std::string m_Error, m_Station, m_Title;
        TDArray<i16> m_Ring; // interleaved
        u64 m_RingRead = 0, m_RingWrite = 0; // in samples
        u32 m_RingFrames = 0;

        u32 m_MetaInt = 0;       // bytes of audio between ICY metadata blocks (0 = none)
        u32 m_UntilMeta = 0;
    };
}
