#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include "AudioLua.h"
#include "LuaSmartPtrTraits.h"
#include "Audio/MusicStream.h"
#include "Audio/Equaliser.h"
#include "Audio/SoundChain.h"
#include "Audio/AudioDecoder.h"
#include "Audio/AudioTags.h"
#include "Graphics/RHI/Texture.h"
#include "Utilities/LoadImage.h"
#include "Graphics/ShaderPreview.h"
#include "Audio/Synth.h"
#include "Audio/AudioAnalysis.h"
#include "Audio/AudioOutput.h"
#include "Audio/NowPlaying.h"
#include "Audio/NetStreamDecoder.h"
#include "Audio/RemoteAudio.h"
#include "Audio/MidiInput.h"
#include "Audio/AudioSession.h"
#include "Audio/AudioTagReader.h"
#include "Audio/AudioTagWriter.h"
#include "Utilities/AsyncImageLoader.h"

namespace Lumos
{
    static SharedPtr<Graphics::Texture2D> TextureFromEncoded(const u8* data, size_t size, u32 maxSize)
    {
        u32 w = 0, h = 0;
        u8* pixels = LoadImageFromMemory(data, size, &w, &h, maxSize);
        if(!pixels)
            return nullptr;

        Graphics::TextureDesc desc;
        desc.format    = Graphics::RHIFormat::R8G8B8A8_Unorm;
        desc.minFilter = Graphics::TextureFilter::LINEAR;
        desc.magFilter = Graphics::TextureFilter::LINEAR;
        desc.wrap      = Graphics::TextureWrap::CLAMP_TO_EDGE;
        desc.srgb      = false; // match LoadTexture2D
        SharedPtr<Graphics::Texture2D> tex(Graphics::Texture2D::CreateFromSource(w, h, pixels, desc));
        FreeImageMemory(pixels);
        return tex;
    }

    // Lua handle: an engine plus (for live synths) the stream playing it
    struct LuaSynth
    {
        SharedPtr<SynthEngine> Engine;
        SharedPtr<MusicStream> Stream;
    };

    static float SynthValue(SynthEngine::Param p, const sol::object& v)
    {
        if(v.is<std::string>())
        {
            float f = SynthEngine::ValueFromName(p, v.as<std::string>().c_str());
            if(f < 0.0f)
                LWARN("Synth: unknown value '%s' for %s", v.as<std::string>().c_str(), SynthEngine::ParamName(p));
            return Maths::Max(0.0f, f);
        }
        if(v.is<bool>())
            return v.as<bool>() ? 1.0f : 0.0f;
        return v.is<double>() ? (float)v.as<double>() : 0.0f;
    }

    static void SynthSetTable(SynthEngine& e, i32 inst, const sol::table& t)
    {
        for(auto& kv : t)
        {
            if(!kv.first.is<std::string>())
                continue;
            std::string key = kv.first.as<std::string>();
            SynthEngine::Param p = SynthEngine::ParamFromName(key.c_str());
            if(p == SynthEngine::P_Count)
            {
                LWARN("Synth: unknown instrument param '%s'", key.c_str());
                continue;
            }
            e.SetParam(inst, p, SynthValue(p, kv.second));
        }
    }

    static void SynthSetEffect(SynthEngine& e, i32 bus, i32 fx, SynthEngine::EffectType type, const std::string& name, const sol::object& v)
    {
        i32 idx = SynthEngine::EffectParamFromName(type, name.c_str());
        if(idx < 0)
        {
            LWARN("Synth: unknown effect param '%s'", name.c_str());
            return;
        }
        float value = v.is<std::string>() ? Maths::Max(0.0f, SynthEngine::ValueFromName(SynthEngine::P_FilterMode, v.as<std::string>().c_str()))
                                          : (v.is<bool>() ? (v.as<bool>() ? 1.0f : 0.0f) : (float)v.as<double>());
        e.SetEffectParam(bus, fx, (u32)idx, value);
    }

