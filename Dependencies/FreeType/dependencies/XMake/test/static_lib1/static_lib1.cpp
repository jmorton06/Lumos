#include "static_lib1.h"


#ifdef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


#ifndef STATIC_LIB1_INTERNAL
#error See compile declarations -- STATIC_LIB1_INTERNAL
#endif


namespace static_lib1
{
    void test()
    {
    }
}
