#include "Precompiled.h"
#include "SystemLua.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"
#include "Core/OS/Http.h"
#include "Utilities/AsyncFileCopy.h"
#include "Utilities/AsyncFileList.h"
#include "Core/OS/FolderWatcher.h"
#include "Core/OS/FileActions.h"
#include "Core/OS/Bookmarks.h"
#include "Core/OS/Store.h"
#include "Core/OS/OpenFiles.h"
#include "Core/OS/Input.h"
#include "Maths/MathsUtilities.h"
#include "Core/OS/OS.h"
#include "Core/Licences.h"
#include "Core/OS/Window.h"
#include "Core/Application.h"
#include "Maths/Vector4.h"

#include <cereal/external/rapidjson/document.h>

#include <sol/sol.hpp>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <chrono>

namespace fs = std::filesystem;

namespace Lumos
{
    namespace
    {
        std::string Resolve(const std::string& path)
        {
            if(path.size() < 2 || path[0] != '/' || path[1] != '/')
                return path;
            std::string out   = path;
            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 resolved  = {};
            if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(path), &resolved) && resolved.size)
                out = std::string((const char*)resolved.str, resolved.size);
            ScratchEnd(scratch);
            return out;
        }
    }

    static sol::object JsonToLua(sol::state_view& lua, const CEREAL_RAPIDJSON_NAMESPACE::Value& v)
    {
        if(v.IsNull())
            return sol::lua_nil;
        if(v.IsBool())
            return sol::make_object(lua, v.GetBool());
        if(v.IsNumber())
            return sol::make_object(lua, v.GetDouble());
        if(v.IsString())
            return sol::make_object(lua, std::string(v.GetString(), v.GetStringLength()));
        sol::table t = lua.create_table();
        if(v.IsArray())
        {
            int i = 1;
            for(auto& e : v.GetArray())
                t[i++] = JsonToLua(lua, e);
        }
        else if(v.IsObject())
        {
            for(auto& m : v.GetObject())
                t[std::string(m.name.GetString(), m.name.GetStringLength())] = JsonToLua(lua, m.value);
        }
        return t;
    }

    static Window* AppWindow()
    {
        return Application::Get().GetWindow();
    }

    void BindSystemLua(sol::state& state)
    {
        // ---- JSON ----
        auto json = state["JSON"].get_or_create<sol::table>();
        // Returns nil, error on bad input. JSON null becomes nil.
        json.set_function("Decode", [](const std::string& text, sol::this_state ts) -> std::tuple<sol::object, sol::object>
                          {
            sol::state_view lua(ts);
            CEREAL_RAPIDJSON_NAMESPACE::Document doc;
            doc.Parse(text.c_str(), text.size());
            if(doc.HasParseError())
                return { sol::lua_nil, sol::make_object(lua, std::string("parse error at ") + std::to_string(doc.GetErrorOffset())) };
            return { JsonToLua(lua, doc), sol::lua_nil }; });

        // ---- HTTP ----
        auto http = state["Http"].get_or_create<sol::table>();
        // Async GET. headers: { ["User-Agent"] = "..." }. Returns a request id for Http.Poll.
        http.set_function("Get", [](const std::string& url, sol::optional<sol::table> headers) -> u32
                          {
            Http::Headers h;
            if(headers)
                for(auto& kv : *headers)
                    if(kv.first.is<std::string>() && kv.second.is<std::string>())
                        h.push_back({ kv.first.as<std::string>(), kv.second.as<std::string>() });
            return Http::Get(url, h); });
        // Async POST of `body` as it is (set Content-Type in headers); polled like Get
        http.set_function("Post", [](const std::string& url, const std::string& body, sol::optional<sol::table> headers) -> u32
                          {
            Http::Headers h;
            if(headers)
                for(auto& kv : *headers)
                    if(kv.first.is<std::string>() && kv.second.is<std::string>())
                        h.push_back({ kv.first.as<std::string>(), kv.second.as<std::string>() });
            return Http::Post(url, body, h); });
        // Async GET saved straight to `path` (the polled body is empty)
        http.set_function("Download", [](const std::string& url, const std::string& path, sol::optional<sol::table> headers) -> u32
                          {
            Http::Headers h;
            if(headers)
                for(auto& kv : *headers)
                    if(kv.first.is<std::string>() && kv.second.is<std::string>())
                        h.push_back({ kv.first.as<std::string>(), kv.second.as<std::string>() });
            return Http::Download(url, path, h); });
        // nil while pending, then { status, body, error } once (the request is then forgotten)
        http.set_function("Poll", [](u32 id, sol::this_state ts) -> sol::object
                          {
            Http::Response r;
            if(!Http::Poll(id, r))
                return sol::lua_nil;
            sol::state_view lua(ts);
            sol::table t = lua.create_table();
            t["status"]  = r.Status;
            t["body"]    = std::move(r.Body);
            t["error"]   = std::move(r.Error);
            t["ok"]      = r.Error.empty() && r.Status >= 200 && r.Status < 300;
            return t; });
        http.set_function("Cancel", [](u32 id) { Http::Cancel(id); });
        http.set_function("Pending", []() { return Http::PendingCount(); });
        http.set_function("UrlEncode", [](const std::string& s) { return Http::UrlEncode(s); });

        // ---- Background file copies (folders created as needed) ----
        auto fc = state["FileCopy"].get_or_create<sol::table>();
        fc.set_function("Queue", [](const std::string& src, const std::string& dst, sol::optional<bool> overwrite)
                        { return AsyncFileCopy::Queue(Resolve(src), Resolve(dst), overwrite.value_or(false)); });
        // { { id, src, dst, ok, error, bytes }, ... } finished since the last call
        fc.set_function("Poll", [](sol::this_state ts)
                        {
            sol::state_view lua(ts);
            TDArray<AsyncFileCopy::Result> done;
            AsyncFileCopy::Poll(done, 256);
            sol::table out = lua.create_table();
            for(u32 i = 0; i < done.Size(); i++)
            {
                sol::table r = lua.create_table();
                r["id"] = done[i].id, r["src"] = done[i].src, r["dst"] = done[i].dst;
                r["ok"] = done[i].error.empty(), r["error"] = done[i].error, r["bytes"] = (double)done[i].bytes;
                out[i + 1] = r;
            }
            return out; });
        fc.set_function("Pending", []() { return AsyncFileCopy::Pending(); });
        fc.set_function("Clear", []() { AsyncFileCopy::Clear(); });

        // ---- Third-party licences ----
        auto lic = state["Licences"].get_or_create<sol::table>();
        // { { name, licence, copyright, url }, ... } for everything built into the engine
        lic.set_function("List", [](sol::this_state ts)
                         {
            sol::state_view lua(ts);
            sol::table out = lua.create_table();
            u32 count      = 0;
            const ThirdPartyLicence* list = GetThirdPartyLicences(count);
            for(u32 i = 0; i < count; i++)
            {
                sol::table r   = lua.create_table();
                r["name"]      = list[i].Name;
                r["licence"]   = list[i].Licence;
                r["copyright"] = list[i].Copyright;
                r["url"]       = list[i].URL;
                out[i + 1]     = r;
            }
            return out; });
        lic.set_function("Text", [](const char* id) { return GetLicenceText(id); });

        // ---- Clipboard / trash ----
        auto clip = state["Clipboard"].get_or_create<sol::table>();
        clip.set_function("SetText", [](const std::string& text) { Input::Get().SetClipboard(text.c_str()); });
        clip.set_function("GetText", []() { return Input::Get().GetClipboard(); });
        clip.set_function("SetFiles", [](sol::table files)
                          {
            TDArray<std::string> list;
            for(auto& kv : files)
                if(kv.second.is<std::string>())
                    list.PushBack(Resolve(kv.second.as<std::string>()));
            return FileActions::CopyFilesToClipboard(list); });
        // Files the OS asked us to open since the last call (Open With, Dock drop); Inject fakes one
        state.set_function("PopOpenedFiles", [](sol::this_state ts) -> sol::table
                           {
            sol::state_view lua(ts);
            sol::table t = lua.create_table();
            TDArray<std::string> files;
            OpenFiles::Pop(files);
            for(u32 i = 0; i < files.Size(); i++)
                t[i + 1] = files[i];
            return t; });
        state.set_function("InjectOpenedFile", [](const std::string& path) { OpenFiles::Push(Resolve(path)); });
        // Lasting access to picked folders (sandboxed apps): Bookmarks.Create(path) -> string,
        // Bookmarks.Resolve(bookmark) -> path, stale (access starts for the run)
        {
            sol::table bm = state.create_named_table("Bookmarks");
            bm.set_function("Create", [](const std::string& path) { return Bookmarks::Create(Resolve(path)); });
            bm.set_function("Resolve", [](const std::string& data) -> std::tuple<std::string, bool>
                            {
                bool stale = false;
                std::string p = Bookmarks::Resolve(data, &stale);
                return { p, stale }; });
        }
        // In-app purchases: Store.Request({ids}), Products() -> nil while loading / { {id, name, description,
        // price, consumable} }, Buy(id), Restore(), Entitled(id), Poll() -> { {kind, id, message} }
        {
            sol::table st = state.create_named_table("Store");
            st.set_function("IsSupported", []() { return Store::IsSupported(); });
            st.set_function("Request", [](sol::table ids)
                            {
                TDArray<std::string> list;
                for(auto& kv : ids)
                    if(kv.second.is<std::string>())
                        list.PushBack(kv.second.as<std::string>());
                Store::Request(list); });
            st.set_function("Products", [](sol::this_state ts) -> sol::object
                            {
                TDArray<Store::Product> list;
                if(!Store::GetProducts(list))
                    return sol::lua_nil;
                sol::state_view lua(ts);
                sol::table out = lua.create_table();
                for(u32 i = 0; i < list.Size(); i++)
                {
                    sol::table p     = lua.create_table();
                    p["id"]          = list[i].id;
                    p["name"]        = list[i].name;
                    p["description"] = list[i].description;
                    p["price"]       = list[i].price;
                    p["plainPrice"]  = list[i].plainPrice;
                    p["consumable"]  = list[i].consumable;
                    out[i + 1]       = p;
                }
                return out; });
            st.set_function("Buy", [](const std::string& id) { Store::Buy(id); });
            st.set_function("Restore", []() { Store::Restore(); });
            st.set_function("Entitled", [](const std::string& id) { return Store::Entitled(id); });
            st.set_function("RequestReview", []() { Store::RequestReview(); });
            st.set_function("AppVersion", []() { return Store::AppVersion(); });
            st.set_function("Poll", [](sol::this_state ts)
                            {
                TDArray<Store::Event> list;
                Store::Poll(list);
                sol::state_view lua(ts);
                sol::table out = lua.create_table();
                for(u32 i = 0; i < list.Size(); i++)
                {
                    sol::table e = lua.create_table();
                    e["kind"]    = list[i].kind;
                    e["id"]      = list[i].id;
                    e["message"] = list[i].message;
                    out[i + 1]   = e;
                }
                return out; });
        }
        // true when assets come from a packed Assets.lpak (a shipped build) rather than loose files
        state.set_function("IsPackaged", []() { return FileSystem::Get().HasMountedPack(); });
        state.set_function("MoveToTrash", [](const std::string& path) { return FileActions::MoveToTrash(Resolve(path)); });

        // ---- Folder change notifications ----
        auto fw = state["FolderWatch"].get_or_create<sol::table>();
        fw.set_function("IsSupported", []() { return FolderWatcher::IsSupported(); });
        fw.set_function("Watch", [](sol::table folders)
                        {
            TDArray<std::string> list;
            for(auto& kv : folders)
                if(kv.second.is<std::string>())
                    list.PushBack(Resolve(kv.second.as<std::string>()));
            FolderWatcher::Watch(list); });
        fw.set_function("Stop", []() { FolderWatcher::Stop(); });
        fw.set_function("Poll", [](sol::this_state ts) -> sol::table
                        {
            sol::state_view lua(ts);
            sol::table t = lua.create_table();
            TDArray<std::string> changed;
            FolderWatcher::Poll(changed);
            for(u32 i = 0; i < changed.Size(); i++)
                t[i + 1] = changed[i];
            return t; });

        // ---- Binary-safe file IO (ReadFile/WriteFile are text mode) ----
        state.set_function("ReadFileBytes", [](const std::string& path) -> sol::optional<std::string>
                           {
            FILE* f = fopen(Resolve(path).c_str(), "rb");
            if(!f)
                return sol::nullopt;
            std::string data;
            char buf[16384];
            size_t n;
            while((n = fread(buf, 1, sizeof(buf), f)) > 0)
                data.append(buf, n);
            fclose(f);
            return data; });
        state.set_function("WriteFileBytes", [](const std::string& path, const std::string& data) -> bool
                           {
            FILE* f = fopen(Resolve(path).c_str(), "wb");
            if(!f)
                return false;
            bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
            fclose(f);
            return ok; });
        state.set_function("DeleteFile", [](const std::string& path) -> bool
                           {
            std::error_code ec;
            return fs::remove(fs::path(Resolve(path)), ec); });

        // ---- Window management (desktop) ----
        auto win = state["Window"].get_or_create<sol::table>();
        win.set_function("SetSize", [](int w, int h) { if(auto* wnd = AppWindow()) wnd->SetWindowSize((u32)Maths::Max(1, w), (u32)Maths::Max(1, h)); });
        win.set_function("GetSize", []() { auto* wnd = AppWindow(); return wnd ? wnd->GetWindowSize() : Vec2(0.0f); });
        win.set_function("SetPosition", [](int x, int y) { if(auto* wnd = AppWindow()) wnd->SetWindowPosition(x, y); });
        win.set_function("GetPosition", []() { auto* wnd = AppWindow(); return wnd ? wnd->GetWindowPosition() : Vec2(0.0f); });
        win.set_function("GetCursorScreenPosition", []() { auto* wnd = AppWindow(); return wnd ? wnd->GetCursorScreenPosition() : Vec2(0.0f); });
        win.set_function("SetDecorated", [](bool d) { if(auto* wnd = AppWindow()) wnd->SetDecorated(d); });
        win.set_function("SetFloating", [](bool f) { if(auto* wnd = AppWindow()) wnd->SetFloating(f); });
        win.set_function("SetShape", [](float radius, sol::optional<bool> shadow) { if(auto* wnd = AppWindow()) wnd->SetWindowShape(radius, shadow.value_or(true)); });
        win.set_function("SetTransparent", [](bool t) { Application::Get().SetWindowTransparent(t); });
        win.set_function("IsTransparent", []() { return Application::Get().IsWindowTransparent(); });
        win.set_function("IsSystemDarkMode", []() { return OS::Get().IsDarkMode(); });
        // Custom titlebars: BeginDrag on press (true = the window manager took over), UpdateDrag while held
        win.set_function("BeginDrag", []() { return OS::Get().BeginWindowDrag(); });
        win.set_function("UpdateDrag", []() { OS::Get().UpdateWindowDrag(); });
        win.set_function("IsMaximised", []() { return OS::Get().IsWindowMaximised(); });
        win.set_function("IsFullscreen", []() { return OS::Get().IsWindowFullscreen(); });
        win.set_function("SetFullscreen", [](bool on) { OS::Get().SetWindowFullscreen(on); });
        win.set_function("ToggleMaximise", []()
                         {
            if(OS::Get().IsWindowMaximised())
                OS::Get().RestoreWindow();
            else
                OS::Get().MaximiseWindow(); });
        state.set_function("OpenFileLocation", [](const std::string& path) { OS::Get().OpenFileLocation(Resolve(path)); });
        state.set_function("OpenURL", [](const std::string& url) { OS::Get().OpenURL(url); });
        // Monotonic wall clock in seconds (profiling, timeouts)
        state.set_function("Clock", []() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); });
        win.set_function("Minimise", []() { if(auto* wnd = AppWindow()) wnd->Minimise(); });
        win.set_function("PopDroppedFiles", [](sol::this_state ts) -> sol::table
                         {
            sol::state_view lua(ts);
            sol::table t = lua.create_table();
            if(auto* wnd = AppWindow())
            {
                auto files = wnd->PopDroppedFiles();
                for(u32 i = 0; i < files.Size(); i++)
                    t[i + 1] = files[i];
            }
            return t; });
        // Editor-style titlebar: content runs under a transparent titlebar with the window
        // buttons floating over it (macOS). Returns the bar height to leave clear, in points.
        win.set_function("SetUnifiedTitleBar", [](bool enabled, sol::optional<Vec4> colour) -> float
                         {
#ifdef LUMOS_PLATFORM_MACOS
            OS::Get().SetWindowDecorations(!enabled);
            if(enabled && colour)
                OS::Get().SetTitleBarColour(*colour, colour->x * 0.299f + colour->y * 0.587f + colour->z * 0.114f < 0.5f);
            return enabled ? 42.0f : 0.0f;
#else
            (void)enabled;
            (void)colour;
            return 0.0f;
#endif
        });

        // Physical-directory listing (VFS paths resolved). Returns sorted absolute paths.
        state.set_function("ListFiles", [](const std::string& dir, sol::optional<bool> recursive, sol::optional<std::string> extensions, sol::this_state ts) -> sol::table
                           {
            sol::state_view lua(ts);
            TDArray<std::string> exts, files;
            AsyncFileList::ParseExtensions(extensions.value_or(""), exts);
            AsyncFileList::List(Resolve(dir), recursive.value_or(false), exts, files);
            sol::table result = lua.create_table((int)files.Size(), 0);
            for(u32 i = 0; i < files.Size(); i++)
                result[i + 1] = files[i];
            return result; });

        // Background listing for big folders: id = FileList.Queue(dir, recursive, exts); files = FileList.Poll(id) (nil while walking)
        auto fl = state["FileList"].get_or_create<sol::table>();
        fl.set_function("Queue", [](const std::string& dir, sol::optional<bool> recursive, sol::optional<std::string> extensions)
                        { return AsyncFileList::Queue(Resolve(dir), recursive.value_or(false), extensions.value_or("")); });
        fl.set_function("QueueMissing", [](sol::table paths)
                        {
            TDArray<std::string> list;
            for(auto& kv : paths)
                if(kv.second.is<std::string>())
                    list.PushBack(kv.second.as<std::string>()); // physical paths, returned as given
            return AsyncFileList::QueueMissing(std::move(list)); });
        fl.set_function("Poll", [](u32 id, sol::this_state ts) -> sol::object
                        {
            TDArray<std::string> files;
            if(!AsyncFileList::Poll(id, files))
                return sol::lua_nil;
            sol::state_view lua(ts);
            sol::table t = lua.create_table((int)files.Size(), 0);
            for(u32 i = 0; i < files.Size(); i++)
                t[i + 1] = files[i];
            return t; });

        state.set_function("ListDirectories", [](const std::string& dir, sol::this_state ts) -> sol::table
                           {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            std::vector<std::string> dirs;
            std::error_code ec;
            for(fs::directory_iterator it(fs::path(Resolve(dir)), fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
            {
                const auto& name = it->path().filename().string();
                if(!name.empty() && name[0] != '.' && it->is_directory(ec))
                    dirs.push_back(it->path().string());
            }
            std::sort(dirs.begin(), dirs.end());
            for(size_t i = 0; i < dirs.size(); i++)
                result[i + 1] = dirs[i];
            return result; });

        state.set_function("FileExists", [](const std::string& path) -> bool
                           {
            std::error_code ec;
            return fs::exists(fs::path(Resolve(path)), ec); });

        state.set_function("MakeDirectory", [](const std::string& path) -> bool
                           {
            std::error_code ec;
            fs::create_directories(fs::path(Resolve(path)), ec);
            return !ec; });

        // Seconds since epoch; 0 if missing. Good enough for "has this changed" checks.
        state.set_function("GetFileModifiedTime", [](const std::string& path) -> double
                           {
            std::error_code ec;
            auto t = fs::last_write_time(fs::path(Resolve(path)), ec);
            if(ec)
                return 0.0;
            return (double)std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count(); });

        state.set_function("GetFileSize", [](const std::string& path) -> double
                           {
            std::error_code ec;
            auto s = fs::file_size(fs::path(Resolve(path)), ec);
            return ec ? 0.0 : (double)s; });

        // Per-user writable folder for app settings/caches, created on demand.
        // macOS/iOS: ~/Library/Application Support/<app>, Windows: %APPDATA%/<app>, Linux: $XDG_DATA_HOME or ~/.local/share/<app>.
        state.set_function("GetUserDataPath", [](const std::string& appName) -> std::string
                           { return FileSystem::UserDataPath(appName); });

        state.set_function("GetHomePath", []() -> std::string
                           {
#if defined(LUMOS_PLATFORM_WINDOWS)
            const char* home = std::getenv("USERPROFILE");
#else
            const char* home = std::getenv("HOME");
#endif
            return home ? std::string(home) : std::string(); });

        // Reveal a file or folder in Finder/Explorer. Desktop only.
        state.set_function("OpenInFileBrowser", [](const std::string& path)
                           {
            std::string p = Resolve(path);
            if(p.find('"') != std::string::npos)
                return;
#if defined(LUMOS_PLATFORM_MACOS)
            std::string cmd = "open -R \"" + p + "\"";
#elif defined(LUMOS_PLATFORM_WINDOWS)
            std::string cmd = "explorer /select,\"" + p + "\"";
#elif defined(LUMOS_PLATFORM_LINUX)
            std::string cmd = "xdg-open \"" + fs::path(p).parent_path().string() + "\" &";
#endif
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_WINDOWS) || defined(LUMOS_PLATFORM_LINUX)
            (void)std::system(cmd.c_str());
#endif
        });
    }
}
