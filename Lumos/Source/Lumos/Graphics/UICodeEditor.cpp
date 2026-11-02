#include "Precompiled.h"
#include "UICodeEditor.h"

#include "Core/OS/Input.h"
#include "Core/Thread.h"
#include "Graphics/UIInternal.h"
#include "Graphics/UITheme.h"
#include "Graphics/Font.h"
#include "Maths/MathsUtilities.h"
#include "Scripting/Lua/LuaBindingRegistry.h"

namespace Lumos
{
    namespace
    {
        constexpr u32 kTabWidth      = 4;
        constexpr u32 kMaxLineBytes  = 1024; // longer lines are drawn (and hit-tested) cut short
        constexpr u32 kMaxItems      = 48;
        constexpr u32 kMaxShownItems = 8;

        enum LexState : u8
        {
            Lex_Normal = 0,
            Lex_BlockComment,
            Lex_LongString,
        };

        enum TokenColour : u8
        {
            Token_Text = 0,
            Token_Keyword,
            Token_String,
            Token_Number,
            Token_Comment,
            Token_Api,
            Token_Count
        };

        enum ItemKind : u8
        {
            Item_Keyword = 0,
            Item_Type,
            Item_Function,
            Item_Field,
            Item_Word,
        };

        struct Edit
        {
            u32 Pos;
            u32 RemovedAt; // into EditText
            u32 RemovedLen;
            u32 InsertedAt;
            u32 InsertedLen;
            u32 CursorBefore;
            u32 AnchorBefore;
            f32 Time;
            bool Typing;
        };

        struct CompleteItem
        {
            char Text[64];
            char Detail[72];
            u8 Kind;
            u8 Rank; // lower sorts first
        };

        const char* kLuaKeywords[] = {
            "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in",
            "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
        };

        bool IsIdentChar(char c)
        {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        }

        bool IsLuaKeyword(const char* text, u32 length)
        {
            for(const char* keyword : kLuaKeywords)
                if(strlen(keyword) == length && memcmp(keyword, text, length) == 0)
                    return true;
            return false;
        }

        char LowerChar(char c) { return (c >= 'A' && c <= 'Z') ? char(c + 32) : c; }

        bool StartsWithNoCase(const char* text, u32 textLength, const char* prefix, u32 prefixLength)
        {
            if(prefixLength > textLength)
                return false;
            for(u32 i = 0; i < prefixLength; i++)
                if(LowerChar(text[i]) != LowerChar(prefix[i]))
                    return false;
            return true;
        }
    }

    struct UICodeEditor
    {
        Arena* Memory = nullptr;

        TDArray<char> Text;
        TDArray<u32> Lines;    // offset of the first byte of each line
        TDArray<u8> LineState; // lexer state on entering each line

        u32 Cursor     = 0;
        u32 Anchor     = 0; // other end of the selection
        f32 WantedX    = -1.0f; // column kept while moving up / down
        f32 ScrollY    = 0.0f;
        f32 ScrollX    = 0.0f;
        f32 CaretTime  = 0.0f;
        f32 LastTime   = 0.0f;
        u64 Hash       = 0;
        bool Dirty     = false;
        bool Changed   = false;
        bool SaveAsked = false;
        bool Reveal    = false; // scroll the cursor into view
        bool Selecting = false; // mouse held since a press in the text
        bool WantFocus = false;

        // touch: a drag scrolls (and coasts), a tap puts the cursor, a hold then drag selects
        bool TouchDown   = false;
        bool TouchMoved  = false;
        bool TouchSelect = false;
        bool TouchDouble = false;
        bool TouchSideways = false;
        Vec2 TouchStart;
        Vec2 TouchLast;
        Vec2 TouchScroll;
        Vec2 Velocity; // px / s

        UICodeLanguage Language = UICodeLanguage_Plain;

        TDArray<Edit> Edits;
        TDArray<char> EditText;
        u32 EditIndex = 0; // edits before this one are applied

        bool CompleteOpen     = false;
        i32 CompleteSelected  = 0;
        u32 CompleteStart     = 0; // first byte of the word being completed
        u32 ItemCount         = 0;
        CompleteItem Items[kMaxItems];
    };

    namespace
    {
        // ---- text ----------------------------------------------------------

        u32 TextSize(const UICodeEditor* e) { return (u32)e->Text.Size(); }
        u32 LineCount(const UICodeEditor* e) { return (u32)e->Lines.Size(); }
        u32 LineStart(const UICodeEditor* e, u32 line) { return e->Lines[line]; }

        // one past the last byte of the line, not counting its newline
        u32 LineEnd(const UICodeEditor* e, u32 line)
        {
            return line + 1 < LineCount(e) ? e->Lines[line + 1] - 1 : TextSize(e);
        }

        u32 LineOf(const UICodeEditor* e, u32 offset)
        {
            u32 low = 0, high = LineCount(e);
            while(high - low > 1)
            {
                const u32 mid = (low + high) / 2;
                if(e->Lines[mid] <= offset)
                    low = mid;
                else
                    high = mid;
            }
            return low;
        }

        void RebuildLines(UICodeEditor* e)
        {
            e->Lines.Clear();
            e->LineState.Clear();
            e->Lines.PushBack(0);
            e->LineState.PushBack(Lex_Normal);

            const char* text = e->Text.Data();
            const u32 size   = TextSize(e);
            const bool lua   = e->Language == UICodeLanguage_Lua;
            u8 state         = Lex_Normal;
            char quote       = 0;

            for(u32 i = 0; i < size; i++)
            {
                const char c = text[i];
                if(c == '\n')
                {
                    quote = 0; // a quoted string does not run past its line
                    e->Lines.PushBack(i + 1);
                    e->LineState.PushBack(state);
                    continue;
                }
                if(!lua)
                    continue;

                if(state != Lex_Normal)
                {
                    if(c == ']' && i + 1 < size && text[i + 1] == ']')
                    {
                        state = Lex_Normal;
                        i++;
                    }
                }
                else if(quote)
                {
                    if(c == '\\')
                        i++;
                    else if(c == quote)
                        quote = 0;
                }
                else if(c == '"' || c == '\'')
                    quote = c;
                else if(c == '-' && i + 1 < size && text[i + 1] == '-')
                {
                    if(i + 3 < size && text[i + 2] == '[' && text[i + 3] == '[')
                    {
                        state = Lex_BlockComment;
                        i += 3;
                    }
                    else
                    {
                        // to the end of the line
                        while(i + 1 < size && text[i + 1] != '\n')
                            i++;
                    }
                }
                else if(c == '[' && i + 1 < size && text[i + 1] == '[')
                {
                    state = Lex_LongString;
                    i++;
                }
            }
        }

        void ApplyReplace(UICodeEditor* e, u32 pos, u32 removeLength, const char* inserted, u32 insertLength)
        {
            const u32 size    = TextSize(e);
            const u32 newSize = size - removeLength + insertLength;
            const u32 tail    = size - (pos + removeLength);

            if(newSize > size)
                e->Text.Resize(newSize);
            char* data = e->Text.Data();
            if(tail > 0 && insertLength != removeLength)
                memmove(data + pos + insertLength, data + pos + removeLength, tail);
            if(insertLength > 0)
                memcpy(data + pos, inserted, insertLength);
            if(newSize < size)
                e->Text.Resize(newSize);

            RebuildLines(e);
            e->Dirty   = true;
            e->Changed = true;
        }

