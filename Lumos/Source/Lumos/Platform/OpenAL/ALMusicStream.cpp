#include "Precompiled.h"
#include "Audio/ChainDecoder.h"
#include "Maths/MathsUtilities.h"
#include "ALMusicStream.h"
#include "Audio/AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include "Audio/AudioSession.h"

#include <atomic>
#include <thread>
#include <cmath>

namespace Lumos
{
    namespace
    {
        // Registry of live streams + the refill thread. Streams unregister themselves under
        // the same lock, so the thread never touches a dead one.
        struct StreamRegistry
        {
            Mutex Lock;
            TDArray<ALMusicStream*> Streams;
            std::thread Thread;
            std::atomic<bool> Running { false };
            bool Initialised = false;

            void EnsureInit()
            {
                if(!Initialised)
                {
                    MutexInit(&Lock);
                    MutexSetName(&Lock, "MusicStreams");
                    Initialised = true;
                }
            }
        };

        StreamRegistry& Registry()
        {
            static StreamRegistry s_Registry;
            s_Registry.EnsureInit();
            return s_Registry;
        }

        void ThreadMain()
        {
            auto& reg = Registry();
            while(reg.Running.load())
            {
                {
                    ScopedMutex lock(&reg.Lock);
                    for(auto* s : reg.Streams)
                        s->Update();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
            }
        }

        void Register(ALMusicStream* s)
        {
            auto& reg = Registry();
            {
                ScopedMutex lock(&reg.Lock);
                reg.Streams.PushBack(s);
            }
            if(!reg.Running.load())
            {
                reg.Running = true;
                reg.Thread  = std::thread(ThreadMain);
            }
        }

        void Unregister(ALMusicStream* s)
        {
            auto& reg = Registry();
            ScopedMutex lock(&reg.Lock);
            for(u32 i = 0; i < reg.Streams.Size(); i++)
            {
                if(reg.Streams[i] == s)
                {
                    reg.Streams[i] = reg.Streams.Back();
                    reg.Streams.PopBack();
                    break;
                }
            }
        }
    }

