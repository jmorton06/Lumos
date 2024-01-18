#pragma once
#include "Core/String.h"

namespace Lumos
{
    struct CommandLineOptionNode
    {
        CommandLineOptionNode* next;
        String8 name;
        String8List values;
        String8 value;
    };

    struct CommandLineOptionSlot
    {
        CommandLineOptionNode* first;
        CommandLineOptionNode* last;
    };

    class CommandLine
    {
    public: 

        CommandLine() = default;
        ~CommandLine() = default;

        void Init(Arena* arena, String8List strings);
        String8List OptionStrings(String8 name);
        String8 OptionString(String8 name);
        bool OptionBool(String8 name);
        double OptionDouble(String8 name);
        int64_t OptionInt64(String8 name);
    private:
        uint64_t m_SlotCount;
        CommandLineOptionSlot* m_Slots;
        String8List m_Inputs;
    };


}