        // Every change to the text goes through here, so it can be undone.
        void Replace(UICodeEditor* e, u32 pos, u32 removeLength, const char* inserted, u32 insertLength, bool typing = false)
        {
            const u32 size = TextSize(e);
            pos            = Maths::Min(pos, size);
            removeLength   = Maths::Min(removeLength, size - pos);
            if(removeLength == 0 && insertLength == 0)
                return;

            // anything undone is gone once a new edit is made
            if(e->EditIndex < (u32)e->Edits.Size())
            {
                e->EditText.Resize(e->Edits[e->EditIndex].RemovedAt);
                e->Edits.Resize(e->EditIndex);
            }

            // a run of typed characters is one step
            bool merged = false;
            if(typing && removeLength == 0 && insertLength == 1 && IsIdentChar(inserted[0]) && e->EditIndex > 0)
            {
                Edit& last = e->Edits[e->EditIndex - 1];
                if(last.Typing && last.RemovedLen == 0 && last.Pos + last.InsertedLen == pos
                   && last.InsertedAt + last.InsertedLen == (u32)e->EditText.Size() && e->LastTime - last.Time < 1.0f)
                {
                    e->EditText.PushBack(inserted[0]);
                    last.InsertedLen++;
                    last.Time = e->LastTime;
                    merged    = true;
                }
            }

            if(!merged)
            {
                Edit edit;
                edit.Pos          = pos;
                edit.RemovedAt    = (u32)e->EditText.Size();
                edit.RemovedLen   = removeLength;
                edit.InsertedAt   = edit.RemovedAt + removeLength;
                edit.InsertedLen  = insertLength;
                edit.CursorBefore = e->Cursor;
                edit.AnchorBefore = e->Anchor;
                edit.Time         = e->LastTime;
                edit.Typing       = typing;

                const u32 at = edit.RemovedAt;
                e->EditText.Resize(at + removeLength + insertLength);
                if(removeLength)
                    memcpy(e->EditText.Data() + at, e->Text.Data() + pos, removeLength);
                if(insertLength)
                    memcpy(e->EditText.Data() + at + removeLength, inserted, insertLength);
                e->Edits.PushBack(edit);
                e->EditIndex = (u32)e->Edits.Size();
            }

            ApplyReplace(e, pos, removeLength, inserted, insertLength);
            e->Cursor = e->Anchor = pos + insertLength;
            e->WantedX            = -1.0f;
            e->Reveal             = true;
        }

        void Undo(UICodeEditor* e)
        {
            if(e->EditIndex == 0)
                return;
            const Edit edit = e->Edits[--e->EditIndex];
            ApplyReplace(e, edit.Pos, edit.InsertedLen, e->EditText.Data() + edit.RemovedAt, edit.RemovedLen);
            e->Cursor = Maths::Min(edit.CursorBefore, TextSize(e));
            e->Anchor = Maths::Min(edit.AnchorBefore, TextSize(e));
            e->Reveal = true;
        }

        void Redo(UICodeEditor* e)
        {
            if(e->EditIndex >= (u32)e->Edits.Size())
                return;
            const Edit edit = e->Edits[e->EditIndex++];
            ApplyReplace(e, edit.Pos, edit.RemovedLen, e->EditText.Data() + edit.InsertedAt, edit.InsertedLen);
            e->Cursor = e->Anchor = edit.Pos + edit.InsertedLen;
            e->Reveal             = true;
        }

        bool HasSelection(const UICodeEditor* e) { return e->Cursor != e->Anchor; }
        u32 SelectionStart(const UICodeEditor* e) { return Maths::Min(e->Cursor, e->Anchor); }
        u32 SelectionEnd(const UICodeEditor* e) { return Maths::Max(e->Cursor, e->Anchor); }

        void InsertAtCursor(UICodeEditor* e, const char* text, u32 length, bool typing = false)
        {
            if(HasSelection(e))
                Replace(e, SelectionStart(e), SelectionEnd(e) - SelectionStart(e), text, length, false);
            else
                Replace(e, e->Cursor, 0, text, length, typing);
        }

        // ---- measuring -----------------------------------------------------

        // The line as it is drawn (tabs opened out). Returns the display length; `column` bytes
        // of the line map to *outColumn display bytes.
        u32 DisplayLine(const UICodeEditor* e, u32 line, char* out, u32 outSize, u32 column = 0, u32* outColumn = nullptr)
        {
            const char* text = e->Text.Data() + LineStart(e, line);
            const u32 length = Maths::Min(LineEnd(e, line) - LineStart(e, line), kMaxLineBytes);
            u32 n            = 0;
            if(outColumn)
                *outColumn = 0;
            for(u32 i = 0; i < length && n + kTabWidth < outSize; i++)
            {
                if(outColumn && i == column)
                    *outColumn = n;
                if(text[i] == '\t')
                {
                    const u32 stop = (n / kTabWidth + 1) * kTabWidth;
                    while(n < stop)
                        out[n++] = ' ';
                }
                else
                    out[n++] = text[i];
            }
            if(outColumn && column >= length)
                *outColumn = n;
            out[n] = 0;
            return n;
        }

        // Where the pen is after `length` bytes. The size of a string stops at the ink of its last
        // glyph (nothing for a space, short of the advance for most letters), so a mark is measured
        // after it and taken off again.
        f32 Width(const char* text, u32 length, f32 fontSize)
        {
            if(length == 0)
                return 0.0f;
            Graphics::Font* font   = Graphics::Font::GetMonoFont().get();
            static f32 s_MarkSize  = 0.0f;
            static f32 s_MarkWidth = 0.0f;
            if(s_MarkSize != fontSize)
            {
                s_MarkSize  = fontSize;
                s_MarkWidth = font->CalculateTextSize(Str8Lit("|"), fontSize).x;
            }
            char buffer[kMaxLineBytes * 2 + 2];
            length = Maths::Min(length, (u32)sizeof(buffer) - 2);
            memcpy(buffer, text, length);
            buffer[length] = '|';
            return Maths::Max(0.0f, font->CalculateTextSize(Str8((u8*)buffer, length + 1), fontSize).x - s_MarkWidth);
        }

        f32 XOfOffset(const UICodeEditor* e, u32 offset, f32 fontSize)
        {
            const u32 line = LineOf(e, offset);
            char display[kMaxLineBytes * 2];
            u32 column = 0;
            DisplayLine(e, line, display, sizeof(display), offset - LineStart(e, line), &column);
            return Width(display, column, fontSize);
        }

        u32 OffsetAtX(const UICodeEditor* e, u32 line, f32 x, f32 fontSize)
        {
            const u32 start  = LineStart(e, line);
            const u32 length = Maths::Min(LineEnd(e, line) - start, kMaxLineBytes);
            char display[kMaxLineBytes * 2];
            u32 best     = 0;
            f32 bestGap  = Maths::Abs(x);
            for(u32 i = 1; i <= length; i++)
            {
                u32 column = 0;
                DisplayLine(e, line, display, sizeof(display), i, &column);
                const f32 gap = Maths::Abs(x - Width(display, column, fontSize));
                if(gap < bestGap)
                {
                    bestGap = gap;
                    best    = i;
                }
                else if(gap > bestGap + fontSize * 2.0f)
                    break; // well past it
            }
            return start + best;
        }

        // ---- completion ----------------------------------------------------

        String8 TypeOfChain(const UICodeEditor* e, const char* chain, u32 length, i32 depth);

        // `local name = <something>` anywhere in the file: the type of the something
        String8 TypeOfLocal(const UICodeEditor* e, const char* name, u32 nameLength, i32 depth)
        {
            if(depth > 2)
                return {};
            const char* text = e->Text.Data();
            const u32 size   = TextSize(e);
            for(u32 line = 0; line < LineCount(e); line++)
            {
                u32 i         = LineStart(e, line);
                const u32 end = LineEnd(e, line);
                while(i < end && (text[i] == ' ' || text[i] == '\t'))
                    i++;
                if(i + 6 > end || memcmp(text + i, "local ", 6) != 0)
                    continue;
                i += 6;
                while(i < end && text[i] == ' ')
                    i++;
                if(i + nameLength > end || memcmp(text + i, name, nameLength) != 0 || (i + nameLength < size && IsIdentChar(text[i + nameLength])))
                    continue;
                i += nameLength;
                while(i < end && text[i] == ' ')
                    i++;
                if(i >= end || text[i] != '=')
                    continue;
                i++;
                while(i < end && text[i] == ' ')
                    i++;
                // the chain the value starts with: names, dots, colons and call brackets
                const u32 start = i;
                i32 brackets    = 0;
                while(i < end && (IsIdentChar(text[i]) || text[i] == '.' || text[i] == ':' || text[i] == '(' || text[i] == ')' || brackets > 0))
                {
                    if(text[i] == '(')
                        brackets++;
                    else if(text[i] == ')')
                        brackets--;
                    i++;
                }
                String8 type = TypeOfChain(e, text + start, i - start, depth + 1);
                if(type.size > 0)
                    return type;
            }
            return {};
        }