    ALMusicStream::ALMusicStream(const std::string& path, AudioDecoder* decoder, u32 bufferFrames, u32 bufferCount)
        : m_Decoder(decoder)
        , m_BufferCount(Maths::Clamp(bufferCount, 2u, MaxBuffers))
        , m_BufferFrames(Maths::Max(bufferFrames, 64u))
    {
        MutexInit(&m_Lock);
        m_Path       = path;
        m_SampleRate = decoder->GetSampleRate();
        m_Channels   = decoder->GetChannels();
        m_Duration   = decoder->GetDuration();
        // float buffers keep the decoders' full precision (24-bit files, the EQ, speed changes);
        // without AL_EXT_FLOAT32 the float is dithered down to 16-bit instead of truncated
        m_Format = 0;
        if(alIsExtensionPresent("AL_EXT_FLOAT32"))
            m_Format = alGetEnumValue(m_Channels == 2 ? "AL_FORMAT_STEREO_FLOAT32" : "AL_FORMAT_MONO_FLOAT32");
        m_Float = m_Format != 0 && m_Format != -1;
        if(!m_Float)
            m_Format = m_Channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

        m_FloatScratch = new float[m_BufferFrames * m_Channels];
        if(!m_Float)
            m_Scratch = new i16[m_BufferFrames * m_Channels];

        alGenSources(1, &m_Source);
        alGenBuffers(m_BufferCount, m_Buffers);
        // Plays "at the listener" regardless of where the listener is
        alSourcei(m_Source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(m_Source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSourcef(m_Source, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcef(m_Source, AL_GAIN, m_Volume);

        Register(this);
    }

    ALMusicStream::~ALMusicStream()
    {
        Unregister(this);
        Release();
        delete m_Decoder;
        delete[] m_Scratch;
        delete[] m_FloatScratch;
        MutexDestroy(&m_Lock);
    }

    void ALMusicStream::Release()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        m_Released = true;
        alSourceStop(m_Source);
        alSourcei(m_Source, AL_BUFFER, 0);
        alDeleteSources(1, &m_Source);
        alDeleteBuffers(m_BufferCount, m_Buffers);
    }

    bool ALMusicStream::AnyPlaying()
    {
        auto& reg = Registry();
        ScopedMutex lock(&reg.Lock);
        for(auto* s : reg.Streams)
            if(s->IsPlaying())
                return true;
        return false;
    }

    void ALMusicStream::ShutdownAll()
    {
        auto& reg = Registry();
        if(reg.Running.load())
        {
            reg.Running = false;
            if(reg.Thread.joinable())
                reg.Thread.join();
        }
        ScopedMutex lock(&reg.Lock);
        for(auto* s : reg.Streams)
            s->Release();
    }

    bool ALMusicStream::FillBuffer(ALuint buffer)
    {
        u32 got = 0;
        while(got < m_BufferFrames)
        {
            u32 n = m_Decoder->ReadFloat(m_FloatScratch + got * m_Channels, m_BufferFrames - got);
            if(n == 0)
            {
                if(m_Looping && m_Decoder->GetTotalFrames() > 0 && m_Decoder->Seek(0))
                {
                    // guard against a decoder that seeks fine but never yields frames
                    n = m_Decoder->ReadFloat(m_FloatScratch + got * m_Channels, m_BufferFrames - got);
                    if(n == 0)
                        break;
                }
                else
                    break;
            }
            got += n;
        }

        if(got == 0)
        {
            m_EndOfFile = true;
            return false;
        }

        ScopePushF32(m_FloatScratch, got, m_Channels, m_DecodedFrames);

        if(m_Float)
            alBufferData(buffer, m_Format, m_FloatScratch, (ALsizei)(got * m_Channels * sizeof(float)), (ALsizei)m_SampleRate);
        else
        {
            // TPDF dither: two uniform randoms, +/- 1 LSB
            const u32 count = got * m_Channels;
            u32 x           = m_DitherSeed;
            for(u32 i = 0; i < count; i++)
            {
                x ^= x << 13;
                x ^= x >> 17;
                x ^= x << 5;
                const float r1 = (float)(x & 0xFFFF) / 65536.0f;
                const float r2 = (float)(x >> 16) / 65536.0f;
                float v        = m_FloatScratch[i] * 32767.0f + (r1 - r2);
                v              = v > 32767.0f ? 32767.0f : (v < -32768.0f ? -32768.0f : v);
                m_Scratch[i]   = (i16)lrintf(v);
            }
            m_DitherSeed = x;
            alBufferData(buffer, m_Format, m_Scratch, (ALsizei)(count * sizeof(i16)), (ALsizei)m_SampleRate);
        }
        alSourceQueueBuffers(m_Source, 1, &buffer);

        m_QueuedFrames[(m_QueueHead + m_QueueCount) % m_BufferCount] = got;
        m_QueueCount++;
        m_DecodedFrames += got;
        return got == m_BufferFrames || m_Looping;
    }

    void ALMusicStream::Prime()
    {
        alSourceStop(m_Source);
        alSourcei(m_Source, AL_BUFFER, 0); // drops every queued buffer
        m_QueueHead  = 0;
        m_QueueCount = 0;
        m_EndOfFile  = false;
        m_PlayedFrames = m_DecodedFrames;
        ScopeReset(m_DecodedFrames);

        for(u32 i = 0; i < m_BufferCount; i++)
            if(!FillBuffer(m_Buffers[i]))
                break;
    }

    void ALMusicStream::Update()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released || !m_WantPlaying || m_Paused)
            return;

        ALint processed = 0;
        alGetSourcei(m_Source, AL_BUFFERS_PROCESSED, &processed);
        while(processed-- > 0)
        {
            ALuint buffer = 0;
            alSourceUnqueueBuffers(m_Source, 1, &buffer);
            if(m_QueueCount > 0)
            {
                m_PlayedFrames += m_QueuedFrames[m_QueueHead];
                m_QueueHead = (m_QueueHead + 1) % m_BufferCount;
                m_QueueCount--;
            }
            if(!m_EndOfFile)
                FillBuffer(buffer);
        }

        ALint state = 0;
        alGetSourcei(m_Source, AL_SOURCE_STATE, &state);
        if(state != AL_PLAYING)
        {
            if(m_QueueCount > 0)
                alSourcePlay(m_Source); // underrun - pick back up
            else if(m_EndOfFile)
                m_WantPlaying = false; // finished
        }
    }

    void ALMusicStream::Play()
    {
        AudioSession::SetOpenALHold(AudioSession::Hold_Idle, false); // output back up before AL calls
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        if(m_Paused)
        {
            m_Paused = false;
            alSourcePlay(m_Source);
            return;
        }
        if(m_WantPlaying)
            return;

        if(m_EndOfFile || m_QueueCount == 0)
        {
            // fresh start (or replay after finishing)
            if(m_EndOfFile)
            {
                m_Decoder->Seek(0);
                m_DecodedFrames = 0;
            }
            Prime();
        }
        m_WantPlaying = true;
        alSourcePlay(m_Source);
    }

