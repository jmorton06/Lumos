#include "dynamic_lib3.h"


#ifdef DYNAMIC_LIB3_IMPORT
#error See compile declarations -- DYNAMIC_LIB3_IMPORT
#endif


#ifndef DYNAMIC_LIB3_INTERNAL
#error See compile declarations -- DYNAMIC_LIB3_INTERNAL
#endif


#ifndef DYNAMIC_LIB2_IMPORT
#error See compile declarations -- DYNAMIC_LIB2_IMPORT
#endif


namespace dynamic_lib3
{
    void test()
    {
        dynamic_lib2::test();
    }
}
