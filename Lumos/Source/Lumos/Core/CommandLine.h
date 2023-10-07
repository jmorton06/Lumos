#pragma once
#include "Core/String.h"

namespace Lumos
{
    struct CommandLineOptNode
    {
        CommandLineOptNode* next;
        String8 name;
        String8List values;
        String8 value;
    };

    struct CommandLineOptSlot
    {
        CommandLineOptNode* first;
        CommandLineOptNode* last;
    };

    struct CommandLine
    {
        uint64_t slots_count;
        CommandLineOptSlot* slots;
        String8List inputs;
    };

    uint64_t CommandLineHashFromString(String8 string);
    CommandLine CommandLineFromStringList(Arena* arena, String8List strings);
    String8List CommandLineOptStrings(CommandLine* cmdln, String8 name);
    String8 CommandLineOptString(CommandLine* cmdln, String8 name);
    bool CommandLineOptBool(CommandLine* cmdln, String8 name);
    double CommandLineOptDouble(CommandLine* cmdln, String8 name);
    int64_t CommandLineOptInt64(CommandLine* cmdln, String8 name);
}
