#pragma once
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    class StringPool
    {
    public:
        StringPool(Arena* arena, u32 MaxStringSize = 260)
            : m_Arena(arena)
            , m_MaxStringSize(MaxStringSize)
        {
        }

        ~StringPool()
        {
        }

        String8 Allocate(const char* text)
        {
            String8 AllocatedString;
            ;
            if(FreeList.Empty())
            {
                AllocatedString = PushStr8FillByte(m_Arena, m_MaxStringSize, 0);
            }
            else
            {
                AllocatedString = FreeList.Back();
            }

            String8 string = Str8F(AllocatedString, text);
            FreeList.PopBack();

            return string;
        }

        void Deallocate(String8& string)
        {
            FreeList.PushBack(string);
        }

    private:
        Arena* m_Arena;
        u32 m_MaxStringSize;

        TDArray<String8> FreeList;
    };

}
