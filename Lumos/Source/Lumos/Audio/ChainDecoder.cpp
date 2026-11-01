#include "Precompiled.h"
#include "ChainDecoder.h"

namespace Lumos
{
    ChainDecoder::ChainDecoder(AudioDecoder* first)
        : m_Cur(first)
    {
        CopyInfo(first);
        m_Bounds.PushBack({ 0, 0, first ? first->GetTotalFrames() : 0 });
    }

    ChainDecoder::~ChainDecoder()
    {
        delete m_Cur;
        delete m_Next;
    }

    void ChainDecoder::CopyInfo(AudioDecoder* d)
    {
        if(!d)
            return;
        m_SampleRate  = d->GetSampleRate();
        m_Channels    = d->GetChannels();
        m_TotalFrames = d->GetTotalFrames();
        m_Codec       = d->GetCodec();
        m_Lossless    = d->IsLossless();
        m_FloatSource = d->IsFloatSource();
        m_SourceBits  = d->GetSourceBits();
    }

    void ChainDecoder::Advance()
    {
        delete m_Cur;
        m_Cur  = m_Next;
        m_Next = nullptr;
        m_Index++;
        CopyInfo(m_Cur);
        m_Bounds.PushBack({ m_Pos, m_Index, m_Cur->GetTotalFrames() });
        // keep the list short: only the last few boundaries can still be in flight
        if(m_Bounds.Size() > 8)
        {
            for(u32 i = 1; i < m_Bounds.Size(); i++)
                m_Bounds[i - 1] = m_Bounds[i];
            m_Bounds.PopBack();
        }
    }

    u32 ChainDecoder::Read(i16* out, u32 frames)
    {
        u32 done = 0;
        while(done < frames && m_Cur)
        {
            u32 n = m_Cur->Read(out + (size_t)done * m_Channels, frames - done);
            done += n;
            m_Pos += n;
            if(n == 0)
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                if(!m_Next)
                    break;
                Advance();
            }
        }
        return done;
    }

    u32 ChainDecoder::ReadFloat(float* out, u32 frames)
    {
        u32 done = 0;
        while(done < frames && m_Cur)
        {
            u32 n = m_Cur->ReadFloat(out + (size_t)done * m_Channels, frames - done);
            done += n;
            m_Pos += n;
            if(n == 0)
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                if(!m_Next)
                    break;
                Advance();
            }
        }
        return done;
    }

    bool ChainDecoder::Seek(u64 frame)
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        if(!m_Cur)
            return false;
        bool ok = m_Cur->Seek(frame);
        m_Pos   = frame;
        m_Bounds.Clear();
        // counted from the file's own start again
        m_Bounds.PushBack({ 0, m_Index, m_Cur->GetTotalFrames() });
        return ok;
    }

    bool ChainDecoder::Queue(AudioDecoder* next)
    {
        if(!next)
            return false;
        if(next->GetSampleRate() != m_SampleRate || next->GetChannels() != m_Channels || next->IsDoP() != IsDoP())
        {
            delete next;
            return false;
        }
        std::lock_guard<std::mutex> lock(m_Lock);
        delete m_Next;
        m_Next = next;
        return true;
    }

    void ChainDecoder::ClearQueue()
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        delete m_Next;
        m_Next = nullptr;
    }

    bool ChainDecoder::HasQueued() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Next != nullptr;
    }

    void ChainDecoder::Locate(u64 frame, u32* index, u64* local, u64* total) const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        const Boundary* b = &m_Bounds[0];
        for(u32 i = 0; i < m_Bounds.Size(); i++)
            if(m_Bounds[i].start <= frame)
                b = &m_Bounds[i];
        if(index)
            *index = b->index;
        if(local)
            *local = frame - b->start;
        if(total)
            *total = b->total;
    }
}
