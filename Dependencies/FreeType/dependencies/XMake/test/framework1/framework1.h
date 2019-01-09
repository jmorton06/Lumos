#ifndef _FRAMEWORK1_
#define _FRAMEWORK1_

#include <dynamic_lib2.h>
#include <static_lib1.h>

#if !defined(FRAMEWORK1_IMPORT) && !defined(FRAMEWORK1_INTERNAL)
#error See compile declarations -- !defined(FRAMEWORK1_IMPORT) && !defined(FRAMEWORK1_INTERNAL)
#endif

#if defined(FRAMEWORK1_IMPORT) && defined(FRAMEWORK1_INTERNAL)
#error See compile declarations -- defined(FRAMEWORK1_IMPORT) && defined(FRAMEWORK1_INTERNAL)
#endif


namespace framework1
{
    void test();
}

#endif
