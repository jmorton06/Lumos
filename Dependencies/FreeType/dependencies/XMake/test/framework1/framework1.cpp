#include "framework1.h"


#ifdef FRAMEWORK1_IMPORT
#error See compile declarations -- FRAMEWORK1_IMPORT
#endif


#ifndef FRAMEWORK1_INTERNAL
#error See compile declarations -- FRAMEWORK1_INTERNAL
#endif


#ifndef DYNAMIC_LIB2_IMPORT
#error See compile declarations -- DYNAMIC_LIB2_IMPORT
#endif

#ifndef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


namespace framework1
{
    void test()
    {
        dynamic_lib2::test();
        static_lib1::test();
    }
}
