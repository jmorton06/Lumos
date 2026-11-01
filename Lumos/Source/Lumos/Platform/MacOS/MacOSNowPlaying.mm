#include "Precompiled.h"
#include "Audio/NowPlaying.h"
#include "Audio/AudioTags.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/Mutex.h"
#include "Maths/MathsUtilities.h"

#import <MediaPlayer/MediaPlayer.h>
// shared by macOS and iOS: only the image type differs
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
typedef UIImage PlatformImage;
#else
#import <AppKit/AppKit.h>
typedef NSImage PlatformImage;
#endif

namespace Lumos
{
    namespace NowPlaying
    {
        namespace
        {
            Mutex s_Lock;
            bool s_LockInit = false;
            TDArray<Command> s_Queue;
            bool s_Registered = false;
            NSMutableArray* s_Targets = nil; // handler tokens, so they can be removed
            std::string s_ArtImage, s_ArtAudio;
            MPMediaItemArtwork* s_Artwork = nil;
            void (*s_Hook)()              = nullptr;

            void EnsureLock()
            {
                if(!s_LockInit)
                {
                    MutexInit(&s_Lock);
                    s_LockInit = true;
                }
            }

            void Push(CommandType type, double position = 0.0)
            {
                EnsureLock();
                ScopedMutex lock(&s_Lock);
                if(s_Queue.Size() < 64)
                    s_Queue.PushBack({ type, position });
            }

            void Add(MPRemoteCommand* cmd, CommandType type)
            {
                cmd.enabled = YES;
                id token = [cmd addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent*) {
                    Push(type);
                    if(s_Hook)
                        s_Hook();
                    return MPRemoteCommandHandlerStatusSuccess;
                }];
                [s_Targets addObject:@[ cmd, token ]];
            }

            void Register()
            {
                if(s_Registered)
                    return;
                EnsureLock();
                s_Registered = true;
                s_Targets    = [[NSMutableArray alloc] init];
                MPRemoteCommandCenter* cc = [MPRemoteCommandCenter sharedCommandCenter];
                Add(cc.playCommand, CommandType::Play);
                Add(cc.pauseCommand, CommandType::Pause);
                Add(cc.togglePlayPauseCommand, CommandType::Toggle);
                Add(cc.stopCommand, CommandType::Stop);
                Add(cc.nextTrackCommand, CommandType::Next);
                Add(cc.previousTrackCommand, CommandType::Previous);

                MPRemoteCommand* seek = cc.changePlaybackPositionCommand;
                seek.enabled          = YES;
                id token = [seek addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* e) {
                    Push(CommandType::Seek, ((MPChangePlaybackPositionCommandEvent*)e).positionTime);
                    if(s_Hook)
                        s_Hook();
                    return MPRemoteCommandHandlerStatusSuccess;
                }];
                [s_Targets addObject:@[ seek, token ]];
            }

            void Unregister()
            {
                if(!s_Registered)
                    return;
                s_Registered = false;
                for(NSArray* pair in s_Targets)
                {
                    MPRemoteCommand* cmd = pair[0];
                    [cmd removeTarget:pair[1]];
                    cmd.enabled = NO;
                }
                [s_Targets release];
                s_Targets = nil;
            }

            PlatformImage* LoadImage(const std::string& imagePath, const std::string& audioPath)
            {
                if(!imagePath.empty())
                {
                    PlatformImage* img = [[PlatformImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:imagePath.c_str()]];
                    if(img)
                        return img;
                }
                if(!audioPath.empty())
                {
                    AudioTags tags;
                    if(ReadAudioTags(audioPath, tags, true) && !tags.Artwork.Empty())
                    {
                        NSData* data = [NSData dataWithBytes:tags.Artwork.Data() length:tags.Artwork.Size()];
                        return [[PlatformImage alloc] initWithData:data];
                    }
                }
                return nil;
            }

