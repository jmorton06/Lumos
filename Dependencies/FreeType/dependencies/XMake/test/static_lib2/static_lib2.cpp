#include "static_lib2.h"


#ifdef STATIC_LIB2_IMPORT
#error See compile declarations -- STATIC_LIB2_IMPORT
#endif


#ifndef STATIC_LIB2_INTERNAL
#error See compile declarations -- STATIC_LIB2_INTERNAL
#endif


#ifndef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


namespace static_lib2
{
    void test()
    {
        static_lib1::test();
    }
}
