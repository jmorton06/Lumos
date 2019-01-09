#ifndef _STATIC_LIB2_
#define _STATIC_LIB2_

#include <static_lib1.h>

#if !defined(STATIC_LIB2_IMPORT) && !defined(STATIC_LIB2_INTERNAL)
#error See compile declarations -- !defined(STATIC_LIB2_IMPORT) && !defined(STATIC_LIB2_INTERNAL)
#endif

#if defined(STATIC_LIB2_IMPORT) && defined(STATIC_LIB2_INTERNAL)
#error See compile declarations -- defined(STATIC_LIB2_IMPORT) && defined(STATIC_LIB2_INTERNAL)
#endif


namespace static_lib2
{
    void test();
}

#endif
