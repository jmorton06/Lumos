#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "CommandLine.h"
#include "Maths/MathsUtilities.h"
#include "Core/Thread.h"
#include "Utilities/StringUtilities.h"

namespace Lumos
{
    void CommandLine::Init(Arena* arena, String8List strings)
    {
        ArenaTemp scratch = ScratchBegin(&arena, 1);

        m_SlotCount = 64;
        m_Slots     = PushArray(arena, CommandLineOptionSlot, m_SlotCount);

        String8List separatedStrings = { 0 };
        for(String8Node* n = strings.first; n != 0; n = n->next)
        {
            String8List stringsFromCurrentNode = { 0 };
            uint64_t startIndex                = 0;
            bool quoted                        = false;
            bool seekingNonWS                  = false;

            for(uint64_t index = 0; index <= n->string.size; index += 1)
            {
                if(seekingNonWS && index < n->string.size && !CharIsSpace(n->string.str[index]))
                {
                    seekingNonWS = false;
                    startIndex   = index;
                }
                if(!seekingNonWS && (index == n->string.size || n->string.str[index] == ' ' || n->string.str[index] == '"'))
                {
                    String8 string = Substr8(n->string, RangeU64({ startIndex, index }));
                    Str8ListPush(scratch.arena, &stringsFromCurrentNode, string);
                    startIndex = index + 1;
                    if(n->string.str[index] == ' ')
                    {
                        seekingNonWS = true;
                    }
                }
                if(index < n->string.size && n->string.str[index] == '"')
                {
                    quoted ^= 1;
                }
            }
            Str8ListConcatInPlace(&separatedStrings, &stringsFromCurrentNode);
        }

        CommandLineOptionNode* activeOptionNode = nullptr;
        for(String8Node* n = separatedStrings.first; n != 0; n = n->next)
        {
            String8 piece       = StringUtilities::Str8SkipChopWhitespace(n->string);
            bool doubleDash     = Str8Match(Prefix8(piece, 2), Str8Lit("--"), MatchFlags(0));
            bool singleDash     = Str8Match(Prefix8(piece, 1), Str8Lit("-"), MatchFlags(0));
            bool valueForOption = (activeOptionNode != nullptr);

            if(!valueForOption && (doubleDash || singleDash))
            {
                uint64_t dashSize           = doubleDash ? 2 : 1;
                String8 optionPart          = Str8Skip(piece, dashSize);
                uint64_t colonPos           = FindSubstr8(optionPart, Str8Lit(":"), 0);
                uint64_t equalPos           = FindSubstr8(optionPart, Str8Lit("="), 0);
                uint64_t valueSpecifierPos  = Maths::Min(colonPos, equalPos);
                String8 optionName          = Prefix8(optionPart, valueSpecifierPos);
                String8 firstPartOfOptValue = Str8Skip(optionPart, valueSpecifierPos + 1);
                uint64_t hash               = StringUtilities::BasicHashFromString(optionName);
                uint64_t slot_idx           = hash % m_SlotCount;

                CommandLineOptionSlot* slot = &m_Slots[slot_idx];
                CommandLineOptionNode* node = PushArray(arena, CommandLineOptionNode, 1);
                QueuePush(slot->first, slot->last, node);
                node->name = optionName;
                if(firstPartOfOptValue.size != 0)
                {
                    Str8ListPush(arena, &node->values, firstPartOfOptValue);
                }
                if(valueSpecifierPos < optionPart.size && (firstPartOfOptValue.size == 0 || Str8Match(Suffix8(firstPartOfOptValue, 1), Str8Lit(","), MatchFlags(0))))
                {
                    activeOptionNode = node;
                }
            }
            else if(valueForOption)
            {
                String8 splits[]        = { Str8Lit(",") };
                String8List value_parts = StrSplit8(arena, piece, ArrayCount(splits), splits);
                Str8ListConcatInPlace(&activeOptionNode->values, &value_parts);
                if(!Str8Match(Suffix8(piece, 1), Str8Lit(","), MatchFlags(0)))
                {
                    activeOptionNode = nullptr;
                }
            }

            {
                Str8ListPush(arena, &m_Inputs, piece);
            }
        }

        {
            for(uint64_t index = 0; index < m_SlotCount; index += 1)
            {
                for(CommandLineOptionNode* n = m_Slots[index].first; n != 0; n = n->next)
                {
                    StringJoin join = { Str8Lit(""), Str8Lit(","), Str8Lit("") };
                    n->value        = Str8ListJoin(arena, n->values, &join);
                }
            }
        }

        ScratchEnd(scratch);
    }

    String8List CommandLine::OptionStrings(String8 name)
    {
        String8List result = { 0 };
        {
            uint64_t hash               = StringUtilities::BasicHashFromString(name);
            uint64_t slot_idx           = hash % m_SlotCount;
            CommandLineOptionSlot* slot = &m_Slots[slot_idx];
            CommandLineOptionNode* node = nullptr;
            for(CommandLineOptionNode* n = slot->first; n != 0; n = n->next)
            {
                if(Str8Match(n->name, name, MatchFlags(0)))
                {
                    node = n;
                    break;
                }
            }
            if(node != nullptr)
            {
                result = node->values;
            }
        }
        return result;
    }

    String8 CommandLine::OptionString(String8 name)
    {
        String8 result = { 0 };
        {
            uint64_t hash               = StringUtilities::BasicHashFromString(name);
            uint64_t slot_idx           = hash % m_SlotCount;
            CommandLineOptionSlot* slot = &m_Slots[slot_idx];
            CommandLineOptionNode* node = nullptr;
            for(CommandLineOptionNode* n = slot->first; n != 0; n = n->next)
            {
                if(Str8Match(n->name, name, MatchFlags(0)))
                {
                    node = n;
                    break;
                }
            }
            if(node != nullptr)
            {
                result = node->value;
            }
        }
        return result;
    }

    bool CommandLine::OptionBool(String8 name)
    {
        bool result = 0;
        {
            uint64_t hash               = StringUtilities::BasicHashFromString(name);
            uint64_t slot_idx           = hash % m_SlotCount;
            CommandLineOptionSlot* slot = &m_Slots[slot_idx];
            CommandLineOptionNode* node = nullptr;
            for(CommandLineOptionNode* n = slot->first; n != 0; n = n->next)
            {
                if(Str8Match(n->name, name, MatchFlags(0)))
                {
                    node = n;
                    break;
                }
            }
            if(node != 0)
            {
                result = (node->value.size == 0 || Str8Match(node->value, Str8Lit("true"), MatchFlags::CaseInsensitive) || Str8Match(node->value, Str8Lit("1"), MatchFlags::CaseInsensitive));
            }
        }
        return result;
    }

    double CommandLine::OptionDouble(String8 name)
    {
        double result = 0;
        {
            String8 string = OptionString(name);
            result         = DoubleFromStr8(string);
        }
        return result;
    }

    int64_t CommandLine::OptionInt64(String8 name)
    {
        int64_t result = 0;
        {
            String8 string = OptionString(name);
            result         = IntFromStr8(string);
        }
        return result;
    }

}
