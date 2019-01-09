#include "dynamic_lib1.h"


#ifdef DYNAMIC_LIB1_IMPORT
#error See compile declarations -- DYNAMIC_LIB1_IMPORT
#endif


#ifndef DYNAMIC_LIB1_INTERNAL
#error See compile declarations -- DYNAMIC_LIB1_INTERNAL
#endif


namespace dynamic_lib1
{
    void test()
    {
        static_lib1::test();
    }
}
