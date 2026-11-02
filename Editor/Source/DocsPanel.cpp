#include "DocsPanel.h"
#include <Lumos/Core/Thread.h>
#include <Lumos/Export/ExportProject.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/OS/Memory.h>
#include <Lumos/Core/String.h>
#include <Lumos/Core/DataStructures/TDArray.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIDock.h>
#include <Lumos/Graphics/UITheme.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

namespace Lumos
{
    namespace
    {
        Arena* s_Arena = nullptr;
        String8 s_Path;
        TDArray<String8> s_Lines;
        bool s_Loaded = false;

        bool StartsWith(const String8& s, const char* prefix)
        {
            u64 n = strlen(prefix);
            return s.size >= n && MemoryCompare(s.str, prefix, n) == 0;
        }

        bool Load()
        {
            s_Loaded = true;
            s_Lines.Clear();
            if(!s_Arena)
                s_Arena = ArenaAlloc(Megabytes(1));
            ArenaClear(s_Arena);

            std::string src       = Export::EngineSourceRoot();
            std::string exe       = OS::Get().GetExecutablePath();
            std::string exeDir    = exe.substr(0, exe.find_last_of('/'));
            const std::string candidates[] = { src.empty() ? std::string() : src + "/Docs/Packaging.md", exeDir + "/../Resources/Docs/Packaging.md", exeDir + "/Docs/Packaging.md" };
            for(const std::string& c : candidates)
            {
                if(c.empty() || !FileSystem::FileExists(Str8StdS(c)))
                    continue;
                String8 text = FileSystem::ReadTextFile(s_Arena, Str8StdS(c));
                if(text.size == 0)
                    continue;
                s_Path = PushStr8Copy(s_Arena, c.c_str());
                u64 start = 0;
                for(u64 i = 0; i <= text.size; i++)
                {
                    if(i < text.size && text.str[i] != '\n')
                        continue;
                    u64 end = i;
                    if(end > start && text.str[end - 1] == '\r')
                        end--;
                    s_Lines.PushBack(Str8(text.str + start, end - start));
                    start = i + 1;
                }
                return true;
            }
            return false;
        }

        // `code` shown plain, ** markers dropped
        String8 Inline(Arena* arena, String8 s)
        {
            u8* out = PushArrayNoZero(arena, u8, s.size + 1);
            u64 n   = 0;
            for(u64 i = 0; i < s.size; i++)
            {
                if(s.str[i] == '`')
                    continue;
                if(s.str[i] == '*' && i + 1 < s.size && s.str[i + 1] == '*')
                {
                    i++;
                    continue;
                }
                out[n++] = s.str[i];
            }
            out[n] = 0;
            return Str8(out, n);
        }

        String8 Trim(String8 s)
        {
            while(s.size && s.str[0] == ' ')
                s = Str8Skip(s, 1);
            while(s.size && s.str[s.size - 1] == ' ')
                s = Str8Chop(s, 1);
            return s;
        }
    }

    void DocsPanel::Release()
    {
        if(s_Arena)
            ArenaRelease(s_Arena);
        s_Arena = nullptr;
        s_Lines.Clear();
        s_Loaded = false;
    }

