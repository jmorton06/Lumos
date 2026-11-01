#include "Precompiled.h"
#include "MidiInput.h"
#include "Maths/MathsUtilities.h"

#include <chrono>
#include <mutex>

#ifdef LUMOS_PLATFORM_MACOS
#include <CoreMIDI/CoreMIDI.h>
#endif

namespace Lumos
{
    namespace MidiInput
    {
        namespace
        {
            std::mutex s_Lock;
            TDArray<Event> s_Queue;

            double Now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

            void Push(const Event& e)
            {
                std::lock_guard<std::mutex> lock(s_Lock);
                if(s_Queue.Size() < 1024)
                    s_Queue.PushBack(e);
            }

            // Channel voice messages from a raw byte stream (running status, realtime bytes skipped)
            void ParseBytes(const u8* data, size_t size)
            {
                u8 status = 0;
                for(size_t i = 0; i < size;)
                {
                    u8 b = data[i];
                    if(b >= 0xF8) { i++; continue; } // clock / active sensing
                    if(b & 0x80)
                    {
                        status = b;
                        i++;
                        if(b >= 0xF0) { status = 0; continue; } // sysex etc: ignore
                    }
                    if(!status)
                    {
                        i++;
                        continue;
                    }
                    u8 kind = status & 0xF0, ch = status & 0x0F;
                    int need = (kind == 0xC0 || kind == 0xD0) ? 1 : 2;
                    if(i + need > size)
                        break;
                    u8 d1 = data[i], d2 = need > 1 ? data[i + 1] : 0;
                    i += need;
                    Event e;
                    e.channel = ch;
                    e.time    = Now();
                    if(kind == 0x90 && d2 > 0)
                    {
                        e.type = EventType::NoteOn, e.number = d1, e.value = d2;
                        Push(e);
                    }
                    else if(kind == 0x80 || (kind == 0x90 && d2 == 0))
                    {
                        e.type = EventType::NoteOff, e.number = d1, e.value = 0;
                        Push(e);
                    }
                    else if(kind == 0xB0)
                    {
                        e.type = EventType::Control, e.number = d1, e.value = d2;
                        Push(e);
                    }
                    else if(kind == 0xE0)
                    {
                        e.type = EventType::PitchBend, e.value = (i32)((d2 << 7) | d1) - 8192;
                        Push(e);
                    }
                }
            }

#ifdef LUMOS_PLATFORM_MACOS
            MIDIClientRef s_Client = 0;
            MIDIPortRef s_Port     = 0;
            TDArray<MIDIEndpointRef> s_Connected;

            void ReadProc(const MIDIPacketList* list, void*, void*)
            {
                const MIDIPacket* p = &list->packet[0];
                for(u32 i = 0; i < list->numPackets; i++)
                {
                    ParseBytes(p->data, p->length);
                    p = MIDIPacketNext(p);
                }
            }

            void ConnectAll()
            {
                ItemCount n = MIDIGetNumberOfSources();
                for(ItemCount i = 0; i < n; i++)
                {
                    MIDIEndpointRef src = MIDIGetSource(i);
                    bool have           = false;
                    for(auto c : s_Connected)
                        if(c == src)
                            have = true;
                    if(!have && MIDIPortConnectSource(s_Port, src, nullptr) == noErr)
                        s_Connected.PushBack(src);
                }
            }

            void NotifyProc(const MIDINotification* n, void*)
            {
                if(n->messageID == kMIDIMsgSetupChanged)
                    ConnectAll();
            }
#endif
        }

#ifdef LUMOS_PLATFORM_MACOS
        bool IsSupported() { return true; }

        bool Start()
        {
            if(s_Client)
                return true;
            if(MIDIClientCreate(CFSTR("Lumos"), NotifyProc, nullptr, &s_Client) != noErr)
                return false;
            if(MIDIInputPortCreate(s_Client, CFSTR("Lumos In"), ReadProc, nullptr, &s_Port) != noErr)
                return false;
            ConnectAll();
            return true;
        }

        void Stop()
        {
            if(!s_Client)
                return;
            MIDIPortDispose(s_Port);
            MIDIClientDispose(s_Client);
            s_Client = 0, s_Port = 0;
            s_Connected.Clear();
        }

        void GetDevices(TDArray<Device>& out)
        {
            out.Clear();
            ItemCount n = MIDIGetNumberOfSources();
            for(ItemCount i = 0; i < n; i++)
            {
                MIDIEndpointRef src = MIDIGetSource(i);
                CFStringRef name    = nullptr;
                Device d;
                if(MIDIObjectGetStringProperty(src, kMIDIPropertyDisplayName, &name) == noErr && name)
                {
                    char buf[256] = {};
                    CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8);
                    d.name = buf;
                    CFRelease(name);
                }
                SInt32 offline = 0;
                MIDIObjectGetIntegerProperty(src, kMIDIPropertyOffline, &offline);
                d.online = offline == 0;
                out.PushBack(d);
            }
        }
#else
        bool IsSupported() { return false; }
        bool Start() { return false; }
        void Stop() { }
        void GetDevices(TDArray<Device>& out) { out.Clear(); }
#endif

        u32 Poll(Event* out, u32 max)
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            u32 n = (u32)Maths::Min<size_t>(max, s_Queue.Size());
            for(u32 i = 0; i < n; i++)
                out[i] = s_Queue[i];
            for(u32 i = n; i < s_Queue.Size(); i++)
                s_Queue[i - n] = s_Queue[i];
            s_Queue.Resize(s_Queue.Size() - n);
            return n;
        }

        void InjectBytes(const u8* data, size_t size) { ParseBytes(data, size); }

        void Inject(const Event& e)
        {
            Event copy = e;
            if(copy.time == 0.0)
                copy.time = Now();
            Push(copy);
        }
    }
}