            void UpdateArtwork(const Info& info)
            {
                if(info.artworkImage == s_ArtImage && info.artworkAudio == s_ArtAudio)
                    return;
                s_ArtImage = info.artworkImage;
                s_ArtAudio = info.artworkAudio;
                [s_Artwork release];
                s_Artwork    = nil;
                PlatformImage* img = LoadImage(s_ArtImage, s_ArtAudio);
                if(img)
                {
                    s_Artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:img.size
                                                                requestHandler:^PlatformImage*(CGSize) { return img; }];
                    [img release]; // the block keeps it
                }
            }
        }

        bool IsSupported() { return true; }

        void Set(const Info& info)
        {
            @autoreleasepool
            {
                Register();
                UpdateArtwork(info);

                MPRemoteCommandCenter* cc               = [MPRemoteCommandCenter sharedCommandCenter];
                cc.nextTrackCommand.enabled             = info.hasNext;
                cc.previousTrackCommand.enabled         = info.hasPrevious;
                cc.changePlaybackPositionCommand.enabled = info.seekable;

                NSMutableDictionary* d = [NSMutableDictionary dictionary];
                d[MPMediaItemPropertyTitle] = [NSString stringWithUTF8String:info.title.c_str()];
                if(!info.artist.empty())
                    d[MPMediaItemPropertyArtist] = [NSString stringWithUTF8String:info.artist.c_str()];
                if(!info.album.empty())
                    d[MPMediaItemPropertyAlbumTitle] = [NSString stringWithUTF8String:info.album.c_str()];
                if(info.duration > 0.0)
                    d[MPMediaItemPropertyPlaybackDuration] = @(info.duration);
                d[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(info.position);
                d[MPNowPlayingInfoPropertyPlaybackRate]        = @(info.playing ? 1.0 : 0.0);
                d[MPNowPlayingInfoPropertyMediaType]           = @(MPNowPlayingInfoMediaTypeAudio);
                if(s_Artwork)
                    d[MPMediaItemPropertyArtwork] = s_Artwork;

                MPNowPlayingInfoCenter* center = [MPNowPlayingInfoCenter defaultCenter];
                center.nowPlayingInfo          = d;
                center.playbackState           = info.playing ? MPNowPlayingPlaybackStatePlaying : MPNowPlayingPlaybackStatePaused;
            }
        }

        void Clear()
        {
            @autoreleasepool
            {
                MPNowPlayingInfoCenter* center = [MPNowPlayingInfoCenter defaultCenter];
                center.nowPlayingInfo          = nil;
                center.playbackState           = MPNowPlayingPlaybackStateStopped;
                Unregister();
                [s_Artwork release];
                s_Artwork = nil;
                s_ArtImage.clear();
                s_ArtAudio.clear();
            }
        }

        u32 Poll(Command* out, u32 max)
        {
            if(!s_LockInit)
                return 0;
            ScopedMutex lock(&s_Lock);
            u32 n = (u32)Maths::Min<size_t>(max, s_Queue.Size());
            for(u32 i = 0; i < n; i++)
                out[i] = s_Queue[i];
            if(n == s_Queue.Size())
                s_Queue.Clear();
            else
            {
                for(u32 i = n; i < s_Queue.Size(); i++)
                    s_Queue[i - n] = s_Queue[i];
                s_Queue.Resize(s_Queue.Size() - n);
            }
            return n;
        }

        bool GetCurrent(Info& out, bool& hasArtwork)
        {
            @autoreleasepool
            {
                MPNowPlayingInfoCenter* center = [MPNowPlayingInfoCenter defaultCenter];
                NSDictionary* d                = center.nowPlayingInfo;
                if(!d)
                    return false;
                auto str = [&](NSString* key) -> std::string
                {
                    NSString* v = d[key];
                    return v ? std::string(v.UTF8String) : std::string();
                };
                out.title    = str(MPMediaItemPropertyTitle);
                out.artist   = str(MPMediaItemPropertyArtist);
                out.album    = str(MPMediaItemPropertyAlbumTitle);
                out.duration = [d[MPMediaItemPropertyPlaybackDuration] doubleValue];
                out.position = [d[MPNowPlayingInfoPropertyElapsedPlaybackTime] doubleValue];
                out.playing  = center.playbackState == MPNowPlayingPlaybackStatePlaying;
                hasArtwork   = d[MPMediaItemPropertyArtwork] != nil;
                return true;
            }
        }

        void Inject(CommandType type, double position) { Push(type, position); }
        void SetCommandHook(void (*hook)()) { s_Hook = hook; }
    }
}
