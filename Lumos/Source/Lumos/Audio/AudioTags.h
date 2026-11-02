#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    struct AudioChapter
    {
        std::string Title;
        double Start = 0.0; // seconds
    };

    // Metadata for a music file. Strings are UTF-8, empty when missing.
    struct AudioTags
    {
        std::string Title;
        std::string Artist;
        std::string Album;
        std::string AlbumArtist;
        std::string Genre;
        std::string Composer;
        // how to sort them ("Beatles, The"); empty = sort by the name itself
        std::string SortTitle, SortArtist, SortAlbum, SortAlbumArtist;
        u32 Track      = 0;
        u32 TrackCount = 0;
        u32 Disc       = 0;
        u32 DiscCount  = 0;
        u32 Year       = 0;
        bool Compilation = false;

        double Duration = 0.0; // seconds, estimated from headers (no decode)
        u32 SampleRate  = 0;
        u32 Channels    = 0;
        u32 Bitrate     = 0; // kbps
        std::string Codec;   // mp3, aac, alac, flac, vorbis, opus, wav, aiff, dsd ("" = unknown)
        u32 Bits        = 0; // source bit depth for lossless / PCM (1 = DSD, 0 = lossy / unknown)

        bool HasArtwork = false; // set even when artwork bytes weren't requested
        TDArray<u8> Artwork;     // encoded image (jpeg/png) as stored in the file
        std::string ArtworkMime;

        // ReplayGain (dB to add so the song / its album plays at the reference level)
        bool HasTrackGain = false, HasAlbumGain = false;
        float TrackGain = 0.0f, AlbumGain = 0.0f;

        std::string Lyrics; // unsynced text (some taggers store LRC timestamps in it)
        // Chapters (audiobooks, podcasts): MP4 chapter track / chpl, ID3 CHAP; sorted by start
        TDArray<AudioChapter> Chapters;
    };

    // ID3v1/v2 (mp3), MP4 atoms (m4a/aac/mp4/alac) and FLAC vorbis comments.
    // Returns false if the file couldn't be read at all; missing tags are not an error.
    bool ReadAudioTags(const std::string& path, AudioTags& out, bool loadArtwork = true);
}
