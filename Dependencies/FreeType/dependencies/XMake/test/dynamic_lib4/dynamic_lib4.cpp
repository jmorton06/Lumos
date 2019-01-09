#include "dynamic_lib4.h"


#ifdef DYNAMIC_LIB4_IMPORT
#error See compile declarations -- DYNAMIC_LIB4_IMPORT
#endif


#ifndef DYNAMIC_LIB4_INTERNAL
#error See compile declarations -- DYNAMIC_LIB4_INTERNAL
#endif


#ifndef DYNAMIC_LIB2_IMPORT
#error See compile declarations -- DYNAMIC_LIB2_IMPORT
#endif

#ifndef DYNAMIC_LIB1_IMPORT
#error See compile declarations -- DYNAMIC_LIB1_IMPORT
#endif


namespace dynamic_lib4
{
    void test()
    {
        dynamic_lib2::test();
        dynamic_lib1::test();
        static_lib1::test();
    }
}
