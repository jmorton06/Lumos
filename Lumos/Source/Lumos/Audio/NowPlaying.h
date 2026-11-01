#pragma once
#include "Core/Core.h"
#include <string>

namespace Lumos
{
    // The OS "now playing" widget (Control Center, lock screen) and media keys / headset buttons.
    namespace NowPlaying
    {
        struct Info
        {
            std::string title, artist, album;
            double duration = 0.0;
            double position = 0.0; // the OS extrapolates from here while playing
            bool playing    = false;
            bool seekable   = true;
            bool hasNext = true, hasPrevious = true;
            // artwork: an image file, else the embedded cover of an audio file
            std::string artworkImage;
            std::string artworkAudio;
        };

        enum class CommandType
        {
            Play,
            Pause,
            Toggle,
            Stop,
            Next,
            Previous,
            Seek, // position = seconds
        };

        struct Command
        {
            CommandType type;
            double position = 0.0;
        };

        LUMOS_EXPORT bool IsSupported();
        // Claims media keys and shows the widget. Artwork is only reloaded when its paths change.
        LUMOS_EXPORT void Set(const Info& info);
        // Hides the widget and lets other apps have the media keys
        LUMOS_EXPORT void Clear();
        // Commands received since the last call, oldest first. Returns how many were written.
        LUMOS_EXPORT u32 Poll(Command* out, u32 max);
        // What the OS currently has (title / artist / album / duration / position / playing); false if nothing
        LUMOS_EXPORT bool GetCurrent(Info& out, bool& hasArtwork);
        // Test hook: queue a command as if the OS sent it
        LUMOS_EXPORT void Inject(CommandType type, double position = 0.0);
        // Called on the main thread right after a command is queued - iOS uses it to handle
        // commands that wake a suspended app, which gets no frames or timer ticks to poll them
        LUMOS_EXPORT void SetCommandHook(void (*hook)());
    }
}
