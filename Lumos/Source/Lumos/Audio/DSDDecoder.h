#pragma once
#include "AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include <cstdio>

namespace Lumos
{
    // DSD files (.dsf, .dff) as PCM: the 1-bit stream goes through a low-pass FIR and is decimated
    // to 88.2 kHz float (DSD64 / 128 / 256 alike). DoP mode instead packs 16 DSD bits per channel
    // into 24-bit words with the DoP markers (176.4 kHz for DSD64) for a DAC that decodes DSD
    // itself - those samples must reach the device untouched (no volume, EQ, dither or resampling).
    class DSDDecoder : public AudioDecoder
    {
    public:
        static bool IsDSDExtension(const std::string& ext);
        // null if the file isn't DSD it can read (DST-compressed DFF isn't supported)
        static DSDDecoder* Open(const std::string& path, bool dop);

        ~DSDDecoder() override;

        u32 Read(i16* out, u32 frames) override;
        u32 ReadFloat(float* out, u32 frames) override;
        bool Seek(u64 frame) override;
        bool IsDoP() const override { return m_DoP; }

        u32 GetDSDRate() const { return m_DSDRate; }

    private:
        bool InitDSF();
        bool InitDFF();
        void BuildFilter();
        // next `n` bytes of one channel's bit stream (MSB = earliest), false at the end
        bool FetchBytes(u32 count);
        bool ReadBlock();

        FILE* m_File = nullptr;
        bool m_IsDSF = false;
        bool m_DoP   = false;
        u32 m_DSDRate = 0;
        u32 m_FileChannels = 0;
        u64 m_DataOffset = 0;   // first data byte
        u64 m_DataBytes  = 0;   // per channel
        u64 m_BytePos    = 0;   // per channel bytes consumed
        bool m_LSBFirst  = false;
        u32 m_BlockSize  = 0;   // DSF: bytes per channel per block

        // per channel byte history for the FIR (ring), bytes read from the file per channel
        u32 m_Decim  = 32;      // DSD bits per output sample
        u32 m_Taps   = 0;       // filter length in bytes
        TDArray<float> m_Table; // m_Taps x 256: contribution of one byte at each tap position
        TDArray<u8> m_Hist;     // channel x m_Taps ring
        u32 m_HistPos = 0;
        TDArray<u8> m_Block;    // DSF block, all channels
        u32 m_BlockPos = 0, m_BlockFill = 0;
        TDArray<u8> m_Pending;  // bytes fetched for the current step, channel-major
        TDArray<float> m_Tmp;
    };
}
