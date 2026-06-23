#include "TextEditPanel.h"
#include "Editor.h"
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Scripting/Lua/LuaManager.h>
#include <Lumos/Scripting/Lua/LuaBindingRegistry.h>

#include <imgui/imgui.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Lumos
{
    static bool JustOpenedFile = false;

    static bool _IsIdentChar(char c)
    {
        return (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }
    TextEditPanel::TextEditPanel(const std::string& filePath)
        : m_FilePath(filePath)
    {
        m_Name           = "Text Editor###textEdit";
        m_ChangedName    = "Text Editor *###textEdit";
        m_SimpleName     = "TextEdit";
        m_OnSaveCallback = NULL;
        m_TextUnsaved    = false;
        editor.SetCustomIdentifiers({});

        auto extension = StringUtilities::GetFilePathExtension(m_FilePath);

        if(extension == "lua" || extension == "Lua")
        {
            auto lang = TextEditor::LanguageDefinition::Lua();
            editor.SetLanguageDefinition(lang);

            auto& customIdentifiers = LuaManager::GetIdentifiers();
            TextEditor::Identifiers identifiers;

            for(auto& k : customIdentifiers)
            {
                TextEditor::Identifier id;
                id.mDeclaration = "Engine function";
                identifiers.insert(std::make_pair(k, id));
            }

            editor.SetCustomIdentifiers(identifiers);
        }
        else if(extension == "cpp")
        {
            auto lang = TextEditor::LanguageDefinition::CPlusPlus();
            editor.SetLanguageDefinition(lang);
        }
        else if(extension == "glsl" || extension == "vert" || extension == "frag")
        {
            auto lang = TextEditor::LanguageDefinition::GLSL();
            editor.SetLanguageDefinition(lang);
        }

        String8 string = FileSystem::ReadTextFile(Application::Get().GetFrameArena(), Str8StdS(m_FilePath));
        editor.SetText((const char*)string.str);
        editor.SetShowWhitespaces(false);
        JustOpenedFile = true;
    }

    void TextEditPanel::SetErrors(const std::unordered_map<int, std::string>& errors)
    {
        editor.SetErrorMarkers(errors);
    }

    void TextEditPanel::OnImGui()
    {
        auto cpos = editor.GetCursorPosition();

        // Detect focus mode changes
        if(m_FocusMode && !m_PreviousFocusMode)
        {
            // Entering focus mode - save dock state
            ImGuiWindow* window = ImGui::FindWindowByName(m_Name.c_str());
            if(window)
            {
                m_SavedDockID = window->DockId;
            }
        }
        m_PreviousFocusMode = m_FocusMode;

        // Focus mode - full screen
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar;
        if(m_FocusMode)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowDockID(0, ImGuiCond_Always); // Undock
            windowFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
        }
        else
        {
            // Restore dock state when exiting focus mode
            if(m_SavedDockID != 0)
            {
                ImGui::SetNextWindowDockID(m_SavedDockID, ImGuiCond_Appearing);
            }
            ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        }

        std::string& windowName = m_TextUnsaved ? m_ChangedName : m_Name;
        if(ImGuiUtilities::BeginPanel(windowName.c_str(), &m_Active, windowFlags))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("File"))
                {
                    if(ImGui::MenuItem("Save", "CTRL+S"))
                    {
                        auto textToSave = editor.GetText();
                        FileSystem::WriteTextFile(Str8StdS(m_FilePath), Str8StdS(textToSave));
                        if(m_OnSaveCallback)
                            m_OnSaveCallback();

                        m_TextUnsaved = false;
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Edit"))
                {
                    bool ro = editor.IsReadOnly();
                    if(ImGui::MenuItem("Read-only mode", nullptr, &ro))
                        editor.SetReadOnly(ro);
                    ImGui::Separator();

                    if(ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
                        editor.Undo();
                    if(ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
                        editor.Redo();

                    ImGui::Separator();

                    if(ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                        editor.Copy();
                    if(ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
                        editor.Cut();
                    if(ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
                        editor.Delete();
                    if(ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                        editor.Paste();

                    ImGui::Separator();

                    if(ImGui::MenuItem("Select all", nullptr, nullptr))
                        editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                    if(ImGui::MenuItem("Close", nullptr, nullptr))
                    {
                        OnClose();
                        ImGui::EndMenu();
                        ImGui::EndMenuBar();
                        ImGui::End();
                        return;
                    }

                    ImGui::EndMenu();
                }

                if(ImGui::BeginMenu("Insert"))
                {
                    if(ImGui::MenuItem("Snippet..."))
                    {
                        m_ShowSnippetBrowser = true;
                        if(!m_SnippetsLoaded)
                            LoadSnippets();
                    }
                    if(ImGui::MenuItem("Reload Snippets"))
                    {
                        LoadSnippets();
                    }
                    ImGui::Separator();
                    if(ImGui::MenuItem("Autocomplete", "Cmd/Ctrl+Space"))
                    {
                        UpdateCompletionContext();
                        if(!m_CompletionItems.empty())
                        {
                            m_CompletionOpen       = true;
                            m_CompletionJustOpened = true;
                            ImVec2 mp              = ImGui::GetMousePos();
                            m_CompletionPopupX     = mp.x;
                            m_CompletionPopupY     = mp.y + 16.0f;
                        }
                        LINFO("[Complete] items=%d owner='%s' prefix='%s' isMember=%d",
                              (int)m_CompletionItems.size(),
                              m_CompletionOwner.c_str(),
                              m_CompletionPrefix.c_str(),
                              m_CompletionIsMember ? 1 : 0);
                    }
                    ImGui::EndMenu();
                }

                if(ImGui::BeginMenu("View"))
                {
                    if(ImGui::MenuItem("Focus Mode", "F11", m_FocusMode))
                        m_FocusMode = !m_FocusMode;

                    ImGui::Separator();

                    if(ImGui::MenuItem("Dark palette"))
                        editor.SetPalette(TextEditor::GetDarkPalette());
                    if(ImGui::MenuItem("Light palette"))
                        editor.SetPalette(TextEditor::GetLightPalette());
                    if(ImGui::MenuItem("Retro blue palette"))
                        editor.SetPalette(TextEditor::GetRetroBluePalette());
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(), editor.IsOverwrite() ? "Ovr" : "Ins", editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), Lumos::StringUtilities::GetFileName(m_FilePath).c_str());

            if(editor.IsTextChanged() && !JustOpenedFile)
            {
                m_TextUnsaved = true;
                m_LocalsDirty = true;

                // Auto-trigger: peek at the char immediately before the cursor.
                // If we just typed `.` or `:` open the popup. If popup is already
                // open (typing identifier chars), refresh items so the prefix
                // filter stays current.
                const std::string line = editor.GetCurrentLineText();
                auto cur               = editor.GetCursorPosition();
                int col                = std::min(cur.mColumn, (int)line.size());
                char prev              = (col > 0) ? line[col - 1] : 0;
                const bool isIdent     = _IsIdentChar(prev);
                const bool isMemberSep = (prev == '.' || prev == ':');

                if(isMemberSep || (m_CompletionOpen && isIdent))
                {
                    UpdateCompletionContext();
                    if(m_CompletionItems.empty())
                    {
                        m_CompletionOpen = false;
                    }
                    else if(!m_CompletionOpen)
                    {
                        m_CompletionOpen       = true;
                        m_CompletionJustOpened = true;
                        ImVec2 mp              = ImGui::GetMousePos();
                        m_CompletionPopupX     = mp.x;
                        m_CompletionPopupY     = mp.y + 16.0f;
                    }
                }
                else if(m_CompletionOpen && !isIdent)
                {
                    // Whitespace / punctuation typed — drop the popup.
                    m_CompletionOpen = false;
                }
            }

            editor.Render(m_Name.c_str());
            const ImVec2 editorMin = ImGui::GetItemRectMin();

#ifdef LUMOS_PLATFORM_IOS
            bool editorFocused = ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows);
            if(editorFocused && !m_KeyboardShowing)
            {
                OS::Get().ShowKeyboard(true);
                m_KeyboardShowing = true;
            }
            else if(!editorFocused && m_KeyboardShowing)
            {
                OS::Get().ShowKeyboard(false);
                m_KeyboardShowing = false;
            }
#endif

            if(ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
            {
                // Save shortcut
                if((Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeld(InputCode::Key::LeftControl)) && Input::Get().GetKeyPressed(InputCode::Key::S))
                {
                    auto textToSave = editor.GetText();
                    FileSystem::WriteTextFile(Str8StdS(m_FilePath), Str8StdS(textToSave));
                    if(m_OnSaveCallback)
                        m_OnSaveCallback();

                    m_TextUnsaved = false;
                }

                // Focus mode toggle (F11)
                if(Input::Get().GetKeyPressed(InputCode::Key::F11))
                {
                    m_FocusMode = !m_FocusMode;
                }

                // Ctrl/Cmd+Space opens completion.
                const bool mod = Input::Get().GetKeyHeld(InputCode::Key::LeftSuper)
                              || Input::Get().GetKeyHeld(InputCode::Key::LeftControl);
                if(mod && Input::Get().GetKeyPressed(InputCode::Key::Space))
                {
                    UpdateCompletionContext();
                    if(!m_CompletionItems.empty() && !m_CompletionOpen)
                    {
                        m_CompletionOpen       = true;
                        m_CompletionJustOpened = true;
                        ImVec2 mp              = ImGui::GetMousePos();
                        m_CompletionPopupX     = mp.x;
                        m_CompletionPopupY     = mp.y + 16.0f;
                    }
                }
                if(m_CompletionOpen && Input::Get().GetKeyPressed(InputCode::Key::Escape))
                    m_CompletionOpen = false;
            }
            JustOpenedFile = false;
        }
        ImGui::End();

        if(m_CompletionOpen)
        {
            const float popupW = 560.0f, popupH = 320.0f;
            ImVec2 pos(m_CompletionPopupX, m_CompletionPopupY);
            ImGuiViewport* vp = ImGui::GetMainViewport();
            if(vp)
            {
                if(pos.x + popupW > vp->WorkPos.x + vp->WorkSize.x)
                    pos.x = vp->WorkPos.x + vp->WorkSize.x - popupW;
                if(pos.y + popupH > vp->WorkPos.y + vp->WorkSize.y)
                    pos.y = vp->WorkPos.y + vp->WorkSize.y - popupH;
                if(pos.x < vp->WorkPos.x) pos.x = vp->WorkPos.x;
                if(pos.y < vp->WorkPos.y) pos.y = vp->WorkPos.y;
            }
            // Anchor + focus only on the first frame so the popup doesn't
            // hijack input on every subsequent frame.
            if(m_CompletionJustOpened)
            {
                ImGui::SetNextWindowPos(pos);
                ImGui::SetNextWindowFocus();
                m_CompletionJustOpened = false;
            }
            ImGui::SetNextWindowSize(ImVec2(popupW, popupH), ImGuiCond_Always);
            DrawCompletionPopup();
        }

        if(m_ShowSnippetBrowser)
            DrawSnippetBrowser();
    }

    void TextEditPanel::LoadSnippets()
    {
        m_SnippetGroups.clear();
        m_SelectedGroup   = -1;
        m_SelectedSnippet = -1;

        const std::string projectRoot = Application::Get().GetProjectSettings().m_ProjectRoot;
        std::filesystem::path snippetDir = std::filesystem::path(projectRoot) / "Assets" / "Scripts" / "Snippets";

        std::error_code ec;
        if(!std::filesystem::is_directory(snippetDir, ec))
        {
            m_SnippetsLoaded = true;
            return;
        }

        std::vector<std::filesystem::path> files;
        for(auto& entry : std::filesystem::directory_iterator(snippetDir, ec))
        {
            if(!entry.is_regular_file()) continue;
            auto p = entry.path();
            if(p.extension() != ".lua") continue;
            files.push_back(p);
        }
        std::sort(files.begin(), files.end());

        // Snippet block marker: lines beginning with "-- ===== Snippet:" start a block;
        // the next line beginning with "-- =====" (no colon required) ends it.
        for(auto& path : files)
        {
            SnippetGroup group;
            group.FileName = path.filename().string();

            std::ifstream in(path);
            if(!in.is_open()) continue;

            std::string  line;
            bool         inBlock = false;
            Snippet      cur;
            std::ostringstream body;

            while(std::getline(in, line))
            {
                const std::string startTag = "-- ===== Snippet:";
                if(!inBlock)
                {
                    if(line.rfind(startTag, 0) == 0)
                    {
                        inBlock = true;
                        // Extract title: text after "Snippet:" up to trailing "=" run.
                        std::string title = line.substr(startTag.size());
                        size_t eq = title.find_first_of('=');
                        if(eq != std::string::npos) title = title.substr(0, eq);
                        // Trim spaces.
                        size_t a = title.find_first_not_of(" \t");
                        size_t b = title.find_last_not_of(" \t");
                        cur = {};
                        cur.Name = (a == std::string::npos) ? "(unnamed)" : title.substr(a, b - a + 1);
                        body.str(std::string());
                        body.clear();
                    }
                    continue;
                }
                // In block.
                if(line.rfind("-- =====", 0) == 0 && line.find("Snippet:") == std::string::npos)
                {
                    cur.Body = body.str();
                    // Drop trailing newline for cleanliness.
                    if(!cur.Body.empty() && cur.Body.back() == '\n') cur.Body.pop_back();
                    group.Snippets.push_back(std::move(cur));
                    inBlock = false;
                    continue;
                }
                body << line << '\n';
            }

            if(!group.Snippets.empty())
                m_SnippetGroups.push_back(std::move(group));
        }

        m_SnippetsLoaded = true;
    }

    void TextEditPanel::DrawSnippetBrowser()
    {
        ImGui::SetNextWindowSize(ImVec2(720, 480), ImGuiCond_FirstUseEver);
        if(!ImGui::Begin("Lua Snippets", &m_ShowSnippetBrowser))
        {
            ImGui::End();
            return;
        }

        ImGui::InputTextWithHint("##filter", "filter...", m_SnippetFilter, sizeof(m_SnippetFilter));
        ImGui::SameLine();
        if(ImGui::Button("Reload")) LoadSnippets();
        ImGui::Separator();

        const float leftWidth = 220.0f;
        ImGui::BeginChild("snippet_groups", ImVec2(leftWidth, 0), true);
        std::string filter = m_SnippetFilter;
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

        for(int g = 0; g < (int)m_SnippetGroups.size(); ++g)
        {
            const auto& grp = m_SnippetGroups[g];
            // Hide groups whose entries are all filtered out, when filter active.
            bool anyVisible = filter.empty();
            if(!anyVisible)
            {
                for(auto& s : grp.Snippets)
                {
                    std::string n = s.Name;
                    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                    if(n.find(filter) != std::string::npos) { anyVisible = true; break; }
                }
            }
            if(!anyVisible) continue;

            ImGui::PushID(g);
            bool open = ImGui::TreeNodeEx(grp.FileName.c_str(),
                                          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);
            if(open)
            {
                for(int s = 0; s < (int)grp.Snippets.size(); ++s)
                {
                    const auto& sn = grp.Snippets[s];
                    if(!filter.empty())
                    {
                        std::string n = sn.Name;
                        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                        if(n.find(filter) == std::string::npos) continue;
                    }
                    bool selected = (m_SelectedGroup == g && m_SelectedSnippet == s);
                    ImGui::PushID(s);
                    if(ImGui::Selectable(sn.Name.c_str(), selected))
                    {
                        m_SelectedGroup   = g;
                        m_SelectedSnippet = s;
                    }
                    if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        m_SelectedGroup   = g;
                        m_SelectedSnippet = s;
                        editor.InsertText(sn.Body);
                        m_TextUnsaved        = true;
                        m_ShowSnippetBrowser = false;
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("snippet_preview", ImVec2(0, 0), true);

        const Snippet* sel = nullptr;
        if(m_SelectedGroup >= 0 && m_SelectedGroup < (int)m_SnippetGroups.size())
        {
            const auto& grp = m_SnippetGroups[m_SelectedGroup];
            if(m_SelectedSnippet >= 0 && m_SelectedSnippet < (int)grp.Snippets.size())
                sel = &grp.Snippets[m_SelectedSnippet];
        }

        if(sel)
        {
            ImGui::TextUnformatted(sel->Name.c_str());
            ImGui::Separator();
            if(ImGui::Button("Insert at Cursor"))
            {
                editor.InsertText(sel->Body);
                m_TextUnsaved        = true;
                m_ShowSnippetBrowser = false;
            }
            ImGui::SameLine();
            if(ImGui::Button("Copy to Clipboard"))
            {
                ImGui::SetClipboardText(sel->Body.c_str());
            }
            ImGui::SameLine();
            if(ImGui::Button("Close"))
            {
                m_ShowSnippetBrowser = false;
            }
            ImGui::Separator();
            ImGui::BeginChild("##preview_body", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(sel->Body.c_str(), sel->Body.c_str() + sel->Body.size());
            ImGui::EndChild();
        }
        else
        {
            ImGui::TextDisabled("Select a snippet to preview.");
            if(m_SnippetGroups.empty())
            {
                ImGui::Spacing();
                ImGui::TextWrapped("No snippet files found. Expected at: Assets/Scripts/Snippets/*.lua");
            }
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void TextEditPanel::OnClose()
    {
        ((Editor*)(&Application::Get()))->RemovePanel(this);
    }

    // ============================================================================
    // Autocomplete
    // ============================================================================

    // Build a map { local_name -> type_name } by scanning the doc for
    //   local <name> = <rhs>
    // and resolving the RHS against the binding registry. Brittle for complex
    // control flow but matches typical script style.
    void TextEditPanel::RebuildLocalsIndex()
    {
        m_LocalsIndex.clear();
        const std::string text = editor.GetText();

        size_t i = 0;
        const size_t N = text.size();
        while(i < N)
        {
            // find "local " at start of line (allow leading whitespace)
            size_t lineStart = i;
            while(i < N && (text[i] == ' ' || text[i] == '\t')) ++i;
            const char* kw = "local ";
            const size_t kwLen = 6;
            bool matched = (i + kwLen <= N) && (text.compare(i, kwLen, kw) == 0);
            if(!matched)
            {
                while(i < N && text[i] != '\n') ++i;
                if(i < N) ++i;
                continue;
            }
            i += kwLen;
            while(i < N && (text[i] == ' ' || text[i] == '\t')) ++i;
            size_t nameStart = i;
            while(i < N && _IsIdentChar(text[i])) ++i;
            if(i == nameStart) { while(i < N && text[i] != '\n') ++i; continue; }
            std::string name = text.substr(nameStart, i - nameStart);
            while(i < N && (text[i] == ' ' || text[i] == '\t')) ++i;
            if(i >= N || text[i] != '=') { while(i < N && text[i] != '\n') ++i; continue; }
            ++i; // past '='
            while(i < N && (text[i] == ' ' || text[i] == '\t')) ++i;

            // Now figure out the type of the RHS. Look at the leading token chain.
            // Cases handled:
            //   Vec3.new(...)             -> Vec3
            //   Quat.new(...)             -> Quat
            //   Foo(...)                  -> registry constructor return
            //   GlobalFn(...)             -> registry global return type
            //   existing_local:Method()   -> recurse
            //   existing_local.field      -> field type
            //   existing_local            -> alias
            std::string rhsType;

            // read first identifier
            size_t s = i;
            while(s < N && _IsIdentChar(text[s])) ++s;
            if(s > i)
            {
                std::string head = text.substr(i, s - i);
                String8 head8 = Str8StdS(head);
                // If head is a registered type, treat as constructor call.
                if(LuaRegistry::IsType(head8))
                {
                    rhsType = head;
                }
                else
                {
                    // alias to existing local?
                    for(auto& p : m_LocalsIndex) if(p.first == head) { rhsType = p.second; break; }
                    // global function?
                    if(rhsType.empty())
                    {
                        // empty owner -> global
                        const Lumos::String8 empty {};
                        if(auto* e = LuaRegistry::FindMember(empty, head8))
                        {
                            if(e->ReturnType.size > 0)
                                rhsType.assign((const char*)e->ReturnType.str, e->ReturnType.size);
                        }
                    }
                }

                // Walk chained calls / fields on the result.
                size_t cur = s;
                while(!rhsType.empty() && cur < N)
                {
                    char sep = text[cur];
                    if(sep != '.' && sep != ':') break;
                    ++cur;
                    size_t mEnd = cur;
                    while(mEnd < N && _IsIdentChar(text[mEnd])) ++mEnd;
                    if(mEnd == cur) break;
                    std::string m = text.substr(cur, mEnd - cur);
                    String8 owner8 = Str8StdS(rhsType);
                    String8 mem8   = Str8StdS(m);
                    const auto* e = LuaRegistry::FindMember(owner8, mem8);
                    if(!e) { rhsType.clear(); break; }
                    if(e->ReturnType.size > 0)
                        rhsType.assign((const char*)e->ReturnType.str, e->ReturnType.size);
                    else
                        rhsType.clear();
                    cur = mEnd;
                    // skip "(...)" call if present
                    if(cur < N && text[cur] == '(')
                    {
                        int depth = 1; ++cur;
                        while(cur < N && depth > 0)
                        {
                            if(text[cur] == '(') ++depth;
                            else if(text[cur] == ')') --depth;
                            ++cur;
                        }
                    }
                }
            }

            if(!rhsType.empty())
                m_LocalsIndex.push_back({ name, rhsType });

            while(i < N && text[i] != '\n') ++i;
            if(i < N) ++i;
        }
        m_LocalsDirty = false;
    }

    // Inspect current line up to cursor to decide:
    //  - is this a member access (after . or :) → owner type lookup, member prefix
    //  - or a bare prefix → globals + types + locals match
    void TextEditPanel::UpdateCompletionContext()
    {
        static bool s_LoggedSize = false;
        if(!s_LoggedSize)
        {
            LINFO("[Complete] registry size = %zu entries", LuaRegistry::All().Size());
            s_LoggedSize = true;
        }
        if(m_LocalsDirty) RebuildLocalsIndex();

        m_CompletionItems.clear();
        m_CompletionOwner.clear();
        m_CompletionPrefix.clear();
        m_CompletionIsMember = false;
        m_CompletionSelected = 0;

        const std::string line = editor.GetCurrentLineText();
        auto cur = editor.GetCursorPosition();
        int col = cur.mColumn;
        if(col > (int)line.size()) col = (int)line.size();

        // Slice the prefix word.
        int wEnd = col;
        int wStart = wEnd;
        while(wStart > 0 && _IsIdentChar(line[wStart - 1])) --wStart;
        m_CompletionPrefix = line.substr(wStart, wEnd - wStart);
        m_CompletionAnchor = TextEditor::Coordinates(cur.mLine, wStart);

        // Member access?
        if(wStart > 0 && (line[wStart - 1] == '.' || line[wStart - 1] == ':'))
        {
            m_CompletionIsMember = true;
            // Walk back through chained method/field calls to find a base name.
            int p = wStart - 1; // sits on . or :
            std::string chain;
            while(p >= 0)
            {
                char c = line[p];
                if(c == ')')
                {
                    int depth = 1; --p;
                    while(p >= 0 && depth > 0)
                    {
                        if(line[p] == ')') ++depth;
                        else if(line[p] == '(') --depth;
                        --p;
                    }
                    continue;
                }
                if(c == '.' || c == ':' || _IsIdentChar(c)) { chain.insert(chain.begin(), c); --p; }
                else break;
            }
            // Resolve chain to a type.
            std::string ownerType;
            size_t i = 0;
            while(i < chain.size())
            {
                size_t s = i;
                while(i < chain.size() && _IsIdentChar(chain[i])) ++i;
                std::string tok = chain.substr(s, i - s);
                if(ownerType.empty())
                {
                    String8 tk = Str8StdS(tok);
                    if(LuaRegistry::IsType(tk)) ownerType = tok;
                    else
                    {
                        for(auto& pr : m_LocalsIndex) if(pr.first == tok) { ownerType = pr.second; break; }
                        if(ownerType.empty())
                        {
                            const String8 empty {};
                            if(auto* e = LuaRegistry::FindMember(empty, tk))
                            {
                                if(e->ReturnType.size > 0)
                                    ownerType.assign((const char*)e->ReturnType.str, e->ReturnType.size);
                            }
                        }
                    }
                }
                else
                {
                    String8 ot = Str8StdS(ownerType);
                    String8 mt = Str8StdS(tok);
                    const auto* e = LuaRegistry::FindMember(ot, mt);
                    if(!e) { ownerType.clear(); break; }
                    if(e->ReturnType.size > 0)
                        ownerType.assign((const char*)e->ReturnType.str, e->ReturnType.size);
                    else
                        ownerType.clear();
                }
                // skip "(...)" call after tok
                if(i < chain.size() && chain[i] == '(')
                {
                    int depth = 1; ++i;
                    while(i < chain.size() && depth > 0)
                    {
                        if(chain[i] == '(') ++depth;
                        else if(chain[i] == ')') --depth;
                        ++i;
                    }
                }
                if(i < chain.size() && (chain[i] == '.' || chain[i] == ':')) ++i;
            }
            m_CompletionOwner = ownerType;
        }

        BuildCompletionCandidates();
    }

    void TextEditPanel::BuildCompletionCandidates()
    {
        m_CompletionItems.clear();
        const auto& all = LuaRegistry::All();

        auto matchPrefix = [&](const std::string& name) -> bool
        {
            if(m_CompletionPrefix.empty()) return true;
            if(name.size() < m_CompletionPrefix.size()) return false;
            for(size_t i = 0; i < m_CompletionPrefix.size(); ++i)
            {
                char a = (char)std::tolower((unsigned char)name[i]);
                char b = (char)std::tolower((unsigned char)m_CompletionPrefix[i]);
                if(a != b) return false;
            }
            return true;
        };

        auto pushEntry = [&](const LuaRegistry::Entry& e)
        {
            std::string name((const char*)e.Name.str, e.Name.size);
            if(!matchPrefix(name)) return;
            CompletionItem it;
            it.Display = name;
            it.Insert  = name;
            if(e.ReturnType.size > 0)
                it.Detail.assign((const char*)e.ReturnType.str, e.ReturnType.size);
            if(e.Signature.size > 0)
                it.Signature.assign((const char*)e.Signature.str, e.Signature.size);
            it.Kind = (int)e.Kind;
            m_CompletionItems.push_back(std::move(it));
        };

        if(m_CompletionIsMember)
        {
            if(m_CompletionOwner.empty()) return; // unknown receiver
            String8 owner8 = Str8StdS(m_CompletionOwner);
            for(size_t i = 0; i < all.Size(); ++i)
            {
                const auto& e = all[i];
                if(e.Owner.size != owner8.size) continue;
                if(memcmp(e.Owner.str, owner8.str, owner8.size) != 0) continue;
                if(e.Kind == LuaRegistry::EntryKind::Constructor) continue;
                pushEntry(e);
            }
        }
        else
        {
            // Bare prefix: globals + type names + locals.
            for(size_t i = 0; i < all.Size(); ++i)
            {
                const auto& e = all[i];
                if(e.Owner.size == 0 || e.Kind == LuaRegistry::EntryKind::Constructor)
                    pushEntry(e);
            }
            for(auto& pr : m_LocalsIndex)
            {
                if(!matchPrefix(pr.first)) continue;
                CompletionItem it;
                it.Display   = pr.first;
                it.Insert    = pr.first;
                it.Detail    = pr.second;
                it.Signature = "(local)";
                it.Kind      = -1;
                m_CompletionItems.push_back(std::move(it));
            }
        }

        std::sort(m_CompletionItems.begin(), m_CompletionItems.end(),
                  [](const CompletionItem& a, const CompletionItem& b)
                  { return a.Display < b.Display; });
        if(m_CompletionItems.size() > 60)
            m_CompletionItems.resize(60);
    }

    void TextEditPanel::DrawCompletionPopup()
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                               | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                               | ImGuiWindowFlags_NoDocking;
        // Distinct background so it stands out if it does render.
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.14f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        bool visible = ImGui::Begin("LuaComplete##popup", &m_CompletionOpen, flags);
        if(!visible)
        {
            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            return;
        }

        // Arrow keys nav (Cmd/Ctrl held so the editor doesn't move caret too).
        const ImGuiIO& io = ImGui::GetIO();
        const bool mod    = io.KeyCtrl || io.KeySuper;
        if(mod && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            m_CompletionSelected = (m_CompletionSelected + 1) % std::max(1, (int)m_CompletionItems.size());
        if(mod && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            int n = (int)m_CompletionItems.size();
            if(n > 0) m_CompletionSelected = (m_CompletionSelected - 1 + n) % n;
        }
        // Enter/Tab are read by the underlying text editor — use double-click
        // (or the explicit Insert button) to commit.
        const bool commit = false;

        const float detailCol = 320.0f;
        ImGui::BeginChild("##items", ImVec2(0, 230), false);
        for(int i = 0; i < (int)m_CompletionItems.size(); ++i)
        {
            const auto& it = m_CompletionItems[i];
            bool sel = (i == m_CompletionSelected);
            ImGui::PushID(i);
            if(ImGui::Selectable(it.Display.c_str(), sel,
                                 ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns))
                m_CompletionSelected = i;
            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                CommitCompletion(it);
                ImGui::PopID();
                ImGui::EndChild();
                ImGui::End();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
                return;
            }
            if(!it.Detail.empty())
            {
                ImGui::SameLine(detailCol);
                ImGui::TextDisabled("%s", it.Detail.c_str());
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        if(m_CompletionSelected >= 0 && m_CompletionSelected < (int)m_CompletionItems.size())
        {
            const auto& it = m_CompletionItems[m_CompletionSelected];
            ImGui::Separator();
            if(!it.Signature.empty())
                ImGui::TextUnformatted(it.Signature.c_str());
            if(ImGui::Button("Insert"))
            {
                CommitCompletion(it);
                ImGui::End();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
                return;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("dbl-click | Esc close | Cmd+Up/Down nav");
            (void)commit;
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    void TextEditPanel::CommitCompletion(const CompletionItem& it)
    {
        if(editor.IsReadOnly())
        {
            LWARN("[Complete] editor is read-only; toggle Edit > Read-only mode");
            m_CompletionOpen = false;
            return;
        }
        // Build the suffix we still need to add (anything beyond what was
        // already typed). Avoids InsertText's no-delete-selection quirk.
        const std::string& full   = it.Insert;
        const std::string& prefix = m_CompletionPrefix;
        std::string suffix;
        if(full.size() >= prefix.size() &&
           std::equal(prefix.begin(), prefix.end(), full.begin(),
                      [](char a, char b)
                      { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }))
        {
            suffix = full.substr(prefix.size());
        }
        else
        {
            // Names diverge — give up and just append the whole token.
            suffix = full;
        }
        if(!suffix.empty())
            editor.InsertText(suffix);
        m_CompletionOpen = false;
        m_TextUnsaved    = true;
    }
}