    void DocsPanel::Draw(const UIDockPanelContext& context)
    {
        if(!s_Loaded)
            Load();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = context.DPIScale;
        const f32 width          = Maths::Max(60.0f, context.Size.x / dpi - 16.0f);

        if(s_Lines.Empty())
        {
            UILabelWrapped("##docsmissing", Str8Lit("Docs/Packaging.md wasn't found next to the engine source or the editor."), width);
            if(UIButton(ICON_MDI_REFRESH " Look again###docsretry").clicked)
                Load();
            return;
        }

        UIBeginRow();
        if(UIButton(ICON_MDI_REFRESH " Reload###docsreload").clicked)
            Load();
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelEllipsis("##docspath", s_Path, Maths::Max(40.0f, width - 90.0f));
        UIPopStyle(StyleVar_TextColor);
        UIEndRow();
        UISeparator();

        ArenaTemp scratch = ScratchBegin(0, 0);
        char id[32];
        for(u32 i = 0; i < s_Lines.Size(); i++)
        {
            const String8& line = s_Lines[i];
            snprintf(id, sizeof(id), "##docl%u", i);
            if(StartsWith(line, "```"))
            {
                // code block: monospace-ish plain text and a copy button
                u32 j      = i + 1;
                u64 bytes  = 0;
                for(; j < s_Lines.Size() && !StartsWith(s_Lines[j], "```"); j++)
                    bytes += s_Lines[j].size + 1;
                u8* code = PushArrayNoZero(scratch.arena, u8, bytes + 1);
                u64 at   = 0;
                for(u32 k = i + 1; k < j; k++)
                {
                    MemoryCopy(code + at, s_Lines[k].str, s_Lines[k].size);
                    at += s_Lines[k].size;
                    code[at++] = '\n';
                }
                if(at)
                    at--; // no trailing newline
                code[at] = 0;
                UIBeginRow();
                UIPushStyle(StyleVar_TextColor, theme.Accent);
                UILabel(id, Str8(code, at));
                UIPopStyle(StyleVar_TextColor);
                snprintf(id, sizeof(id), ICON_MDI_CONTENT_COPY "###doccopy%u", i);
                if(UIIconButton(id, 20.0f).clicked)
                    Input::Get().SetClipboard((const char*)code);
                UITooltip("Copy");
                UIEndRow();
                UISpacer(4.0f);
                i = j;
                continue;
            }
            if(StartsWith(line, "|"))
            {
                // table: cells joined with a gap, the header dimmed
                u32 j      = i;
                bool first = true;
                for(; j < s_Lines.Size() && StartsWith(s_Lines[j], "|"); j++)
                {
                    String8 row = s_Lines[j];
                    bool rule   = false;
                    for(u64 k = 0; k + 2 < row.size; k++)
                        rule |= row.str[k] == '-' && row.str[k + 1] == '-' && row.str[k + 2] == '-';
                    if(rule)
                        continue;
                    u8* out    = PushArrayNoZero(scratch.arena, u8, row.size * 2 + 1);
                    u64 n      = 0;
                    u64 cell   = 1;
                    for(u64 k = 1; k <= row.size; k++)
                    {
                        if(k < row.size && row.str[k] != '|')
                            continue;
                        String8 text = Inline(scratch.arena, Trim(Str8(row.str + cell, k - cell)));
                        if(n && text.size)
                        {
                            MemoryCopy(out + n, "   ", 3);
                            n += 3;
                        }
                        MemoryCopy(out + n, text.str, text.size);
                        n += text.size;
                        cell = k + 1;
                    }
                    snprintf(id, sizeof(id), "##doct%u", j);
                    if(first)
                        UIPushStyle(StyleVar_TextColor, theme.TextDim);
                    UILabelWrapped(id, Str8(out, n), width);
                    if(first)
                        UIPopStyle(StyleVar_TextColor);
                    first = false;
                }
                UISpacer(4.0f);
                i = j - 1;
                continue;
            }
            if(StartsWith(line, "# "))
            {
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.5f * dpi);
                UILabelWrapped(id, Str8Skip(line, 2), width);
                UIPopStyle(StyleVar_FontSize);
                UISpacer(4.0f);
            }
            else if(StartsWith(line, "## "))
            {
                UISpacer(6.0f);
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.2f * dpi);
                UILabelWrapped(id, Str8Skip(line, 3), width);
                UIPopStyle(StyleVar_FontSize);
                UISeparator();
            }
            else if(StartsWith(line, "- "))
            {
                String8 text = Inline(scratch.arena, Str8Skip(line, 2));
                UILabelWrapped(id, PushStr8F(scratch.arena, "-  %.*s", (int)text.size, text.str), width);
            }
            else if(line.size == 0)
                UISpacer(4.0f);
            else
                UILabelWrapped(id, Inline(scratch.arena, line), width);
        }
        ScratchEnd(scratch);
    }
}