        // "player:GetTransform().Position" -> the type the whole thing gives
        String8 TypeOfChain(const UICodeEditor* e, const char* chain, u32 length, i32 depth)
        {
            String8 type = {};
            bool first   = true;
            u32 i        = 0;
            while(i < length)
            {
                const u32 start = i;
                while(i < length && IsIdentChar(chain[i]))
                    i++;
                const String8 token = Str8((u8*)chain + start, i - start);
                if(token.size == 0)
                    return {};

                if(first)
                {
                    first = false;
                    if(LuaRegistry::IsType(token))
                    {
                        // Type.new(...) / Type(...) is a value of that type
                        type = token;
                        if(i + 4 <= length && memcmp(chain + i, ".new", 4) == 0)
                            i += 4;
                    }
                    else
                    {
                        type = TypeOfLocal(e, (const char*)token.str, (u32)token.size, depth);
                        if(type.size == 0)
                            if(const LuaRegistry::Entry* entry = LuaRegistry::FindMember(String8 {}, token))
                                type = entry->ReturnType;
                    }
                }
                else
                {
                    const LuaRegistry::Entry* entry = LuaRegistry::FindMember(type, token);
                    type                            = entry ? entry->ReturnType : String8 {};
                }
                if(type.size == 0)
                    return {};

                // skip a call's brackets
                if(i < length && chain[i] == '(')
                {
                    i32 brackets = 0;
                    while(i < length)
                    {
                        if(chain[i] == '(')
                            brackets++;
                        else if(chain[i] == ')' && --brackets == 0)
                        {
                            i++;
                            break;
                        }
                        i++;
                    }
                }
                if(i < length && (chain[i] == '.' || chain[i] == ':'))
                    i++;
                else
                    break;
            }
            return type;
        }

        void AddItem(UICodeEditor* e, const char* text, u32 length, const char* detail, u32 detailLength, u8 kind, const char* prefix, u32 prefixLength)
        {
            if(length == 0 || length >= sizeof(CompleteItem::Text) || !StartsWithNoCase(text, length, prefix, prefixLength))
                return;
            for(u32 i = 0; i < e->ItemCount; i++)
                if(strlen(e->Items[i].Text) == length && memcmp(e->Items[i].Text, text, length) == 0)
                    return;
            if(e->ItemCount >= kMaxItems)
                return;

            CompleteItem& item = e->Items[e->ItemCount++];
            memcpy(item.Text, text, length);
            item.Text[length] = 0;
            detailLength      = Maths::Min(detailLength, (u32)sizeof(item.Detail) - 1);
            if(detailLength)
                memcpy(item.Detail, detail, detailLength);
            item.Detail[detailLength] = 0;
            item.Kind                 = kind;
            // same case as typed first, then by where it came from
            item.Rank = (u8)((prefixLength > 0 && memcmp(text, prefix, prefixLength) == 0 ? 0 : 8) + kind);
        }

        void CloseCompletion(UICodeEditor* e)
        {
            e->CompleteOpen = false;
            e->ItemCount    = 0;
        }

        // Works out what could go at the cursor. `force` = asked for by hand (Ctrl+Space).
        void UpdateCompletion(UICodeEditor* e, bool force)
        {
            CloseCompletion(e);
            if(e->Language != UICodeLanguage_Lua || HasSelection(e))
                return;

            const char* text    = e->Text.Data();
            const u32 line      = LineOf(e, e->Cursor);
            const u32 lineStart = LineStart(e, line);
            if(e->LineState[line] != Lex_Normal)
                return;

            u32 wordStart = e->Cursor;
            while(wordStart > lineStart && IsIdentChar(text[wordStart - 1]))
                wordStart--;
            const char* prefix     = text + wordStart;
            const u32 prefixLength = e->Cursor - wordStart;
            if(prefixLength > 0 && prefix[0] >= '0' && prefix[0] <= '9')
                return;

            const bool member = wordStart > lineStart && (text[wordStart - 1] == '.' || text[wordStart - 1] == ':');
            if(!member && prefixLength < 2 && !force)
                return;

            if(member)
            {
                // back over the chain the dot hangs off
                u32 chainStart = wordStart - 1;
                i32 brackets   = 0;
                while(chainStart > lineStart)
                {
                    const char c = text[chainStart - 1];
                    if(c == ')')
                        brackets++;
                    else if(c == '(')
                    {
                        if(brackets == 0)
                            break;
                        brackets--;
                    }
                    else if(brackets == 0 && !IsIdentChar(c) && c != '.' && c != ':')
                        break;
                    chainStart--;
                }
                const String8 owner = TypeOfChain(e, text + chainStart, wordStart - 1 - chainStart, 0);
                if(owner.size == 0)
                    return;

                const TDArray<LuaRegistry::Entry>& entries = LuaRegistry::All();
                for(u32 i = 0; i < (u32)entries.Size(); i++)
                {
                    const LuaRegistry::Entry& entry = entries[i];
                    if(entry.Owner.size != owner.size || memcmp(entry.Owner.str, owner.str, owner.size) != 0)
                        continue;
                    const bool function = entry.Kind == LuaRegistry::EntryKind::Method || entry.Kind == LuaRegistry::EntryKind::Constructor;
                    const String8 detail = entry.Signature.size > 0 ? entry.Signature : entry.ReturnType;
                    AddItem(e, (const char*)entry.Name.str, (u32)entry.Name.size, (const char*)detail.str, (u32)detail.size,
                            function ? Item_Function : Item_Field, prefix, prefixLength);
                }
            }
            else
            {
                for(const char* keyword : kLuaKeywords)
                    AddItem(e, keyword, (u32)strlen(keyword), "keyword", 7, Item_Keyword, prefix, prefixLength);

                const TDArray<LuaRegistry::Entry>& entries = LuaRegistry::All();
                for(u32 i = 0; i < (u32)entries.Size(); i++)
                {
                    const LuaRegistry::Entry& entry = entries[i];
                    if(entry.Owner.size == 0)
                    {
                        const String8 detail = entry.Signature.size > 0 ? entry.Signature : entry.ReturnType;
                        AddItem(e, (const char*)entry.Name.str, (u32)entry.Name.size, (const char*)detail.str, (u32)detail.size, Item_Function, prefix, prefixLength);
                    }
                    else
                        AddItem(e, (const char*)entry.Owner.str, (u32)entry.Owner.size, "type", 4, Item_Type, prefix, prefixLength);
                }

                // words already in the file (not the one being typed)
                const u32 size = TextSize(e);
                for(u32 i = 0; i < size && e->ItemCount < kMaxItems;)
                {
                    if(!IsIdentChar(text[i]) || (text[i] >= '0' && text[i] <= '9'))
                    {
                        i++;
                        continue;
                    }
                    const u32 start = i;
                    while(i < size && IsIdentChar(text[i]))
                        i++;
                    if(start != wordStart && i - start > 2)
                        AddItem(e, text + start, i - start, "", 0, Item_Word, prefix, prefixLength);
                }
            }

            // nothing to offer beyond what is already typed
            if(e->ItemCount == 0 || (e->ItemCount == 1 && strlen(e->Items[0].Text) == prefixLength && memcmp(e->Items[0].Text, prefix, prefixLength) == 0))
            {
                CloseCompletion(e);
                return;
            }

            // by rank, then name
            for(u32 i = 1; i < e->ItemCount; i++)
            {
                const CompleteItem item = e->Items[i];
                i32 j                   = (i32)i - 1;
                while(j >= 0 && (e->Items[j].Rank > item.Rank || (e->Items[j].Rank == item.Rank && strcmp(e->Items[j].Text, item.Text) > 0)))
                {
                    e->Items[j + 1] = e->Items[j];
                    j--;
                }
                e->Items[j + 1] = item;
            }

            e->CompleteOpen     = true;
            e->CompleteSelected = 0;
            e->CompleteStart    = wordStart;
        }

        void AcceptCompletion(UICodeEditor* e)
        {
            if(!e->CompleteOpen || e->CompleteSelected < 0 || e->CompleteSelected >= (i32)e->ItemCount)
                return;
            const CompleteItem item = e->Items[e->CompleteSelected];
            const u32 start         = Maths::Min(e->CompleteStart, e->Cursor);
            e->Anchor               = e->Cursor;
            Replace(e, start, e->Cursor - start, item.Text, (u32)strlen(item.Text));
            CloseCompletion(e);
        }

        // ---- editing keys --------------------------------------------------

        u32 WordLeft(const UICodeEditor* e, u32 offset)
        {
            const char* text = e->Text.Data();
            while(offset > 0 && !IsIdentChar(text[offset - 1]))
                offset--;
            while(offset > 0 && IsIdentChar(text[offset - 1]))
                offset--;
            return offset;
        }