    static void BindSynthLua(sol::state& state)
    {
        auto synth = state.new_usertype<LuaSynth>("Synth", sol::no_constructor);
        // Live synth: plays through its own low-latency stream (~45ms). relaxed = latency doesn't
        // matter (generated music, ambience): a deeper buffer that follows the chosen output device
        synth["new"] = [](sol::optional<int> sampleRate, sol::optional<bool> relaxed) -> SharedPtr<LuaSynth>
        {
            SharedPtr<LuaSynth> s = CreateSharedPtr<LuaSynth>();
            s->Engine             = CreateSharedPtr<SynthEngine>((u32)sampleRate.value_or(44100));
            const bool deep       = relaxed.value_or(false);
            s->Stream             = MusicStream::CreateFromDecoder("synth", SynthEngine::CreateDecoder(s->Engine), deep ? 4096 : 512, deep ? 4 : 4);
            return s;
        };
        // Offline synth: Render(seconds) then SaveWav(path)
        synth["newOffline"] = [](sol::optional<int> sampleRate) -> SharedPtr<LuaSynth>
        {
            SharedPtr<LuaSynth> s = CreateSharedPtr<LuaSynth>();
            s->Engine             = CreateSharedPtr<SynthEngine>((u32)sampleRate.value_or(44100));
            return s;
        };

        synth["Play"]      = [](LuaSynth& s) { if(s.Stream) s.Stream->Play(); };
        synth["Stop"]      = [](LuaSynth& s) { if(s.Stream) s.Stream->Pause(); };
        synth["IsPlaying"] = [](LuaSynth& s) { return s.Stream && s.Stream->IsPlaying(); };
        synth["SetVolume"] = [](LuaSynth& s, float v) { if(s.Stream) s.Stream->SetVolume(v); };
        synth["GetVolume"] = [](LuaSynth& s) { return s.Stream ? s.Stream->GetVolume() : 0.0f; };
        synth["GetStream"] = [](LuaSynth& s) { return s.Stream; };
        synth["Now"]       = [](LuaSynth& s) { return s.Engine->Now(); };
        synth["GetSampleRate"] = [](LuaSynth& s) { return s.Engine->GetSampleRate(); };

        synth["AddInstrument"] = [](LuaSynth& s, sol::optional<sol::table> params)
        {
            i32 id = s.Engine->AddInstrument();
            if(params)
                SynthSetTable(*s.Engine, id, *params);
            return id;
        };
        synth["RemoveInstrument"] = [](LuaSynth& s, int inst) { s.Engine->RemoveInstrument(inst); };
        // Set(inst, "cutoff", 800) or Set(inst, { cutoff = 800, osc1 = "square" })
        synth["Set"] = [](LuaSynth& s, int inst, sol::object key, sol::optional<sol::object> value)
        {
            if(key.is<sol::table>())
                SynthSetTable(*s.Engine, inst, key.as<sol::table>());
            else if(key.is<std::string>() && value)
            {
                auto p = SynthEngine::ParamFromName(key.as<std::string>().c_str());
                if(p != SynthEngine::P_Count)
                    s.Engine->SetParam(inst, p, SynthValue(p, *value));
                else
                    LWARN("Synth: unknown instrument param '%s'", key.as<std::string>().c_str());
            }
        };
        synth["Get"] = [](LuaSynth& s, int inst, const std::string& key)
        {
            auto p = SynthEngine::ParamFromName(key.c_str());
            return p == SynthEngine::P_Count ? 0.0f : s.Engine->GetParam(inst, p);
        };
        // Ramp(inst, "cutoff", 2000, time, seconds)
        synth["Ramp"] = [](LuaSynth& s, int inst, const std::string& key, float value, double time, sol::optional<float> seconds)
        {
            auto p = SynthEngine::ParamFromName(key.c_str());
            if(p != SynthEngine::P_Count)
                s.Engine->ScheduleParam(inst, p, value, time, seconds.value_or(0.0f));
        };
        synth["NoteOn"] = [](LuaSynth& s, int inst, float note, sol::optional<float> vel, sol::optional<double> time, sol::optional<float> duration)
        { s.Engine->NoteOn(inst, note, vel.value_or(1.0f), time.value_or(0.0), duration.value_or(0.0f)); };
        synth["NoteOff"] = [](LuaSynth& s, int inst, float note, sol::optional<double> time)
        { s.Engine->NoteOff(inst, note, time.value_or(0.0)); };
        synth["AllNotesOff"]    = [](LuaSynth& s, sol::optional<int> inst) { s.Engine->AllNotesOff(inst.value_or(-1)); };
        synth["ClearScheduled"] = [](LuaSynth& s, sol::optional<int> inst) { s.Engine->ClearScheduled(inst.value_or(-1)); };

        synth["AddBus"]        = [](LuaSynth& s) { return s.Engine->AddBus(); };
        synth["SetBusVolume"]  = [](LuaSynth& s, int bus, float v) { s.Engine->SetBusVolume(bus, v); };
        synth["RampBusVolume"] = [](LuaSynth& s, int bus, float v, double time, sol::optional<float> seconds)
        { s.Engine->ScheduleBusVolume(bus, v, time, seconds.value_or(0.0f)); };
        synth["SetBusPan"]     = [](LuaSynth& s, int bus, float pan) { s.Engine->SetBusPan(bus, pan); };
        synth["SetBusOutput"]  = [](LuaSynth& s, int bus, int out) { s.Engine->SetBusOutput(bus, out); };
        synth["GetBusLevel"]   = [](LuaSynth& s, int bus) { return s.Engine->GetBusLevel(bus); };

        // AddEffect(bus, "reverb", { size = 0.8, mix = 0.3 }) -> effect index on that bus
        synth["AddEffect"] = [](LuaSynth& s, int bus, const std::string& type, sol::optional<sol::table> params) -> int
        {
            auto t = SynthEngine::EffectFromName(type.c_str());
            if(t == SynthEngine::FX_Count)
            {
                LWARN("Synth: unknown effect '%s'", type.c_str());
                return -1;
            }
            i32 fx = s.Engine->AddEffect(bus, t);
            if(fx >= 0 && params)
                for(auto& kv : *params)
                    if(kv.first.is<std::string>())
                        SynthSetEffect(*s.Engine, bus, fx, t, kv.first.as<std::string>(), kv.second);
            return fx;
        };
        // SetEffect(bus, fx, "reverb", "mix", 0.4) - the type names the params
        synth["SetEffect"] = [](LuaSynth& s, int bus, int fx, const std::string& type, const std::string& name, sol::object value)
        {
            auto t = SynthEngine::EffectFromName(type.c_str());
            if(t != SynthEngine::FX_Count)
                SynthSetEffect(*s.Engine, bus, fx, t, name, value);
        };
        synth["EnableEffect"] = [](LuaSynth& s, int bus, int fx, bool on) { s.Engine->SetEffectEnabled(bus, fx, on); };
        synth["ClearEffects"] = [](LuaSynth& s, int bus) { s.Engine->ClearEffects(bus); };

        synth["LoadSample"]      = [](LuaSynth& s, const std::string& path, sol::optional<float> maxSeconds) { return s.Engine->LoadSample(path, maxSeconds.value_or(30.0f)); };
        synth["GetSampleLength"] = [](LuaSynth& s, int id) { return s.Engine->GetSampleLength(id); };

        synth["Render"]           = [](LuaSynth& s, double seconds) { s.Engine->Render(seconds); };
        synth["SaveWav"]          = [](LuaSynth& s, const std::string& path) { return s.Engine->SaveWav(path); };
        synth["GetCaptureLength"] = [](LuaSynth& s) { return s.Engine->GetCaptureLength(); };
        synth["ClearCapture"]     = [](LuaSynth& s) { s.Engine->ClearCapture(); };

        // ---- analysis ----
        static const char* keyNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        auto toResult = [](sol::state_view lua, const AudioAnalysisResult& r) -> sol::object
        {
            if(!r.ok)
                return sol::make_object(lua, false);
            sol::table t        = lua.create_table();
            t["bpm"]            = r.bpm;
            t["beatStrength"]   = r.beatStrength;
            t["key"]            = r.key;
            t["minor"]          = r.minor;
            t["keyName"]        = std::string(r.key >= 0 ? keyNames[r.key] : "?") + (r.minor ? " minor" : " major");
            t["keyConfidence"]  = r.keyConfidence;
            t["energy"]         = r.energy;
            t["brightness"]     = r.brightness;
            t["duration"]       = r.duration;
            t["loudness"]       = r.loudness;
            t["peakDb"]         = r.peakDb;
            return t;
        };
        // spectrogram -> texture here: GPU uploads have to happen on the main thread
        auto toQuality = [](sol::state_view lua, const AudioQualityResult& r) -> sol::object
        {
            if(!r.ok)
                return sol::make_object(lua, false);
            sol::table t          = lua.create_table();
            t["codec"]            = r.codec;
            t["lossless"]         = r.lossless;
            t["floatSource"]      = r.floatSource;
            t["sourceBits"]       = r.sourceBits;
            t["effectiveBits"]    = r.effectiveBits;
            t["sampleRate"]       = r.sampleRate;
            t["channels"]         = r.channels;
            t["duration"]         = r.duration;
            t["integratedLufs"]   = r.integratedLufs;
            t["loudnessRange"]    = r.loudnessRange;
            t["samplePeakDb"]     = r.samplePeakDb;
            t["truePeakDb"]       = r.truePeakDb;
            t["crestDb"]          = r.crestDb;
            t["dynamicRange"]     = r.dynamicRange;
            t["clipEvents"]       = r.clipEvents;
            t["clippedSamples"]   = (double)r.clippedSamples;
            t["dcOffset"]         = r.dcOffset;
            t["correlation"]      = r.correlation;
            t["sideDb"]           = r.sideDb;
            t["cutoffHz"]         = r.cutoffHz;
            if(r.specWidth > 0 && !r.spectrogram.Empty())
            {
                Graphics::TextureDesc desc;
                desc.format    = Graphics::RHIFormat::R8G8B8A8_Unorm;
                desc.minFilter = Graphics::TextureFilter::LINEAR;
                desc.magFilter = Graphics::TextureFilter::LINEAR;
                desc.wrap      = Graphics::TextureWrap::CLAMP_TO_EDGE;
                desc.srgb      = false;
                SharedPtr<Graphics::Texture2D> tex(Graphics::Texture2D::CreateFromSource(r.specWidth, r.specHeight, (void*)r.spectrogram.Data(), desc));
                t["spectrogram"] = tex;
            }
            return t;
        };
        auto an = state["AudioAnalysis"].get_or_create<sol::table>();
        // Blocking; use Queue/Poll for libraries
        an.set_function("Analyse", [toResult](const std::string& path, sol::optional<float> maxSeconds, sol::this_state ts)
                        {
            AudioAnalysisResult r;
            AnalyseAudioFile(path, r, maxSeconds.value_or(90.0f));
            return toResult(ts, r); });
        an.set_function("Queue", [](const std::string& path) { return AudioAnalyser::Queue(path); });
        // nil while pending, false if it failed, else the result table
        an.set_function("Poll", [toResult](u32 id, sol::this_state ts) -> sol::object
                        {
            AudioAnalysisResult r;
            if(!AudioAnalyser::Poll(id, r))
                return sol::lua_nil;
            return toResult(ts, r); });
        an.set_function("AnalyseQuality", [toQuality](const std::string& path, sol::optional<int> w, sol::optional<int> h, sol::this_state ts)
                        {
            AudioQualityResult r;
            AnalyseAudioQuality(path, r, w.value_or(0), h.value_or(0));
            return toQuality(ts, r); });
        an.set_function("QueueQuality", [](const std::string& path, sol::optional<int> w, sol::optional<int> h)
                        { return AudioAnalyser::QueueQuality(path, w.value_or(0), h.value_or(0)); });
        an.set_function("PollQuality", [toQuality](u32 id, sol::this_state ts) -> sol::object
                        {
            AudioQualityResult r;
            if(!AudioAnalyser::PollQuality(id, r))
                return sol::lua_nil;
            return toQuality(ts, r); });
        an.set_function("Pending", []() { return AudioAnalyser::Pending(); });

        // ---- OS now playing widget + media keys ----
        static const char* cmdNames[] = { "play", "pause", "toggle", "stop", "next", "previous", "seek" };
        auto np = state["NowPlaying"].get_or_create<sol::table>();
        np.set_function("IsSupported", []() { return NowPlaying::IsSupported(); });
        np.set_function("Set", [](sol::table t)
                        {
            NowPlaying::Info info;
            info.title        = t.get_or<std::string>("title", "");
            info.artist       = t.get_or<std::string>("artist", "");
            info.album        = t.get_or<std::string>("album", "");
            info.duration     = t.get_or("duration", 0.0);
            info.position     = t.get_or("position", 0.0);
            info.playing      = t.get_or("playing", false);
            info.seekable     = t.get_or("seekable", true);
            info.hasNext      = t.get_or("hasNext", true);
            info.hasPrevious  = t.get_or("hasPrevious", true);
            info.artworkImage = t.get_or<std::string>("artworkImage", "");
            info.artworkAudio = t.get_or<std::string>("artworkAudio", "");
            NowPlaying::Set(info); });
        np.set_function("Clear", []() { NowPlaying::Clear(); });
        // { { type = "toggle" | "play" | ... , position = seconds (seek) }, ... }
        np.set_function("Poll", [](sol::this_state ts)
                        {
            sol::state_view lua(ts);
            sol::table list = lua.create_table();
            NowPlaying::Command cmds[16];
            u32 n = NowPlaying::Poll(cmds, 16);
            for(u32 i = 0; i < n; i++)
            {
                sol::table c = lua.create_table();
                c["type"]     = cmdNames[(int)cmds[i].type];
                c["position"] = cmds[i].position;
                list[i + 1]   = c;
            }
            return list; });
        np.set_function("GetCurrent", [](sol::this_state ts) -> sol::object
                        {
            NowPlaying::Info info;
            bool art = false;
            if(!NowPlaying::GetCurrent(info, art))
                return sol::lua_nil;
            sol::table t = sol::state_view(ts).create_table();
            t["title"] = info.title, t["artist"] = info.artist, t["album"] = info.album;
            t["duration"] = info.duration, t["position"] = info.position, t["playing"] = info.playing, t["hasArtwork"] = art;
            return t; });
        np.set_function("Inject", [](const std::string& type, sol::optional<double> pos)
                        {
            for(int i = 0; i < 7; i++)
                if(type == cmdNames[i])
                    NowPlaying::Inject((NowPlaying::CommandType)i, pos.value_or(0.0)); });

        auto out = state["AudioOutput"].get_or_create<sol::table>();
        // nil when the platform can't say
        auto deviceTable = [](sol::state_view lua, const AudioOutputInfo& info) -> sol::table
        {
            sol::table t       = lua.create_table();
            t["uid"]           = info.uid;
            t["name"]          = info.name;
            t["transport"]     = info.transport;
            t["isDefault"]     = info.isDefault;
            t["sampleRate"]    = info.sampleRate;
            t["physicalBits"]  = info.physicalBits;
            t["hogged"]        = info.hogPid != -1;
            sol::table rates   = lua.create_table();
            for(u32 i = 0; i < info.sampleRates.Size(); i++)
                rates[i + 1] = info.sampleRates[i];
            t["sampleRates"]   = rates;
            sol::table sources = lua.create_table();
            for(u32 i = 0; i < info.sources.Size(); i++)
            {
                sol::table src = lua.create_table();
                src["id"]      = info.sources[i].id;
                src["name"]    = info.sources[i].name;
                sources[i + 1] = src;
            }
            t["sources"]       = sources;
            t["currentSource"] = info.currentSource;
            return t;
        };
        // nil when the platform can't say
        out.set_function("GetInfo", [deviceTable](sol::this_state ts) -> sol::object
                         {
            AudioOutputInfo info;
            if(!GetAudioOutputInfo(info))
                return sol::lua_nil;
            return deviceTable(ts, info); });
        out.set_function("GetDevices", [deviceTable](sol::this_state ts)
                         {
            sol::state_view lua(ts);
            sol::table list = lua.create_table();
            TDArray<AudioOutputInfo> devices;
            GetAudioOutputDevices(devices);
            for(u32 i = 0; i < devices.Size(); i++)
                list[i + 1] = deviceTable(lua, devices[i]);
            return list; });
        out.set_function("SetSource", [](const std::string& uid, u32 id) { return SetAudioOutputSource(uid, id); });
        // the device's own volume 0..1 (uid "" = system default); GetVolume is nil when it has no control
        out.set_function("GetVolume", [](sol::optional<std::string> uid) -> sol::optional<float>
                         {
            float v = 0.0f;
            if(!GetAudioOutputVolume(uid.value_or(""), v))
                return sol::nullopt;
            return v; });
        out.set_function("SetVolume", [](const std::string& uid, float v) { return SetAudioOutputVolume(uid, v); });
        // the system AirPlay / output picker, pointing at a window position (pixels)
        out.set_function("ShowRoutePicker", [](float x, float y) { return ShowAudioRoutePicker(x, y); });
        // { device = uid or "", matchRate, exclusive, native, matchBits, dither } - applies to streams created afterwards
        out.set_function("SetConfig", [](sol::table t)
                         {
            MusicOutputConfig c;
            c.deviceUID = t.get_or<std::string>("device", "");
            c.matchRate = t.get_or("matchRate", false);
            c.exclusive = t.get_or("exclusive", false);
            c.native    = t.get_or("native", false);
            c.matchBits = t.get_or("matchBits", false);
            c.dither    = t.get_or("dither", true);
            c.dop       = t.get_or("dop", false);
            MusicStream::SetOutputConfig(c); });
        out.set_function("GetConfig", [](sol::this_state ts)
                         {
            const MusicOutputConfig& c = MusicStream::GetOutputConfig();
            sol::table t = sol::state_view(ts).create_table();
            t["device"]    = c.deviceUID;
            t["matchRate"] = c.matchRate;
            t["exclusive"] = c.exclusive;
            t["native"]    = c.native;
            t["matchBits"] = c.matchBits;
            t["dither"]    = c.dither;
            t["dop"]       = c.dop;
            return t; });
        an.set_function("ClearQueue", []() { AudioAnalyser::ClearQueue(); });
    }