    void ALMusicStream::Pause()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released || !m_WantPlaying || m_Paused)
            return;
        m_Paused = true;
        alSourcePause(m_Source);
    }

    void ALMusicStream::Stop()
    {
        AudioSession::SetOpenALHold(AudioSession::Hold_Idle, false); // output back up before AL calls
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        m_WantPlaying = false;
        m_Paused      = false;
        m_Decoder->Seek(0);
        m_DecodedFrames = 0;
        Prime();
    }

    void ALMusicStream::Seek(double seconds)
    {
        AudioSession::SetOpenALHold(AudioSession::Hold_Idle, false); // output back up before AL calls
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        if(seconds < 0.0)
            seconds = 0.0;
        u64 frame = (u64)(seconds * m_SampleRate);
        const u64 fileFrames = m_Chain ? m_Chain->GetTotalFrames() : m_Decoder->GetTotalFrames();
        if(fileFrames > 0 && frame >= fileFrames)
            frame = fileFrames - 1;

        m_Decoder->Seek(frame);
        m_DecodedFrames = frame;
        Prime();
        if(m_WantPlaying && !m_Paused)
            alSourcePlay(m_Source);
    }

    u64 ALMusicStream::PlayFrameLocked() const
    {
        if(m_Released)
            return m_PlayedFrames;
        ALint offset = 0;
        alGetSourcei(m_Source, AL_SAMPLE_OFFSET, &offset);
        u64 frame = m_PlayedFrames + (u64)Maths::Max(0, offset);
        return frame > m_DecodedFrames ? m_DecodedFrames : frame;
    }

    u64 ALMusicStream::PlayedSourceFrame() const
    {
        ScopedMutex lock(&m_Lock);
        return m_Decoder->SourceFrame(PlayFrameLocked());
    }

    double ALMusicStream::GetPosition() const
    {
        ScopedMutex lock(&m_Lock);
        u64 frame = m_Decoder->SourceFrame(PlayFrameLocked());
        if(m_Chain)
            frame = LocalFrame(frame, nullptr);
        else
        {
            u64 total = m_Decoder->GetTotalFrames();
            if(total > 0)
                frame = m_Looping ? frame % total : Maths::Min(frame, total);
        }
        return m_SampleRate ? (double)frame / m_SampleRate : 0.0;
    }

    bool ALMusicStream::IsPlaying() const
    {
        ScopedMutex lock(&m_Lock);
        return m_WantPlaying && !m_Paused;
    }

    bool ALMusicStream::IsPaused() const
    {
        ScopedMutex lock(&m_Lock);
        return m_Paused;
    }

    bool ALMusicStream::IsFinished() const
    {
        ScopedMutex lock(&m_Lock);
        return !m_WantPlaying && m_EndOfFile && m_QueueCount == 0;
    }

    void ALMusicStream::SetVolume(float volume)
    {
        ScopedMutex lock(&m_Lock);
        m_Volume = Maths::Max(0.0f, volume);
        if(!m_Released)
            alSourcef(m_Source, AL_GAIN, m_Volume);
    }

    void ALMusicStream::SetPitch(float pitch)
    {
        ScopedMutex lock(&m_Lock);
        m_Pitch = Maths::Clamp(pitch, 0.25f, 4.0f);
        if(!m_Released)
            alSourcef(m_Source, AL_PITCH, m_Pitch);
    }

    u64 ALMusicStream::ScopePlayFrame() const
    {
        ScopedMutex lock(&m_Lock);
        return PlayFrameLocked();
    }

    bool ALMusicStream::ScopeActive() const
    {
        ScopedMutex lock(&m_Lock);
        return m_WantPlaying;
    }

    MusicOutputStatus ALMusicStream::GetOutputStatus() const
    {
        MusicOutputStatus st;
        st.backend    = "openal";
        st.fileRate   = m_SampleRate;
        st.fileBits   = m_Decoder ? m_Decoder->GetSourceBits() : 0;
        st.floatPath  = true; // decoders hand over float either way
        st.deviceBits = m_Float ? 32 : 16;
        st.dithering  = !m_Float;
        return st;
    }

    std::string ALMusicStream::GetStreamTitle() const
    {
        return m_Decoder ? m_Decoder->GetStreamTitle() : std::string();
    }

    bool ALMusicStream::IsBuffering() const
    {
        return m_Decoder && m_Decoder->IsBuffering();
    }
}