        u32 WordRight(const UICodeEditor* e, u32 offset)
        {
            const char* text = e->Text.Data();
            const u32 size   = TextSize(e);
            while(offset < size && !IsIdentChar(text[offset]))
                offset++;
            while(offset < size && IsIdentChar(text[offset]))
                offset++;
            return offset;
        }

        void MoveTo(UICodeEditor* e, u32 offset, bool select, bool keepColumn = false)
        {
            e->Cursor = Maths::Min(offset, TextSize(e));
            if(!select)
                e->Anchor = e->Cursor;
            if(!keepColumn)
                e->WantedX = -1.0f;
            e->Reveal    = true;
            e->CaretTime = 0.0f;
        }

        void MoveVertical(UICodeEditor* e, i32 lines, bool select, f32 fontSize)
        {
            const u32 line = LineOf(e, e->Cursor);
            if(e->WantedX < 0.0f)
                e->WantedX = XOfOffset(e, e->Cursor, fontSize);
            const i32 target = Maths::Clamp((i32)line + lines, 0, (i32)LineCount(e) - 1);
            MoveTo(e, OffsetAtX(e, (u32)target, e->WantedX, fontSize), select, true);
        }

        void CopySelection(const UICodeEditor* e)
        {
            if(!HasSelection(e))
                return;
            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            const u32 length  = SelectionEnd(e) - SelectionStart(e);
            char* copy        = PushArrayNoZero(scratch.arena, char, length + 1);
            memcpy(copy, e->Text.Data() + SelectionStart(e), length);
            copy[length] = 0;
            Input::Get().SetClipboard(copy);
            ScratchEnd(scratch);
        }

        void NewLine(UICodeEditor* e)
        {
            const char* text = e->Text.Data();
            const u32 line   = LineOf(e, SelectionStart(e));
            const u32 start  = LineStart(e, line);
            const u32 cursor = SelectionStart(e);

            char inserted[160];
            u32 n          = 0;
            inserted[n++]  = '\n';
            // same indent as the line being left
            for(u32 i = start; i < cursor && (text[i] == ' ' || text[i] == '\t') && n < 120; i++)
                inserted[n++] = text[i];

            // one step in after something that opens a block
            if(e->Language == UICodeLanguage_Lua)
            {
                u32 end = cursor;
                while(end > start && (text[end - 1] == ' ' || text[end - 1] == '\t'))
                    end--;
                u32 wordStart = end;
                while(wordStart > start && IsIdentChar(text[wordStart - 1]))
                    wordStart--;
                const u32 wordLength = end - wordStart;
                auto ends            = [&](const char* word) { return strlen(word) == wordLength && memcmp(text + wordStart, word, wordLength) == 0; };
                bool opens           = ends("then") || ends("do") || ends("else") || ends("repeat") || (end > start && (text[end - 1] == '{' || text[end - 1] == '('));
                // function name(...) on its own line
                if(!opens && end > start && text[end - 1] == ')')
                {
                    for(u32 i = start; i + 8 <= end; i++)
                        if(memcmp(text + i, "function", 8) == 0)
                            opens = true;
                }
                if(opens)
                    for(u32 i = 0; i < kTabWidth; i++)
                        inserted[n++] = ' ';
            }
            InsertAtCursor(e, inserted, n);
        }

        // Tab with lines selected: every one of them moves in or out a step.
        void IndentSelection(UICodeEditor* e, bool outdent)
        {
            const u32 firstLine = LineOf(e, SelectionStart(e));
            u32 lastLine        = LineOf(e, SelectionEnd(e));
            if(lastLine > firstLine && SelectionEnd(e) == LineStart(e, lastLine))
                lastLine--;

            const u32 from    = LineStart(e, firstLine);
            const u32 to      = LineEnd(e, lastLine);
            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            char* out         = PushArrayNoZero(scratch.arena, char, (to - from) + (lastLine - firstLine + 1) * kTabWidth + 1);
            u32 n             = 0;
            const char* text  = e->Text.Data();
            for(u32 line = firstLine; line <= lastLine; line++)
            {
                u32 i         = LineStart(e, line);
                const u32 end = LineEnd(e, line);
                if(outdent)
                {
                    u32 removed = 0;
                    while(i < end && removed < kTabWidth && (text[i] == ' ' || text[i] == '\t'))
                    {
                        removed += text[i] == '\t' ? kTabWidth : 1;
                        i++;
                    }
                }
                else if(end > i)
                {
                    for(u32 k = 0; k < kTabWidth; k++)
                        out[n++] = ' ';
                }
                memcpy(out + n, text + i, end - i);
                n += end - i;
                if(line < lastLine)
                    out[n++] = '\n';
            }
            Replace(e, from, to - from, out, n);
            e->Anchor = from;
            e->Cursor = from + n;
            ScratchEnd(scratch);
        }

        void HandleKey(UICodeEditor* e, const UIKeyEvent& event, f32 fontSize, f32 pageLines)
        {
            const u32 size = TextSize(e);

            if(event.Character)
            {
                if(event.Ctrl)
                    return;
                const char c = event.Character;
                InsertAtCursor(e, &c, 1, true);
                if(IsIdentChar(c) || c == '.' || c == ':')
                    UpdateCompletion(e, false);
                else
                    CloseCompletion(e);
                return;
            }

            const InputCode::Key key = (InputCode::Key)event.Key;

            // the list takes these while it is up
            if(e->CompleteOpen)
            {
                if(key == InputCode::Key::Down || key == InputCode::Key::Up)
                {
                    const i32 count     = (i32)e->ItemCount;
                    e->CompleteSelected = (e->CompleteSelected + (key == InputCode::Key::Down ? 1 : count - 1)) % count;
                    return;
                }
                if(key == InputCode::Key::Enter || key == InputCode::Key::Tab)
                {
                    AcceptCompletion(e);
                    return;
                }
                if(key == InputCode::Key::Escape)
                {
                    CloseCompletion(e);
                    return;
                }
            }

            bool keepCompletion = false;
            switch(key)
            {
            case InputCode::Key::Left:
                if(HasSelection(e) && !event.Shift)
                    MoveTo(e, SelectionStart(e), false);
                else
                    MoveTo(e, event.Ctrl || event.Alt ? WordLeft(e, e->Cursor) : (e->Cursor > 0 ? e->Cursor - 1 : 0), event.Shift);
                break;
            case InputCode::Key::Right:
                if(HasSelection(e) && !event.Shift)
                    MoveTo(e, SelectionEnd(e), false);
                else
                    MoveTo(e, event.Ctrl || event.Alt ? WordRight(e, e->Cursor) : e->Cursor + 1, event.Shift);
                break;
            case InputCode::Key::Up:
                if(event.Ctrl)
                    MoveTo(e, 0, event.Shift);
                else
                    MoveVertical(e, -1, event.Shift, fontSize);
                break;
            case InputCode::Key::Down:
                if(event.Ctrl)
                    MoveTo(e, size, event.Shift);
                else
                    MoveVertical(e, 1, event.Shift, fontSize);
                break;
            case InputCode::Key::PageUp: MoveVertical(e, -(i32)pageLines, event.Shift, fontSize); break;
            case InputCode::Key::PageDown: MoveVertical(e, (i32)pageLines, event.Shift, fontSize); break;
            case InputCode::Key::Home:
            {
                // first to the text, then to the margin
                const u32 line  = LineOf(e, e->Cursor);
                const u32 start = LineStart(e, line);
                u32 indent      = start;
                while(indent < LineEnd(e, line) && (e->Text[indent] == ' ' || e->Text[indent] == '\t'))
                    indent++;
                MoveTo(e, e->Cursor == indent ? start : indent, event.Shift);
                break;
            }
            case InputCode::Key::End: MoveTo(e, LineEnd(e, LineOf(e, e->Cursor)), event.Shift); break;
            case InputCode::Key::Backspace:
                if(HasSelection(e))
                    Replace(e, SelectionStart(e), SelectionEnd(e) - SelectionStart(e), nullptr, 0);
                else if(e->Cursor > 0)
                {
                    const u32 from = event.Ctrl || event.Alt ? WordLeft(e, e->Cursor) : e->Cursor - 1;
                    Replace(e, from, e->Cursor - from, nullptr, 0);
                    if(e->Cursor > 0 && IsIdentChar(e->Text[e->Cursor - 1]))
                    {
                        UpdateCompletion(e, false);
                        keepCompletion = true;
                    }
                }
                break;
            case InputCode::Key::Delete:
                if(HasSelection(e))
                    Replace(e, SelectionStart(e), SelectionEnd(e) - SelectionStart(e), nullptr, 0);
                else if(e->Cursor < size)
                {
                    const u32 to = event.Ctrl || event.Alt ? WordRight(e, e->Cursor) : e->Cursor + 1;
                    Replace(e, e->Cursor, to - e->Cursor, nullptr, 0);
                }
                break;
            case InputCode::Key::Enter: NewLine(e); break;
            case InputCode::Key::Tab:
                // Ctrl + Tab belongs to whoever switches panels
                if(Input::Get().GetKeyHeldRaw(InputCode::Key::LeftControl) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightControl))
                    break;
                if(HasSelection(e) && LineOf(e, SelectionStart(e)) != LineOf(e, SelectionEnd(e)))
                    IndentSelection(e, event.Shift);
                else if(event.Shift)
                    IndentSelection(e, true);
                else
                {
                    // to the next stop
                    const u32 column = e->Cursor - LineStart(e, LineOf(e, e->Cursor));
                    const u32 count  = kTabWidth - (column % kTabWidth);
                    InsertAtCursor(e, "    ", count);
                }
                break;
            case InputCode::Key::A:
                if(event.Ctrl)
                {
                    e->Anchor = 0;
                    e->Cursor = size;
                }
                break;
            case InputCode::Key::C:
                if(event.Ctrl)
                    CopySelection(e);
                break;
            case InputCode::Key::X:
                if(event.Ctrl && HasSelection(e))
                {
                    CopySelection(e);
                    Replace(e, SelectionStart(e), SelectionEnd(e) - SelectionStart(e), nullptr, 0);
                }
                break;
            case InputCode::Key::V:
                if(event.Ctrl)
                {
                    std::string clipboard = Input::Get().GetClipboard();
                    // only the characters the font draws, and Unix line ends
                    u32 n = 0;
                    for(size_t i = 0; i < clipboard.size(); i++)
                        if(clipboard[i] != '\r')
                            clipboard[n++] = clipboard[i];
                    if(n > 0)
                        InsertAtCursor(e, clipboard.data(), n);
                }
                break;
            case InputCode::Key::Z:
                if(event.Ctrl)
                {
                    if(event.Shift)
                        Redo(e);
                    else
                        Undo(e);
                }
                break;
            case InputCode::Key::Y:
                if(event.Ctrl)
                    Redo(e);
                break;
            case InputCode::Key::S:
                if(event.Ctrl)
                    e->SaveAsked = true;
                break;
            case InputCode::Key::Space:
                if(event.Ctrl)
                {
                    UpdateCompletion(e, true);
                    keepCompletion = true;
                }
                break;
            default:
                keepCompletion = true; // a modifier on its own
                break;
            }

