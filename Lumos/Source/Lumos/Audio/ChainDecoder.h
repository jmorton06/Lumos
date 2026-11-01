#pragma once
#include "AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include <mutex>

namespace Lumos
{
    // Gapless playback: plays one file, then carries straight on into the next one queued, in the
    // same stream (no restart, no silence). Frames count on across files; Locate maps a played
    // frame back to which file it was in. A seek restarts the count inside the file being decoded.
    class ChainDecoder : public AudioDecoder
    {
    public:
        explicit ChainDecoder(AudioDecoder* first);
        ~ChainDecoder() override;

        u32 Read(i16* out, u32 frames) override;
        u32 ReadFloat(float* out, u32 frames) override;
        bool Seek(u64 frame) override;
        std::string GetStreamTitle() const override { return m_Cur ? m_Cur->GetStreamTitle() : std::string(); }
        bool IsBuffering() const override { return m_Cur && m_Cur->IsBuffering(); }
        bool IsDoP() const override { return m_Cur && m_Cur->IsDoP(); }

        // Takes ownership. False (and deletes it) when it can't join seamlessly: another sample
        // rate or channel count. Replaces anything queued before that hasn't started yet.
        bool Queue(AudioDecoder* next);
        void ClearQueue();
        bool HasQueued() const;

        // Which file a played frame belongs to (0 = the one the stream opened with), where in it,
        // and that file's length
        void Locate(u64 frame, u32* index, u64* local, u64* total) const;

    private:
        struct Boundary
        {
            u64 start;
            u32 index;
            u64 total;
        };
        void Advance(); // under m_Lock
        void CopyInfo(AudioDecoder* d);

        AudioDecoder* m_Cur  = nullptr;
        AudioDecoder* m_Next = nullptr;
        mutable std::mutex m_Lock;
        TDArray<Boundary> m_Bounds;
        u64 m_Pos   = 0;
        u32 m_Index = 0;
    };
}
