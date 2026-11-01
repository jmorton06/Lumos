#pragma once
#include "Core/Core.h"
#include <string>

namespace Lumos
{
    // Fields to change in a music file's tags. Only the fields flagged in `set` are touched;
    // an empty string / 0 in a flagged field removes it.
    struct AudioTagEdit
    {
        enum Field : u32
        {
            Title       = 1 << 0,
            Artist      = 1 << 1,
            Album       = 1 << 2,
            AlbumArtist = 1 << 3,
            Genre       = 1 << 4,
            Composer    = 1 << 5,
            Year        = 1 << 6,
            TrackNum    = 1 << 7, // track + track count
            DiscNum     = 1 << 8, // disc + disc count
            Lyrics          = 1 << 9,
            SortTitle       = 1 << 10,
            SortArtist      = 1 << 11,
            SortAlbum       = 1 << 12,
            SortAlbumArtist = 1 << 13,
            Artwork         = 1 << 14, // front cover: encoded jpeg / png bytes, empty removes every picture
        };
        u32 set = 0;
        std::string title, artist, album, albumArtist, genre, composer;
        std::string lyrics, sortTitle, sortArtist, sortAlbum, sortAlbumArtist;
        std::string artwork, artworkMime; // mime guessed from the bytes when empty
        u32 year = 0, track = 0, trackCount = 0, disc = 0, discCount = 0;
    };

    // Writes tags back into the file: ID3v2 (mp3), MP4 metadata (m4a / m4b / aac / alac / mp4) and
    // FLAC vorbis comments. Everything else in the file (artwork, other frames, the audio) is kept.
    // Rewrites in place when the new tags fit the old space (ID3 padding, a free atom, FLAC
    // padding), otherwise into a temporary file that replaces the original.
    LUMOS_EXPORT bool WriteAudioTags(const std::string& path, const AudioTagEdit& edit, std::string* error = nullptr);
}