            if(!keepCompletion)
                CloseCompletion(e);
        }

        // ---- drawing -------------------------------------------------------

        UI_Widget* Quad(u64 hash, const Vec2& position, const Vec2& size, const Vec4& colour, f32 rounding = 0.0f)
        {
            UI_Widget* quad = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""), hash,
                                         { SizeKind_Pixels, size.x }, { SizeKind_Pixels, size.y });
            quad->relative_position                 = position;
            quad->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            quad->style_vars[StyleVar_CornerRadius] = Vec4(rounding, 0.0f, 0.0f, 0.0f);
            UIClearBorders(quad);
            UISetBackground(quad, colour, colour, colour);
            return quad;
        }

        UI_Widget* TextAt(u64 hash, const Vec2& position, const char* text, u32 length, const Vec4& colour, f32 fontSize)
        {
            // sized by hand: the layout would measure it in the other font
            Graphics::Font* font = Graphics::Font::GetMonoFont().get();
            // every run the same height (the measured one follows the ink, and short glyphs would ride high)
            const Vec2 size      = font->CalculateTextSize(Str8((u8*)text, length), fontSize);
            const f32 height     = font->CalculateTextSize(Str8Lit("A"), fontSize).y;
            UI_Widget* label     = PushWidget(WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8((u8*)text, length), hash,
                                              { SizeKind_Pixels, ceilf(size.x) + 2.0f }, { SizeKind_Pixels, ceilf(height) });
            label->MonoText                        = true;
            label->relative_position               = position;
            label->style_vars[StyleVar_Padding]    = Vec4(0.0f);
            label->style_vars[StyleVar_FontSize]   = Vec4(fontSize, 0.0f, 0.0f, 0.0f);
            UISetTextColour(label, colour);
            return label;
        }

        struct Run
        {
            u16 Start;
            u16 Length;
            u8 Colour;
        };

        // Colour runs of one displayed line. Whitespace joins the run before it, so a line is few widgets.
        u32 LexLine(const UICodeEditor* e, const char* text, u32 length, u8 state, Run* runs, u32 maxRuns)
        {
            u32 count = 0;
            auto push = [&](u32 start, u32 end, u8 colour)
            {
                if(end <= start)
                    return;
                if(count > 0 && (runs[count - 1].Colour == colour) && runs[count - 1].Start + runs[count - 1].Length == start)
                {
                    runs[count - 1].Length = (u16)(runs[count - 1].Length + (end - start));
                    return;
                }
                if(count >= maxRuns)
                {
                    runs[count - 1].Length = (u16)(end - runs[count - 1].Start);
                    return;
                }
                runs[count++] = { (u16)start, (u16)(end - start), colour };
            };

            if(e->Language == UICodeLanguage_Config || e->Language == UICodeLanguage_Shader)
            {
                const bool shader = e->Language == UICodeLanguage_Shader;
                auto isWord       = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; };
                auto wordIs       = [&](u32 start, u32 end, const char* const* list, u32 listCount)
                {
                    for(u32 k = 0; k < listCount; k++)
                        if(strlen(list[k]) == end - start && memcmp(list[k], text + start, end - start) == 0)
                            return true;
                    return false;
                };
                static const char* const kConfigWords[] = { "true", "false", "null", "yes", "no", "on", "off", "nil" };
                static const char* const kShaderWords[] = { "void", "float", "int", "uint", "bool", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "mat3", "mat4",
                                                            "sampler2D", "samplerCube", "sampler2DArray", "uniform", "layout", "in", "out", "inout", "const", "struct",
                                                            "if", "else", "for", "while", "return", "discard", "break", "continue", "flat", "true", "false",
                                                            "#include", "#define", "#ifdef", "#ifndef", "#endif", "#if", "#else", "#version", "#extension" };
                // a config line's key: what comes before the first : or =
                u32 keyEnd = 0;
                if(!shader)
                    for(u32 k = 0; k < length; k++)
                    {
                        if(text[k] == '"' || text[k] == '#' || text[k] == ';')
                            break;
                        if(text[k] == ':' || text[k] == '=')
                        {
                            keyEnd = k;
                            break;
                        }
                    }

                u32 i = 0;
                while(i < length)
                {
                    const char c = text[i];
                    const bool lineComment = (c == '/' && i + 1 < length && text[i + 1] == '/') || (!shader && (c == '#' || (c == ';' && i == 0)));
                    if(lineComment)
                    {
                        push(i, length, Token_Comment);
                        break;
                    }
                    if(c == '"' || (!shader && c == '\''))
                    {
                        u32 end = i + 1;
                        while(end < length && text[end] != c)
                            end += (text[end] == '\\' && end + 1 < length) ? 2 : 1;
                        end = Maths::Min(end + 1, length);
                        // "key": in json reads as a key, not a string
                        u32 after = end;
                        while(after < length && text[after] == ' ')
                            after++;
                        push(i, end, (!shader && after < length && text[after] == ':') ? Token_Api : Token_String);
                        i = end;
                    }
                    else if((c >= '0' && c <= '9') || ((c == '-' || c == '.') && i + 1 < length && text[i + 1] >= '0' && text[i + 1] <= '9' && (i == 0 || !isWord(text[i - 1]))))
                    {
                        u32 end = i + 1;
                        while(end < length && (isWord(text[end]) || text[end] == '.'))
                            end++;
                        push(i, end, Token_Number);
                        i = end;
                    }
                    else if(isWord(c) || (shader && c == '#'))
                    {
                        u32 end = i + 1;
                        while(end < length && isWord(text[end]))
                            end++;
                        u8 colour = Token_Text;
                        if(shader ? wordIs(i, end, kShaderWords, (u32)ArrayCount(kShaderWords)) : wordIs(i, end, kConfigWords, (u32)ArrayCount(kConfigWords)))
                            colour = Token_Keyword;
                        else if(!shader && end <= keyEnd)
                            colour = Token_Api;
                        push(i, end, colour);
                        i = end;
                    }
                    else
                    {
                        // spaces join the run before them
                        push(i, i + 1, (c == ' ' || c == '\t') && count > 0 ? runs[count - 1].Colour : (u8)Token_Text);
                        i++;
                    }
                }
                return count;
            }

            if(e->Language != UICodeLanguage_Lua)
            {
                push(0, length, Token_Text);
                return count;
            }

            u32 i = 0;
            if(state != Lex_Normal)
            {
                const u8 colour = state == Lex_BlockComment ? Token_Comment : Token_String;
                u32 end         = 0;
                while(end + 1 < length && !(text[end] == ']' && text[end + 1] == ']'))
                    end++;
                if(end + 1 >= length)
                {
                    push(0, length, colour);
                    return count;
                }
                push(0, end + 2, colour);
                i = end + 2;
            }

            while(i < length)
            {
                const char c = text[i];
                if(c == ' ')
                {
                    // rides along with whatever came before
                    u32 end = i;
                    while(end < length && text[end] == ' ')
                        end++;
                    push(i, end, count > 0 ? runs[count - 1].Colour : (u8)Token_Text);
                    i = end;
                }
                else if(c == '-' && i + 1 < length && text[i + 1] == '-')
                {
                    push(i, length, Token_Comment);
                    i = length;
                }
                else if(c == '"' || c == '\'')
                {
                    u32 end = i + 1;
                    while(end < length && text[end] != c)
                        end += text[end] == '\\' ? 2 : 1;
                    end = Maths::Min(end + 1, length);
                    push(i, end, Token_String);
                    i = end;
                }
                else if(c == '[' && i + 1 < length && text[i + 1] == '[')
                {
                    u32 end = i + 2;
                    while(end + 1 < length && !(text[end] == ']' && text[end + 1] == ']'))
                        end++;
                    end = end + 1 < length ? end + 2 : length;
                    push(i, end, Token_String);
                    i = end;
                }
                else if(c >= '0' && c <= '9')
                {
                    u32 end = i;
                    while(end < length && (IsIdentChar(text[end]) || text[end] == '.'))
                        end++;
                    push(i, end, Token_Number);
                    i = end;
                }
                else if(IsIdentChar(c))
                {
                    u32 end = i;
                    while(end < length && IsIdentChar(text[end]))
                        end++;
                    u8 colour = Token_Text;
                    if(IsLuaKeyword(text + i, end - i))
                        colour = Token_Keyword;
                    else
                    {
                        const String8 word = Str8((u8*)text + i, end - i);
                        if(LuaRegistry::IsType(word) || LuaRegistry::FindMember(String8 {}, word))
                            colour = Token_Api;
                    }
                    push(i, end, colour);
                    i = end;
                }
                else
                {
                    push(i, i + 1, Token_Text);
                    i++;
                }
            }
            return count;
        }

        const char* KindLabel(u8 kind)
        {
            switch(kind)
            {
            case Item_Keyword: return "key";
            case Item_Type: return "type";
            case Item_Function: return "fn";
            case Item_Field: return "field";
            default: return "word";
            }
        }
    }

    // ---- api ---------------------------------------------------------------

    UICodeEditor* UICodeEditorCreate()
    {
        Arena* arena         = ArenaAlloc(sizeof(UICodeEditor) + Kilobytes(1));
        UICodeEditor* editor = new(PushArrayNoZero(arena, UICodeEditor, 1)) UICodeEditor();
        editor->Memory       = arena;
        RebuildLines(editor);
        return editor;
    }

    void UICodeEditorDestroy(UICodeEditor* editor)
    {
        if(!editor)
            return;
        Arena* arena = editor->Memory;
        editor->~UICodeEditor();
        ArenaRelease(arena);
    }

    void UICodeEditorSetText(UICodeEditor* editor, const String8& text)
    {
        editor->Text.Clear();
        // tabs stay as they are; carriage returns go
        editor->Text.Reserve((size_t)text.size + 1);
        for(u64 i = 0; i < text.size; i++)
            if(text.str[i] != '\r')
                editor->Text.PushBack((char)text.str[i]);
        editor->Edits.Clear();
        editor->EditText.Clear();
        editor->EditIndex = 0;
        editor->Cursor = editor->Anchor = 0;
        editor->ScrollX = editor->ScrollY = 0.0f;
        editor->WantedX                   = -1.0f;
        editor->Dirty                     = false;
        editor->Changed                   = false;
        CloseCompletion(editor);
        RebuildLines(editor);
    }

    String8 UICodeEditorGetText(UICodeEditor* editor)
    {
        return Str8((u8*)editor->Text.Data(), TextSize(editor));
    }

    void UICodeEditorSetLanguage(UICodeEditor* editor, UICodeLanguage language)
    {
        editor->Language = language;
        RebuildLines(editor);
    }

    bool UICodeEditorIsDirty(UICodeEditor* editor) { return editor->Dirty; }
    void UICodeEditorMarkSaved(UICodeEditor* editor) { editor->Dirty = false; }

    bool UICodeEditorTakeSaveRequest(UICodeEditor* editor)
    {
        const bool asked  = editor->SaveAsked;
        editor->SaveAsked = false;
        return asked;
    }

    void UICodeEditorGetCursor(UICodeEditor* editor, u32* line, u32* column)
    {
        const u32 at = LineOf(editor, editor->Cursor);
        if(line)
            *line = at + 1;
        if(column)
            *column = editor->Cursor - LineStart(editor, at) + 1;
    }

    void UICodeEditorFocus(UICodeEditor* editor) { editor->WantFocus = true; }

    bool UICodeEditorDraw(const char* id, UICodeEditor* e, const Vec2& size)
    {
        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;
        const f32 fontSize       = floorf(theme.FontSize * dpi);
        const f32 textHeight     = Graphics::Font::GetMonoFont()->CalculateTextSize(Str8Lit("A"), fontSize).y;
        const f32 lineHeight     = floorf(Maths::Max(textHeight * 1.25f, fontSize * 1.35f));
        const f32 textPad        = floorf((lineHeight - textHeight) * 0.5f);
        const f32 gutter         = floorf(Width("00000", 5, fontSize) + 18.0f * dpi);
        const f32 pageLines      = Maths::Max(1.0f, floorf(size.y / lineHeight) - 1.0f);

        const f32 frameTime = Maths::Clamp(ui->CurrentTime - e->LastTime, 0.0f, 0.1f);
        e->LastTime         = ui->CurrentTime;

        u64 hash;
        HandleUIString(id, &hash);
        UI_Widget* box = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground, Str8Lit(""), hash,
                                    { SizeKind_Pixels, size.x }, { SizeKind_Pixels, size.y });
        box->Clip                              = true;
        box->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        box->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
        UIClearBorders(box);
        UISetBackground(box, theme.Canvas, theme.Canvas, theme.Canvas);
        e->Hash = box->hash;

        const UI_Interaction interaction = HandleWidgetInteraction(box);
        const Vec2 mouse                 = UIMousePosition() - box->position;
        const bool mouseHeld             = Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft);
        bool focused                     = UIIsKeySink(box->hash);

        if(e->WantFocus)
        {
            e->WantFocus = false;
            UISetKeySink(box->hash);
            focused = true;
        }

        auto offsetUnderMouse = [&]()
        {
            const i32 line = Maths::Clamp((i32)floorf((mouse.y + e->ScrollY) / lineHeight), 0, (i32)LineCount(e) - 1);
            return OffsetAtX(e, (u32)line, mouse.x - gutter + e->ScrollX, fontSize);
        };

        // ---- touch ----
        // A finger drags the text about; it does not select with a drag the way a mouse does.
        const bool touch = ui->ClickOnRelease;
        if(touch)
        {
            const Vec2 pointer = UIMousePosition();
            if(interaction.pressed)
            {
                if(!focused)
                    UISetKeySink(box->hash);
                focused = true;
                UISetKeyboardStyle(UIKeyboardStyle_Code);
                e->TouchDown     = true;
                e->TouchMoved    = false;
                e->TouchSelect   = false;
                e->TouchSideways = false;
                e->TouchDouble   = interaction.double_clicked;
                e->TouchStart    = pointer;
                e->TouchLast     = pointer;
                e->TouchScroll   = Vec2(e->ScrollX, e->ScrollY);
                e->Velocity      = Vec2(0.0f);
                CloseCompletion(e);
            }
            else if(e->TouchDown && mouseHeld)
            {
                const Vec2 moved = pointer - e->TouchStart;
                // held still: from here the drag selects
                if(!e->TouchMoved && !e->TouchSelect && UILongPressed())
                {
                    e->TouchSelect = true;
                    const u32 at   = offsetUnderMouse();
                    e->Anchor      = at;
                    while(e->Anchor > 0 && IsIdentChar(e->Text[e->Anchor - 1]))
                        e->Anchor--;
                    e->Cursor = at;
                    while(e->Cursor < TextSize(e) && IsIdentChar(e->Text[e->Cursor]))
                        e->Cursor++;
                }
                if(e->TouchSelect)
                {
                    if(Maths::Abs(moved.x) + Maths::Abs(moved.y) > 6.0f * dpi)
                        e->Cursor = offsetUnderMouse();
                    if(mouse.y < lineHeight)
                        e->ScrollY -= lineHeight * 0.5f;
                    else if(mouse.y > size.y - lineHeight)
                        e->ScrollY += lineHeight * 0.5f;
                }
                else
                {
                    if(!e->TouchMoved && (Maths::Abs(moved.x) > 8.0f * dpi || Maths::Abs(moved.y) > 8.0f * dpi))
                    {
                        e->TouchMoved    = true;
                        // one way at a time: mostly it is up and down
                        e->TouchSideways = Maths::Abs(moved.x) > Maths::Abs(moved.y) * 1.5f;
                    }
                    if(e->TouchMoved)
                    {
                        if(e->TouchSideways)
                            e->ScrollX = e->TouchScroll.x - moved.x;
                        else
                            e->ScrollY = e->TouchScroll.y - moved.y;
                        if(frameTime > 0.0001f)
                        {
                            const Vec2 speed = (e->TouchLast - pointer) / frameTime;
                            e->Velocity      = e->Velocity * 0.6f + (e->TouchSideways ? Vec2(speed.x, 0.0f) : Vec2(0.0f, speed.y)) * 0.4f;
                        }
                    }
                }
                e->TouchLast = pointer;
            }
            else if(e->TouchDown)
            {
                // lifted
                e->TouchDown = false;
                if(!e->TouchMoved && !e->TouchSelect)
                {
                    e->Velocity = Vec2(0.0f);
                    const u32 at = offsetUnderMouse();
                    if(e->TouchDouble)
                    {
                        e->Anchor = at;
                        while(e->Anchor > 0 && IsIdentChar(e->Text[e->Anchor - 1]))
                            e->Anchor--;
                        e->Cursor = at;
                        while(e->Cursor < TextSize(e) && IsIdentChar(e->Text[e->Cursor]))
                            e->Cursor++;
                    }
                    else
                        MoveTo(e, at, false);
                    e->Reveal = false;
                    UIShowKeyboard(true);
                }
                else if(e->TouchSelect)
                    e->Velocity = Vec2(0.0f);
            }
            else
            {
                // coasting
                if(Maths::Abs(e->Velocity.x) + Maths::Abs(e->Velocity.y) > 8.0f * dpi)
                {
                    e->ScrollX += e->Velocity.x * frameTime;
                    e->ScrollY += e->Velocity.y * frameTime;
                    e->Velocity = e->Velocity * expf(-3.5f * frameTime);
                }
                else
                    e->Velocity = Vec2(0.0f);

                if(focused && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft) && !interaction.hovering)
                {
                    UISetKeySink(0);
                    focused = false;
                    CloseCompletion(e);
                }
            }
        }
        // ---- mouse ----
        else if(interaction.double_clicked)
        {
            const u32 at = offsetUnderMouse();
            e->Anchor    = at;
            while(e->Anchor > 0 && IsIdentChar(e->Text[e->Anchor - 1]))
                e->Anchor--;
            e->Cursor = at;
            while(e->Cursor < TextSize(e) && IsIdentChar(e->Text[e->Cursor]))
                e->Cursor++;
            e->Selecting = false;
            CloseCompletion(e);
        }
        else if(interaction.pressed)
        {
            if(!focused)
                UISetKeySink(box->hash);
            focused          = true;
            UISetKeyboardStyle(UIKeyboardStyle_Code);
            const bool shift = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift);
            MoveTo(e, offsetUnderMouse(), shift);
            e->Reveal    = false;
            e->Selecting = true;
            CloseCompletion(e);
        }
        else if(e->Selecting)
        {
            if(mouseHeld)
            {
                e->Cursor = offsetUnderMouse();
                // dragging past the top or bottom walks the view
                if(mouse.y < 0.0f)
                    e->ScrollY -= lineHeight;
                else if(mouse.y > size.y)
                    e->ScrollY += lineHeight;
            }
            else
                e->Selecting = false;
        }
        else if(focused && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft) && !interaction.hovering)
        {
            // a press anywhere else lets go of the keyboard
            UISetKeySink(0);
            focused = false;
            CloseCompletion(e);
        }

        if(interaction.hovering)
        {
            const f32 wheel = Input::Get().GetScrollOffset();
            if(wheel != 0.0f)
            {
                e->ScrollY -= wheel * lineHeight * 3.0f;
                CloseCompletion(e);
            }
        }

        // a tap (not a drag that scrolls or selects) brings the on-screen keyboard up - again too, after it was put away
        if(!touch && focused && interaction.clicked && !interaction.double_clicked)
            UIShowKeyboard(true);

        // ---- keys ----
        if(focused)
        {
            UIKeyEvent events[64];
            const u32 count = UITakeKeyEvents(box->hash, events, 64);
            for(u32 i = 0; i < count; i++)
                HandleKey(e, events[i], fontSize, pageLines);
            if(count > 0)
                e->CaretTime = 0.0f;
        }
        e->Cursor = Maths::Min(e->Cursor, TextSize(e));
        e->Anchor = Maths::Min(e->Anchor, TextSize(e));

        // ---- scroll ----
        const f32 contentHeight = (f32)LineCount(e) * lineHeight;
        const f32 textWidth     = Maths::Max(0.0f, size.x - gutter - 10.0f * dpi);
        if(e->Reveal)
        {
            e->Reveal        = false;
            const f32 y      = (f32)LineOf(e, e->Cursor) * lineHeight;
            const f32 x      = XOfOffset(e, e->Cursor, fontSize);
            const f32 margin = lineHeight;
            if(y < e->ScrollY + margin)
                e->ScrollY = y - margin;
            if(y + lineHeight > e->ScrollY + size.y - margin)
                e->ScrollY = y + lineHeight - size.y + margin;
            if(x < e->ScrollX)
                e->ScrollX = Maths::Max(0.0f, x - 40.0f * dpi);
            if(x > e->ScrollX + textWidth - 20.0f * dpi)
                e->ScrollX = x - textWidth + 60.0f * dpi;
        }
        // room to scroll the last line up off the bottom edge
        e->ScrollY = Maths::Clamp(e->ScrollY, 0.0f, Maths::Max(0.0f, contentHeight - size.y + lineHeight * 3.0f));
        e->ScrollX = Maths::Max(0.0f, e->ScrollX);

        const u32 firstLine  = (u32)Maths::Clamp((i32)floorf(e->ScrollY / lineHeight), 0, (i32)LineCount(e) - 1);
        const u32 lastLine   = Maths::Min(LineCount(e), firstLine + (u32)ceilf(size.y / lineHeight) + 1);
        const u32 cursorLine = LineOf(e, e->Cursor);

        Vec4 colours[Token_Count];
        colours[Token_Text]    = theme.Text;
        colours[Token_Keyword] = theme.Accent;
        colours[Token_String]  = theme.Success;
        colours[Token_Number]  = theme.Warning;
        colours[Token_Comment] = Vec4(theme.TextDim.x, theme.TextDim.y, theme.TextDim.z, 1.0f) * 0.85f;
        colours[Token_Comment].w = 1.0f;
        colours[Token_Api]       = Vec4((theme.Text.x + theme.Accent.x) * 0.5f, (theme.Text.y + theme.Accent.y) * 0.5f + 0.08f, (theme.Text.z + theme.Accent.z) * 0.5f + 0.12f, 1.0f);

        PushParent(box);
        {
            // the line being edited
            if(focused)
                Quad(UIHashCombine(box->hash, 0x4355524C494Eull), Vec2(0.0f, (f32)cursorLine * lineHeight - e->ScrollY), Vec2(size.x, lineHeight), theme.Hover);

            // selection, a strip per line
            if(HasSelection(e))
            {
                Vec4 selection = theme.Accent;
                selection.w    = 0.30f;
                const u32 from = SelectionStart(e), to = SelectionEnd(e);
                for(u32 line = Maths::Max(firstLine, LineOf(e, from)); line < lastLine && LineStart(e, line) <= to; line++)
                {
                    const u32 a = Maths::Max(from, LineStart(e, line));
                    const u32 b = Maths::Min(to, LineEnd(e, line));
                    if(b < a)
                        continue;
                    const f32 x0 = XOfOffset(e, a, fontSize);
                    f32 x1       = XOfOffset(e, b, fontSize);
                    if(to > LineEnd(e, line))
                        x1 += fontSize * 0.5f; // the line end is selected too
                    Quad(UIHashCombine(box->hash, 0x53454C00ull + line), Vec2(gutter + x0 - e->ScrollX, (f32)line * lineHeight - e->ScrollY),
                         Vec2(Maths::Max(x1 - x0, 2.0f * dpi), lineHeight), selection, 2.0f * dpi);
                }
            }

            // text
            char display[kMaxLineBytes * 2];
            for(u32 line = firstLine; line < lastLine; line++)
            {
                const f32 y      = (f32)line * lineHeight - e->ScrollY + textPad;
                const u32 length = DisplayLine(e, line, display, sizeof(display));

                Run runs[48];
                const u32 runCount = LexLine(e, display, length, e->LineState[line], runs, 48);
                for(u32 r = 0; r < runCount; r++)
                {
                    const f32 x = gutter + Width(display, runs[r].Start, fontSize) - e->ScrollX;
                    if(x > size.x)
                        break;
                    TextAt(UIHashCombine(box->hash, ((u64)line << 8) | r), Vec2(floorf(x), y), display + runs[r].Start, runs[r].Length, colours[runs[r].Colour], fontSize);
                }
            }

            // gutter over the text, so a line scrolled sideways slides under it
            Quad(UIHashCombine(box->hash, 0x475554ull), Vec2(0.0f, 0.0f), Vec2(gutter - 6.0f * dpi, size.y), theme.Canvas);
            for(u32 line = firstLine; line < lastLine; line++)
            {
                char number[16];
                const int digits = snprintf(number, sizeof(number), "%u", line + 1);
                const f32 width  = Width(number, (u32)digits, fontSize);
                Vec4 colour      = line == cursorLine ? theme.Text : theme.TextDim;
                colour.w         = line == cursorLine ? 1.0f : 0.7f;
                TextAt(UIHashCombine(box->hash, 0x4E554D0000ull + line), Vec2(floorf(gutter - 14.0f * dpi - width), (f32)line * lineHeight - e->ScrollY + textPad),
                       number, (u32)digits, colour, fontSize);
            }

            // caret
            if(focused)
            {
                e->CaretTime += frameTime;
                const f32 pulse = e->CaretTime < 0.5f ? 1.0f : 0.55f + 0.45f * cosf((e->CaretTime - 0.5f) * 5.0f);
                Vec4 caret      = theme.Accent;
                caret.w         = pulse;
                const f32 x     = gutter + XOfOffset(e, e->Cursor, fontSize) - e->ScrollX;
                if(x >= gutter - 2.0f * dpi)
                    Quad(UIHashCombine(box->hash, 0x4341524554ull), Vec2(floorf(x), (f32)cursorLine * lineHeight - e->ScrollY + 2.0f * dpi),
                         Vec2(Maths::Max(1.0f, floorf(1.5f * dpi)), lineHeight - 4.0f * dpi), caret);
            }

            // where the view is in the file
            if(contentHeight > size.y)
            {
                const f32 track = size.y - 4.0f * dpi;
                const f32 thumb = Maths::Max(24.0f * dpi, track * size.y / (contentHeight + lineHeight * 3.0f));
                const f32 at    = (track - thumb) * e->ScrollY / Maths::Max(1.0f, contentHeight - size.y + lineHeight * 3.0f);
                Vec4 colour     = theme.TextDim;
                colour.w        = 0.35f;
                Quad(UIHashCombine(box->hash, 0x5448554D42ull), Vec2(size.x - 6.0f * dpi, 2.0f * dpi + at), Vec2(4.0f * dpi, thumb), colour, 2.0f * dpi);
            }
        }
        PopParent(box);

        // ---- completion list, over everything ----
        if(e->CompleteOpen && focused && e->ItemCount > 0)
        {
            const u32 shown    = Maths::Min(e->ItemCount, kMaxShownItems);
            const u32 first    = (u32)Maths::Clamp(e->CompleteSelected - (i32)shown + 1, 0, (i32)(e->ItemCount - shown));
            const f32 rowH     = lineHeight + 4.0f * dpi;
            const f32 width    = 360.0f * dpi;
            const f32 height   = (f32)shown * rowH + 8.0f * dpi;
            const Vec2 screen  = ui->root_parent.size;
            Vec2 position      = box->position + Vec2(gutter + XOfOffset(e, e->CompleteStart, fontSize) - e->ScrollX, (f32)(cursorLine + 1) * lineHeight - e->ScrollY + 2.0f * dpi);
            if(position.y + height > screen.y - 4.0f * dpi)
                position.y -= height + lineHeight + 4.0f * dpi;
            position.x = Maths::Clamp(position.x, 4.0f * dpi, Maths::Max(4.0f * dpi, screen.x - width - 4.0f * dpi));

            ui->parents.PushBack(&ui->root_parent);
            UI_Widget* list = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost | WidgetFlags_StackVertically,
                                         Str8Lit(""), UIHashCombine(box->hash, 0x434F4D504C455445ull), { SizeKind_Pixels, width }, { SizeKind_Pixels, height });
            list->relative_position                 = Vec2(floorf(position.x), floorf(position.y)) - ui->root_parent.position;
            list->style_vars[StyleVar_Padding]      = Vec4(4.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
            list->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f);
            list->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi + 2.0f * dpi, 0.0f, 0.0f, 0.0f);
            list->style_vars[StyleVar_Border]       = Vec4(1.0f, 1.0f, 0.0f, 0.0f);
            // opaque: the border is a quad behind the fill
            const Vec4 fill   = Vec4(theme.Panel.x, theme.Panel.y, theme.Panel.z, 1.0f);
            const Vec4 border = Vec4(theme.Border.x, theme.Border.y, theme.Border.z, 1.0f);
            UISetBackground(list, fill, fill, fill);
            list->style_vars[StyleVar_BorderColor]       = border;
            list->style_vars[StyleVar_HotBorderColor]    = border;
            list->style_vars[StyleVar_ActiveBorderColor] = border;

            PushParent(list);
            for(u32 i = first; i < first + shown; i++)
            {
                const CompleteItem& item = e->Items[i];
                const bool selected      = (i32)i == e->CompleteSelected;
                UI_Widget* row           = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""),
                                                      UIHashCombine(list->hash, i + 1), { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, rowH });
                row->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing]  = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi, 0.0f, 0.0f, 0.0f);
                UIClearBorders(row);
                const Vec4 rowFill = selected ? theme.Selection : Vec4(0.0f);
                UISetBackground(row, rowFill, rowFill, rowFill);

                PushParent(row);
                UIPushStyle(StyleVar_FontSize, fontSize);
                UIPushStyle(StyleVar_TextColor, theme.Text);
                UILabel("###name", Str8C((char*)item.Text));
                UIPopStyle(StyleVar_TextColor);
                UIFlexSpacer();
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelEllipsis("###detail", Str8C((char*)(item.Detail[0] ? item.Detail : KindLabel(item.Kind))), 190.0f);
                UIPopStyle(StyleVar_TextColor);
                UIPopStyle(StyleVar_FontSize);
                PopParent(row);
            }
            PopParent(list);
            ui->parents.PopBack();
        }

        const bool changed = e->Changed;
        e->Changed         = false;
        return changed;
    }
}