    void BindAudioLua(sol::state& state)
    {
        BindSynthLua(state);
        auto stream = state.new_usertype<MusicStream>("MusicStream", sol::no_constructor);
        stream["Create"]        = [](const std::string& path) -> SharedPtr<MusicStream> { return MusicStream::Create(path); };
        stream["IsSupported"]   = [](const std::string& ext) { return AudioDecoder::IsSupportedExtension(ext); };
        stream["Play"]          = &MusicStream::Play;
        stream["Pause"]         = &MusicStream::Pause;
        stream["Stop"]          = &MusicStream::Stop;
        stream["Seek"]          = &MusicStream::Seek;
        stream["IsPlaying"]     = &MusicStream::IsPlaying;
        stream["IsPaused"]      = &MusicStream::IsPaused;
        stream["IsFinished"]    = &MusicStream::IsFinished;
        stream["GetPosition"]   = &MusicStream::GetPosition;
        stream["GetDuration"]   = &MusicStream::GetDuration;
        stream["SetVolume"]     = &MusicStream::SetVolume;
        stream["GetVolume"]     = &MusicStream::GetVolume;
        stream["SetPitch"]      = &MusicStream::SetPitch;
        stream["SetSpeed"]      = &MusicStream::SetSpeed; // pitch kept
        // gapless: QueueNext(path) -> bool, ClearNext(), GetTrackIndex() -> files started since open / seek
        {
            // 10-band graphic EQ for every music stream: Equaliser.Set({10 dB gains}, preampDb), SetEnabled, Frequencies
            sol::table eq = state.create_named_table("Equaliser");
            eq.set_function("Set", [](sol::table gains, sol::optional<float> preamp)
                            {
                float g[Equaliser::Bands] = {};
                for(int i = 0; i < Equaliser::Bands; i++)
                    g[i] = gains.get_or(i + 1, 0.0f);
                Equaliser::Set(g, preamp.value_or(0.0f)); });
            eq.set_function("SetEnabled", [](bool on) { Equaliser::SetEnabled(on); });
            eq.set_function("IsActive", []() { return Equaliser::IsActive(); });
            eq.set_function("Frequencies", [](sol::this_state ts)
                            {
                sol::state_view lua(ts);
                sol::table t = lua.create_table();
                for(int i = 0; i < Equaliser::Bands; i++)
                    t[i + 1] = Equaliser::BandFrequencies()[i];
                return t; });
        }
        // Effects chain for music: SoundChain.Set({ stages = { { type = "eq"|"crossfeed"|"bass"|"room"|"tape"|"correction"|"limiter",
        // enabled, amount, param }, ... }, correction = { preamp = dB, filters = { { type = "peak"|"lowshelf"|"highshelf",
        // freq, gain, q }, ... } } }); SoundChain.Get() returns the same shape
        {
            sol::table sc = state.create_named_table("SoundChain");
            auto typeOf = [](const std::string& n) {
                for(u32 i = 0; i < (u32)SoundChain::StageType::Count; i++)
                    if(n == SoundChain::StageName((SoundChain::StageType)i))
                        return (SoundChain::StageType)i;
                return SoundChain::StageType::Count;
            };
            sc.set_function("Set", [typeOf](sol::table t)
                            {
                SoundChain::Config c;
                if(auto stages = t.get<sol::optional<sol::table>>("stages"))
                    for(auto& kv : *stages)
                    {
                        if(!kv.second.is<sol::table>())
                            continue;
                        sol::table st = kv.second.as<sol::table>();
                        SoundChain::Stage s;
                        s.type = typeOf(st.get_or<std::string>("type", ""));
                        if(s.type == SoundChain::StageType::Count)
                            continue;
                        s.enabled = st.get_or("enabled", true);
                        s.amount  = st.get_or("amount", 0.5f);
                        s.param   = st.get_or("param", 0.5f);
                        c.stages.PushBack(s);
                    }
                if(auto corr = t.get<sol::optional<sol::table>>("correction"))
                {
                    c.correctionPreampDb = corr->get_or("preamp", 0.0f);
                    if(auto filters = corr->get<sol::optional<sol::table>>("filters"))
                        for(auto& kv : *filters)
                        {
                            if(!kv.second.is<sol::table>())
                                continue;
                            sol::table ft = kv.second.as<sol::table>();
                            SoundChain::Filter f;
                            std::string ty = ft.get_or<std::string>("type", "peak");
                            f.type   = ty == "lowshelf" ? SoundChain::FilterType::LowShelf : (ty == "highshelf" ? SoundChain::FilterType::HighShelf : SoundChain::FilterType::Peak);
                            f.freq   = ft.get_or("freq", 1000.0f);
                            f.gainDb = ft.get_or("gain", 0.0f);
                            f.q      = ft.get_or("q", 0.707f);
                            c.correction.PushBack(f);
                        }
                }
                SoundChain::Set(c); });
            sc.set_function("HasActiveEffects", []() { return SoundChain::HasActiveEffects(); });
            // whether this build has a stage type
            sc.set_function("Has", [typeOf](const std::string& type) { return typeOf(type) != SoundChain::StageType::Count; });
        }
        stream["SetVarispeed"]  = &MusicStream::SetVarispeed; // speed + pitch together (tape-style)
        stream["GetVarispeed"]  = &MusicStream::GetVarispeed;
        stream["QueueNext"]     = &MusicStream::QueueNext;
        stream["ClearNext"]     = &MusicStream::ClearNext;
        stream["GetTrackIndex"] = &MusicStream::GetTrackIndex;
        stream["GetSpeed"]      = &MusicStream::GetSpeed;
        stream["GetPitch"]      = &MusicStream::GetPitch;
        stream["SetLooping"]    = &MusicStream::SetLooping;
        stream["GetLooping"]    = &MusicStream::GetLooping;
        stream["GetSampleRate"] = &MusicStream::GetSampleRate;
        stream["GetChannels"]   = &MusicStream::GetChannels;
        stream["GetPath"]       = &MusicStream::GetPath;
        stream["GetLevel"]      = &MusicStream::GetLevel;
        stream["GetMeters"]     = [](MusicStream& s, sol::this_state ts)
        {
            MusicMeters m = s.GetMeters();
            sol::table t  = sol::state_view(ts).create_table();
            t["peakL"] = m.peak[0], t["peakR"] = m.peak[1], t["rmsL"] = m.rms[0], t["rmsR"] = m.rms[1];
            return t;
        };
        stream["GetStreamTitle"] = [](MusicStream& s) { return s.GetStreamTitle(); };
        stream["IsBuffering"]    = [](MusicStream& s) { return s.IsBuffering(); };

        // ---- MIDI in ----
        static const char* midiTypes[] = { "noteon", "noteoff", "cc", "bend" };
        auto midi = state["Midi"].get_or_create<sol::table>();
        midi.set_function("IsSupported", []() { return MidiInput::IsSupported(); });
        midi.set_function("Start", []() { return MidiInput::Start(); });
        midi.set_function("Stop", []() { MidiInput::Stop(); });
        midi.set_function("GetDevices", [](sol::this_state ts)
                          {
            sol::state_view lua(ts);
            TDArray<MidiInput::Device> devs;
            MidiInput::GetDevices(devs);
            sol::table out = lua.create_table();
            for(u32 i = 0; i < devs.Size(); i++)
            {
                sol::table d = lua.create_table();
                d["name"] = devs[i].name, d["online"] = devs[i].online;
                out[i + 1] = d;
            }
            return out; });
        // { { type = "noteon" | "noteoff" | "cc" | "bend", channel, number, value, time }, ... }
        midi.set_function("Poll", [](sol::this_state ts)
                          {
            sol::state_view lua(ts);
            sol::table out = lua.create_table();
            MidiInput::Event ev[64];
            u32 n = MidiInput::Poll(ev, 64);
            for(u32 i = 0; i < n; i++)
            {
                sol::table e = lua.create_table();
                e["type"] = midiTypes[(int)ev[i].type], e["channel"] = ev[i].channel, e["number"] = ev[i].number;
                e["value"] = ev[i].value, e["time"] = ev[i].time;
                out[i + 1] = e;
            }
            return out; });
        midi.set_function("InjectBytes", [](const std::string& bytes) { MidiInput::InjectBytes((const u8*)bytes.data(), bytes.size()); });
        midi.set_function("Inject", [](const std::string& type, int number, sol::optional<int> value)
                          {
            MidiInput::Event e;
            e.type   = type == "noteoff" ? MidiInput::EventType::NoteOff : (type == "cc" ? MidiInput::EventType::Control : (type == "bend" ? MidiInput::EventType::PitchBend : MidiInput::EventType::NoteOn));
            e.number = (u8)Maths::Clamp(number, 0, 127);
            e.value  = value.value_or(100);
            MidiInput::Inject(e); });

        // ---- internet radio (MP3 streams) ----
        // Radio.Open(url) -> id; Radio.Poll(id) -> nil while connecting, false + error, or a MusicStream + station name
        static std::unordered_map<u32, NetStreamDecoder*> s_Radio;
        static u32 s_RadioId = 1;
        auto radio = state["Radio"].get_or_create<sol::table>();
        radio.set_function("Open", [](const std::string& url)
                           {
            u32 id = s_RadioId++;
            s_Radio[id] = new NetStreamDecoder(url);
            return id; });
        radio.set_function("Poll", [](u32 id, sol::this_state ts) -> std::tuple<sol::object, sol::object>
                           {
            sol::state_view lua(ts);
            auto it = s_Radio.find(id);
            if(it == s_Radio.end())
                return { sol::make_object(lua, false), sol::make_object(lua, std::string("unknown id")) };
            NetStreamDecoder* d = it->second;
            auto st = d->GetState();
            if(st == NetStreamDecoder::State::Connecting)
                return { sol::lua_nil, sol::lua_nil };
            s_Radio.erase(it);
            if(st != NetStreamDecoder::State::Ready)
            {
                std::string err = d->GetError();
                delete d;
                return { sol::make_object(lua, false), sol::make_object(lua, err.empty() ? std::string("stream ended") : err) };
            }
            std::string name = d->GetStationName();
            auto ms = MusicStream::CreateFromDecoder("radio", d, 4096, 6); // takes the decoder
            if(!ms)
                return { sol::make_object(lua, false), sol::make_object(lua, std::string("no audio output")) };
            return { sol::make_object(lua, ms), sol::make_object(lua, name) }; });
        radio.set_function("Cancel", [](u32 id)
                           {
            auto it = s_Radio.find(id);
            if(it == s_Radio.end())
                return;
            delete it->second;
            s_Radio.erase(it); });

        // ---- media server songs: http(s) URLs streamed through a cache file ----
        // RemoteAudio.Prepare(url, cachePath [, headers, decode = true]); MusicStream.Create(url) then
        // starts at once. Status(url) -> { state, error, have, total, complete }. decode = false is a
        // plain download with progress.
        auto remote = state["RemoteAudio"].get_or_create<sol::table>();
        remote.set_function("Prepare", [](const std::string& url, const std::string& cachePath, sol::optional<sol::table> headers, sol::optional<bool> decode)
                            {
            Http::Headers h;
            if(headers)
                for(auto& kv : *headers)
                    if(kv.first.is<std::string>() && kv.second.is<std::string>())
                        h.push_back({ kv.first.as<std::string>(), kv.second.as<std::string>() });
            RemoteAudio::Prepare(url, cachePath, h, decode.value_or(true)); });
        remote.set_function("Status", [](const std::string& url, sol::this_state ts)
                            {
            RemoteAudio::Status st = RemoteAudio::GetStatus(url);
            sol::table t = sol::state_view(ts).create_table();
            t["state"]    = std::string(st.state);
            t["error"]    = st.error;
            t["have"]     = (double)st.have;
            t["total"]    = (double)st.total;
            t["complete"] = st.complete;
            return t; });
        remote.set_function("Release", [](const std::string& url) { RemoteAudio::Release(url); });

        stream["GetOutputStatus"] = [](MusicStream& s, sol::this_state ts)
        {
            MusicOutputStatus st = s.GetOutputStatus();
            sol::table t         = sol::state_view(ts).create_table();
            t["backend"]     = std::string(st.backend);
            t["device"]      = st.deviceName;
            t["fileRate"]    = st.fileRate;
            t["fileBits"]    = st.fileBits;
            t["deviceRate"]  = st.deviceRate;
            t["deviceBits"]  = st.deviceBits;
            t["floatPath"]   = st.floatPath;
            t["rateMatched"] = st.rateMatched;
            t["exclusive"]   = st.exclusive;
            t["unityGain"]   = st.unityGain;
            t["lossyLink"]   = st.lossyLink;
            t["dop"]         = st.dop;
            t["dithering"]   = st.dithering;
            t["latency"]     = st.latency;
            t["bitPerfect"]  = st.BitPerfect();
            return t;
        };
        stream["GetSpectrum"]   = [](MusicStream& s, int bands, sol::this_state ts) -> sol::table
        {
            sol::state_view lua(ts);
            bands = Maths::Clamp(bands, 1, 256);
            float values[256];
            u32 n        = s.GetSpectrum(values, (u32)bands);
            sol::table t = lua.create_table(n, 0);
            for(u32 i = 0; i < n; i++)
                t[i + 1] = values[i];
            return t;
        };

        auto toTable = [](sol::this_state ts, const float* v, u32 n)
        {
            sol::state_view lua(ts);
            sol::table t = lua.create_table(n, 0);
            for(u32 i = 0; i < n; i++)
                t[i + 1] = v[i];
            return t;
        };
        stream["GetFFT"] = [toTable](MusicStream& s, int bins, sol::this_state ts) -> sol::table
        {
            float v[1024];
            u32 n = s.GetFFT(v, (u32)Maths::Clamp(bins, 1, 1024));
            return toTable(ts, v, n);
        };
        stream["GetWaveform"] = [toTable](MusicStream& s, int count, sol::this_state ts) -> sol::table
        {
            float v[2048];
            u32 n = s.GetWaveform(v, (u32)Maths::Clamp(count, 1, 2048));
            return toTable(ts, v, n);
        };

        // Feed a stream into a shader preview as Shadertoy's music texture
        sol::usertype<Graphics::ShaderPreview> previewType = state["ShaderPreview"];
        // ShaderPreview.SetSpirvCache({ dirs... }, writeDir): compiled shaders by source hash - how
        // shipped builds (no GLSL compiler) get their baked presets
        previewType["SetSpirvCache"] = [](sol::table dirs, sol::optional<std::string> writeDir)
        {
            TDArray<std::string> list;
            for(auto& kv : dirs)
                if(kv.second.is<std::string>())
                    list.PushBack(kv.second.as<std::string>());
            Graphics::ShaderPreview::SetSpirvCache(list, writeDir.value_or(""));
        };
        // MilkDrop-style trails: last frame's output as iChannel<n> (-1 = off)
        previewType["SetFeedback"] = [](Graphics::ShaderPreview& p, int channel) { p.SetFeedbackChannel(channel); };
        previewType["SetAudio"] = [](Graphics::ShaderPreview& p, MusicStream* s, sol::optional<int> channel)
        { p.SetAudio(s, (uint32_t)Maths::Clamp(channel.value_or(0), 0, 3)); };

        // iOS: true = a music app (plays when locked / silenced), false = game audio. No-op elsewhere.
        auto session = state["AudioSession"].get_or_create<sol::table>();
        session.set_function("SetPlayback", [](bool on) { AudioSession::SetPlayback(on); });

        auto tags = state["AudioTags"].get_or_create<sol::table>();
        // Chapters: { { title, start (s) }, ... } from an MP4 chapter track / chpl or ID3 CHAP frames; empty if none
        tags.set_function("ReadChapters", [](const std::string& path, sol::this_state ts) -> sol::table
                          {
            sol::state_view lua(ts);
            sol::table out = lua.create_table();
            AudioTags t;
            if(ReadAudioTags(path, t, false))
                for(u32 i = 0; i < t.Chapters.Size(); i++)
                {
                    sol::table c = lua.create_table();
                    c["title"]   = t.Chapters[i].Title;
                    c["start"]   = t.Chapters[i].Start;
                    out[i + 1]   = c;
                }
            return out; });
        // Embedded lyrics text, or nil (kept out of Read so library scans stay light)
        tags.set_function("ReadLyrics", [](const std::string& path) -> sol::optional<std::string>
                          {
            AudioTags t;
            if(!ReadAudioTags(path, t, false) || t.Lyrics.empty())
                return sol::nullopt;
            return t.Lyrics; });
        tags["ReplayGain"] = true; // Read results carry rg / trackGain / albumGain
        // Returns nil if the file can't be opened. withArtwork adds `artwork` (encoded bytes as a string).
        tags.set_function("Read", [](const std::string& path, sol::optional<bool> withArtwork, sol::this_state ts) -> sol::object
                          {
            sol::state_view lua(ts);
            AudioTags t;
            bool art = withArtwork.value_or(false);
            if(!ReadAudioTags(path, t, art))
                return sol::lua_nil;

            sol::table r       = lua.create_table();
            r["path"]          = path;
            r["title"]         = t.Title;
            r["artist"]        = t.Artist;
            r["album"]         = t.Album;
            r["albumArtist"]   = t.AlbumArtist;
            r["genre"]         = t.Genre;
            r["composer"]      = t.Composer;
            r["track"]         = t.Track;
            r["trackCount"]    = t.TrackCount;
            r["disc"]          = t.Disc;
            r["discCount"]     = t.DiscCount;
            r["year"]          = t.Year;
            r["compilation"]   = t.Compilation;
            r["duration"]      = t.Duration;
            r["sampleRate"]    = t.SampleRate;
            r["channels"]      = t.Channels;
            r["bitrate"]       = t.Bitrate;
            r["codec"]         = t.Codec;
            r["sortTitle"]       = t.SortTitle;
            r["sortArtist"]      = t.SortArtist;
            r["sortAlbum"]       = t.SortAlbum;
            r["sortAlbumArtist"] = t.SortAlbumArtist;
            r["bits"]          = t.Bits;
            r["rg"]            = true; // ReplayGain tags were looked for
            if(t.HasTrackGain)
                r["trackGain"] = t.TrackGain;
            if(t.HasAlbumGain)
                r["albumGain"] = t.AlbumGain;
            r["hasArtwork"]    = t.HasArtwork || !t.Artwork.Empty();
            if(art && !t.Artwork.Empty())
            {
                r["artwork"]     = std::string((const char*)t.Artwork.Data(), t.Artwork.Size());
                r["artworkMime"] = t.ArtworkMime;
            }
            return r; });

        // Background tag reading for library scans: Queue(path) -> id; Poll() -> { { id, path, tags | false }, ... }
        tags.set_function("Queue", [](const std::string& path) { return AudioTagReader::Queue(path); });
        tags.set_function("Poll", [](sol::optional<int> maxN, sol::this_state ts)
                          {
            sol::state_view lua(ts);
            TDArray<AudioTagReader::Result> results;
            AudioTagReader::Poll(results, (u32)Maths::Max(1, maxN.value_or(256)));
            sol::table out = lua.create_table();
            for(u32 i = 0; i < results.Size(); i++)
            {
                const auto& r = results[i];
                sol::table e = lua.create_table();
                e["id"]   = r.id;
                e["path"] = r.path;
                if(r.ok)
                {
                    const AudioTags& t = r.tags;
                    sol::table g       = lua.create_table();
                    g["path"] = r.path, g["title"] = t.Title, g["artist"] = t.Artist, g["album"] = t.Album;
                    g["albumArtist"] = t.AlbumArtist, g["genre"] = t.Genre, g["composer"] = t.Composer;
                    g["track"] = t.Track, g["trackCount"] = t.TrackCount, g["disc"] = t.Disc, g["discCount"] = t.DiscCount;
                    g["year"] = t.Year, g["compilation"] = t.Compilation, g["duration"] = t.Duration;
                    g["sampleRate"] = t.SampleRate, g["channels"] = t.Channels, g["bitrate"] = t.Bitrate, g["hasArtwork"] = t.HasArtwork;
                    g["codec"] = t.Codec, g["bits"] = t.Bits;
                    g["rg"] = true;
                    if(t.HasTrackGain)
                        g["trackGain"] = t.TrackGain;
                    if(t.HasAlbumGain)
                        g["albumGain"] = t.AlbumGain;
                    g["sortArtist"] = t.SortArtist, g["sortAlbum"] = t.SortAlbum, g["sortAlbumArtist"] = t.SortAlbumArtist, g["sortTitle"] = t.SortTitle;
                    e["tags"] = g;
                }
                else
                    e["tags"] = false;
                out[i + 1] = e;
            }
            return out; });
        // Write({ title=, artist=, album=, albumArtist=, genre=, composer=, year=, track=, trackCount=, disc=, discCount=,
        //         lyrics=, sortTitle=, sortArtist=, sortAlbum=, sortAlbumArtist=, artwork= (bytes, "" removes), artworkMime= })
        // -> ok, err. Only the keys present change; "" / 0 removes the field.
        tags.set_function("Write", [](const std::string& path, sol::table fields) -> std::tuple<bool, sol::optional<std::string>>
                          {
            AudioTagEdit e;
            auto str = [&](const char* key, std::string& dst, u32 flag) {
                if(auto v = fields.get<sol::optional<std::string>>(key)) { dst = *v; e.set |= flag; } };
            auto num = [&](const char* key, u32& dst, u32 flag) {
                sol::object o = fields[key];
                if(o.get_type() == sol::type::number) { dst = (u32)Maths::Max(0.0, o.as<double>()); e.set |= flag; } };
            str("title", e.title, AudioTagEdit::Title);
            str("artist", e.artist, AudioTagEdit::Artist);
            str("album", e.album, AudioTagEdit::Album);
            str("albumArtist", e.albumArtist, AudioTagEdit::AlbumArtist);
            str("genre", e.genre, AudioTagEdit::Genre);
            str("composer", e.composer, AudioTagEdit::Composer);
            str("lyrics", e.lyrics, AudioTagEdit::Lyrics);
            str("sortTitle", e.sortTitle, AudioTagEdit::SortTitle);
            str("sortArtist", e.sortArtist, AudioTagEdit::SortArtist);
            str("sortAlbum", e.sortAlbum, AudioTagEdit::SortAlbum);
            str("sortAlbumArtist", e.sortAlbumArtist, AudioTagEdit::SortAlbumArtist);
            str("artwork", e.artwork, AudioTagEdit::Artwork); // encoded image bytes; "" removes
            if(auto m = fields.get<sol::optional<std::string>>("artworkMime"))
                e.artworkMime = *m;
            num("year", e.year, AudioTagEdit::Year);
            num("track", e.track, AudioTagEdit::TrackNum);
            num("trackCount", e.trackCount, AudioTagEdit::TrackNum);
            num("disc", e.disc, AudioTagEdit::DiscNum);
            num("discCount", e.discCount, AudioTagEdit::DiscNum);
            std::string err;
            if(WriteAudioTags(path, e, &err))
                return { true, sol::nullopt };
            return { false, err }; });
        tags.set_function("Pending", []() { return AudioTagReader::Pending(); });
        tags.set_function("ClearQueue", []() { AudioTagReader::Clear(); });

        // Embedded cover art straight to a texture (skips the Lua string copy). maxSize clamps the larger side.
        tags.set_function("LoadArtwork", [](const std::string& path, sol::optional<int> maxSize) -> SharedPtr<Graphics::Texture2D>
                          {
            AudioTags t;
            if(!ReadAudioTags(path, t, true) || t.Artwork.Empty())
                return nullptr;
            return TextureFromEncoded(t.Artwork.Data(), t.Artwork.Size(), (u32)Maths::Max(0, maxSize.value_or(512))); });

        // Vivid accent colour of encoded image bytes (Vec4, alpha 1), or nil for greyscale images
        state.set_function("ImageAccentColour", [](const std::string& bytes) -> sol::optional<Vec4>
                           {
            float rgb[3];
            if(!ImageAccentColour((const u8*)bytes.data(), bytes.size(), rgb))
                return sol::nullopt;
            return Vec4(rgb[0], rgb[1], rgb[2], 1.0f); });

        // Background image decoding: ImageLoader.Queue({ { file = p }, { audio = p }, { bytes = s } }, maxSize, square) -> id
        auto il = state["ImageLoader"].get_or_create<sol::table>();
        il.set_function("Queue", [](sol::table sources, sol::optional<int> maxSize, sol::optional<bool> square)
                        {
            TDArray<AsyncImageLoader::Source> list;
            for(auto& kv : sources)
            {
                if(!kv.second.is<sol::table>())
                    continue;
                sol::table t = kv.second.as<sol::table>();
                AsyncImageLoader::Source src;
                if(auto f = t.get<sol::optional<std::string>>("file"))
                    src.kind = AsyncImageLoader::Source::File, src.data = *f;
                else if(auto a = t.get<sol::optional<std::string>>("audio"))
                    src.kind = AsyncImageLoader::Source::Audio, src.data = *a;
                else if(auto b = t.get<sol::optional<std::string>>("bytes"))
                    src.kind = AsyncImageLoader::Source::Bytes, src.data = *b;
                else
                    continue;
                list.PushBack(std::move(src));
            }
            return AsyncImageLoader::Queue(list, (u32)Maths::Max(0, maxSize.value_or(512)), square.value_or(false)); });
        // nil while pending, false if nothing decoded, else texture + which source (1-based)
        il.set_function("Poll", [](u32 id, sol::this_state ts) -> std::tuple<sol::object, int>
                        {
            AsyncImageLoader::Result r;
            if(!AsyncImageLoader::Poll(id, r))
                return { sol::lua_nil, 0 };
            if(!r.ok)
                return { sol::make_object(ts, false), 0 };
            Graphics::TextureDesc desc;
            desc.format    = Graphics::RHIFormat::R8G8B8A8_Unorm;
            desc.minFilter = Graphics::TextureFilter::LINEAR;
            desc.magFilter = Graphics::TextureFilter::LINEAR;
            desc.wrap      = Graphics::TextureWrap::CLAMP_TO_EDGE;
            desc.srgb      = false; // match LoadTexture2D
            SharedPtr<Graphics::Texture2D> tex(Graphics::Texture2D::CreateFromSource(r.width, r.height, r.pixels.Data(), desc));
            return { sol::make_object(ts, tex), (int)r.sourceIndex + 1 }; });
        il.set_function("Cancel", [](u32 id) { AsyncImageLoader::Cancel(id); });
        il.set_function("Pending", []() { return AsyncImageLoader::Pending(); });

        state.set_function("LoadTextureFromMemory", [](const std::string& bytes, sol::optional<int> maxSize) -> SharedPtr<Graphics::Texture2D>
                           { return TextureFromEncoded((const u8*)bytes.data(), bytes.size(), (u32)Maths::Max(0, maxSize.value_or(0))); });
    }
}
