#include "Audio/AudioSession.h"
#include "Audio/AudioOutput.h"
#include "Core/LMLog.h"
#import <AVFoundation/AVFoundation.h>
#include <AL/alc.h>
#include <cmath>

namespace Lumos
{
    namespace AudioSession
    {
        void SetPlayback(bool playback)
        {
            AVAudioSession* session = [AVAudioSession sharedInstance];
            NSError* err            = nil;
            [session setCategory:(playback ? AVAudioSessionCategoryPlayback : AVAudioSessionCategoryAmbient) error:&err];
            if(err)
                LWARN("AudioSession: couldn't set the category: %s", err.localizedDescription.UTF8String);
            [session setActive:YES error:&err];
        }

        double RequestSampleRate(double hz)
        {
            AVAudioSession* session = [AVAudioSession sharedInstance];
            NSError* err            = nil;
            if(hz > 0.0 && std::abs(session.sampleRate - hz) > 0.5)
            {
                [session setPreferredSampleRate:hz error:&err];
                // the preference applies on (re)activation
                [session setActive:YES error:&err];
            }
            return session.sampleRate;
        }

        static u32 s_Holds           = 0;
        static ALCcontext* s_Parked  = nullptr; // the context while it's taken down

        void SetOpenALHold(u32 hold, bool on)
        {
            const u32 before = s_Holds;
            s_Holds          = on ? (s_Holds | hold) : (s_Holds & ~hold);
            if(before == 0 && s_Holds != 0)
            {
                if(ALCcontext* ctx = alcGetCurrentContext())
                {
                    s_Parked = ctx;
                    alcMakeContextCurrent(nullptr); // Apple's OpenAL stops its output graph
                    alcSuspendContext(ctx);
                }
            }
            else if(before != 0 && s_Holds == 0 && s_Parked)
            {
                alcMakeContextCurrent(s_Parked);
                alcProcessContext(s_Parked);
                s_Parked = nullptr;
            }
        }

        double GetSampleRate() { return [AVAudioSession sharedInstance].sampleRate; }
        double GetOutputLatency() { return [AVAudioSession sharedInstance].outputLatency + [AVAudioSession sharedInstance].IOBufferDuration; }

        static AVAudioSessionPortDescription* CurrentOutput()
        {
            NSArray<AVAudioSessionPortDescription*>* outs = [AVAudioSession sharedInstance].currentRoute.outputs;
            return outs.count > 0 ? outs[0] : nil;
        }

        std::string GetRouteName()
        {
            AVAudioSessionPortDescription* p = CurrentOutput();
            return p ? std::string(p.portName.UTF8String) : std::string();
        }

        bool IsLossyRoute()
        {
            AVAudioSessionPortDescription* p = CurrentOutput();
            if(!p)
                return false;
            NSString* t = p.portType;
            return [t isEqualToString:AVAudioSessionPortBluetoothA2DP] || [t isEqualToString:AVAudioSessionPortBluetoothLE]
                || [t isEqualToString:AVAudioSessionPortBluetoothHFP] || [t isEqualToString:AVAudioSessionPortAirPlay];
        }
    }

    // iOS has one output at a time: the current route
    bool GetAudioOutputInfo(AudioOutputInfo& out)
    {
        out = {};
        AVAudioSessionPortDescription* p = AudioSession::CurrentOutput();
        if(!p)
            return false;
        NSString* t    = p.portType;
        out.ok         = true;
        out.uid        = p.UID ? std::string(p.UID.UTF8String) : std::string();
        out.name       = p.portName.UTF8String;
        out.isDefault  = true;
        out.sampleRate = (float)[AVAudioSession sharedInstance].sampleRate;
        if([t isEqualToString:AVAudioSessionPortUSBAudio])
            out.transport = "usb";
        else if([t isEqualToString:AVAudioSessionPortBluetoothA2DP] || [t isEqualToString:AVAudioSessionPortBluetoothLE] || [t isEqualToString:AVAudioSessionPortBluetoothHFP])
            out.transport = "bluetooth";
        else if([t isEqualToString:AVAudioSessionPortAirPlay])
            out.transport = "airplay";
        else if([t isEqualToString:AVAudioSessionPortHDMI])
            out.transport = "hdmi";
        else
            out.transport = "builtin";
        return true;
    }

    bool GetAudioOutputDevices(TDArray<AudioOutputInfo>& out)
    {
        out.Clear();
        AudioOutputInfo info;
        if(!GetAudioOutputInfo(info))
            return false;
        out.PushBack(info);
        return true;
    }

    bool SetAudioOutputSource(const std::string&, u32) { return false; }
    // iOS keeps the system volume to the hardware buttons / Control Centre
    bool GetAudioOutputVolume(const std::string&, float&) { return false; }
    bool SetAudioOutputVolume(const std::string&, float) { return false; }
}
