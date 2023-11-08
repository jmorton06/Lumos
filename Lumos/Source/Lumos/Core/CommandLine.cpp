#include "Precompiled.h"
#include "CommandLine.h"
#include "Maths/MathsUtilities.h"
#include "Core/Thread.h"

namespace Lumos
{
    uint64_t CommandLineHashFromString(String8 string)
    {
        uint64_t result = 5381;
        for(uint64_t i = 0; i < string.size; i += 1)
        {
            result = ((result << 5) + result) + string.str[i];
        }
        return result;
    }

    CommandLine CommandLineFromStringList(Arena* arena, String8List strings)
    {
        ArenaTemp scratch = ScratchBegin(&arena, 1);

        CommandLine cmdln = { 0 };
        cmdln.slots_count = 64;
        cmdln.slots       = PushArray(arena, CommandLineOptSlot, cmdln.slots_count);

        String8List separated_strings = { 0 };
        for(String8Node* n = strings.first; n != 0; n = n->next)
        {
            String8List strings_from_this_n = { 0 };
            uint64_t start_idx              = 0;
            bool quoted                     = 0;
            bool seeking_non_ws             = 0;

            for(uint64_t idx = 0; idx <= n->string.size; idx += 1)
            {
                if(seeking_non_ws && idx < n->string.size && !CharIsSpace(n->string.str[idx]))
                {
                    seeking_non_ws = 0;
                    start_idx      = idx;
                }
                if(!seeking_non_ws && (idx == n->string.size || n->string.str[idx] == ' ' || n->string.str[idx] == '"'))
                {
                    String8 string = Substr8(n->string, Range1U64({ start_idx, idx }));
                    Str8ListPush(scratch.arena, &strings_from_this_n, string);
                    start_idx = idx + 1;
                    if(n->string.str[idx] == ' ')
                    {
                        seeking_non_ws = 1;
                    }
                }
                if(idx < n->string.size && n->string.str[idx] == '"')
                {
                    quoted ^= 1;
                }
            }
            Str8ListConcatInPlace(&separated_strings, &strings_from_this_n);
        }

        CommandLineOptNode* active_opt_node = 0;
        for(String8Node* n = separated_strings.first; n != 0; n = n->next)
        {
            String8 piece      = Str8SkipChopWhitespace(n->string);
            bool double_dash   = Str8Match(Prefix8(piece, 2), Str8Lit("--"), 0);
            bool single_dash   = Str8Match(Prefix8(piece, 1), Str8Lit("-"), 0);
            bool value_for_opt = (active_opt_node != 0);

            if(value_for_opt == 0 && (double_dash || single_dash))
            {
                uint64_t dash_prefix_size       = !!single_dash + !!double_dash;
                String8 opt_part                = Str8Skip(piece, dash_prefix_size);
                uint64_t colon_pos              = FindSubstr8(opt_part, Str8Lit(":"), 0, 0);
                uint64_t equal_pos              = FindSubstr8(opt_part, Str8Lit("="), 0, 0);
                uint64_t value_specifier_pos    = Maths::Min(colon_pos, equal_pos);
                String8 opt_name                = Prefix8(opt_part, value_specifier_pos);
                String8 first_part_of_opt_value = Str8Skip(opt_part, value_specifier_pos + 1);
                uint64_t hash                   = CommandLineHashFromString(opt_name);
                uint64_t slot_idx               = hash % cmdln.slots_count;
                CommandLineOptSlot* slot        = &cmdln.slots[slot_idx];
                CommandLineOptNode* node        = PushArray(arena, CommandLineOptNode, 1);
                QueuePush(slot->first, slot->last, node);
                node->name = opt_name;
                if(first_part_of_opt_value.size != 0)
                {
                    Str8ListPush(arena, &node->values, first_part_of_opt_value);
                }
                if(value_specifier_pos < opt_part.size && (first_part_of_opt_value.size == 0 || Str8Match(Suffix8(first_part_of_opt_value, 1), Str8Lit(","), 0)))
                {
                    active_opt_node = node;
                }
            }
            else if(value_for_opt)
            {
                String8 splits[]        = { Str8Lit(",") };
                String8List value_parts = StrSplit8(arena, piece, ArrayCount(splits), splits);
                Str8ListConcatInPlace(&active_opt_node->values, &value_parts);
                if(!Str8Match(Suffix8(piece, 1), Str8Lit(","), 0))
                {
                    active_opt_node = 0;
                }
            }

            {
                Str8ListPush(arena, &cmdln.inputs, piece);
            }
        }

        {
            for(uint64_t slot_idx = 0; slot_idx < cmdln.slots_count; slot_idx += 1)
            {
                for(CommandLineOptNode* n = cmdln.slots[slot_idx].first; n != 0; n = n->next)
                {
                    StringJoin join = { Str8Lit(""), Str8Lit(","), Str8Lit("") };
                    n->value        = Str8ListJoin(arena, n->values, &join);
                }
            }
        }

        ScratchEnd(scratch);
        return cmdln;
    }

    String8List CommandLineOptStrings(CommandLine* cmdln, String8 name)
    {
        String8List result = { 0 };
        {
            uint64_t hash            = CommandLineHashFromString(name);
            uint64_t slot_idx        = hash % cmdln->slots_count;
            CommandLineOptSlot* slot = &cmdln->slots[slot_idx];
            CommandLineOptNode* node = 0;
            for(CommandLineOptNode* n = slot->first; n != 0; n = n->next)
            {
                if(Str8Match(n->name, name, 0))
                {
                    node = n;
                    break;
                }
            }
            if(node != 0)
            {
                result = node->values;
            }
        }
        return result;
    }

    String8 CommandLineOptString(CommandLine* cmdln, String8 name)
    {
        String8 result = { 0 };
        {
            uint64_t hash            = CommandLineHashFromString(name);
            uint64_t slot_idx        = hash % cmdln->slots_count;
            CommandLineOptSlot* slot = &cmdln->slots[slot_idx];
            CommandLineOptNode* node = 0;
            for(CommandLineOptNode* n = slot->first; n != 0; n = n->next)
            {
                if(Str8Match(n->name, name, 0))
                {
                    node = n;
                    break;
                }
            }
            if(node != 0)
            {
                result = node->value;
            }
        }
        return result;
    }

    bool CommandLineOptBool(CommandLine* cmdln, String8 name)
    {
        bool result = 0;
        {
            uint64_t hash            = CommandLineHashFromString(name);
            uint64_t slot_idx        = hash % cmdln->slots_count;
            CommandLineOptSlot* slot = &cmdln->slots[slot_idx];
            CommandLineOptNode* node = 0;
            for(CommandLineOptNode* n = slot->first; n != 0; n = n->next)
            {
                if(Str8Match(n->name, name, 0))
                {
                    node = n;
                    break;
                }
            }
            if(node != 0)
            {
                result = (node->value.size == 0 || Str8Match(node->value, Str8Lit("true"), MatchFlag_CaseInsensitive) || Str8Match(node->value, Str8Lit("1"), MatchFlag_CaseInsensitive));
            }
        }
        return result;
    }

    double CommandLineOptDouble(CommandLine* cmdln, String8 name)
    {
        double result = 0;
        {
            String8 string = CommandLineOptString(cmdln, name);
            result         = DoubleFromStr8(string);
        }
        return result;
    }

    int64_t CommandLineOptInt64(CommandLine* cmdln, String8 name)
    {
        int64_t result = 0;
        {
            String8 string = CommandLineOptString(cmdln, name);
            result         = CStyleIntFromStr8(string);
        }
        return result;
    }

}
