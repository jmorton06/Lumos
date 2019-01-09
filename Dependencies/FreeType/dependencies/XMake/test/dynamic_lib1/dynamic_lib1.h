#ifndef _DYNAMIC_LIB1_
#define _DYNAMIC_LIB1_

#include <static_lib1.h>

#if !defined(DYNAMIC_LIB1_IMPORT) && !defined(DYNAMIC_LIB1_INTERNAL)
#error See compile declarations -- !defined(DYNAMIC_LIB1_IMPORT) && !defined(DYNAMIC_LIB1_INTERNAL)
#endif

#if defined(DYNAMIC_LIB1_IMPORT) && defined(DYNAMIC_LIB1_INTERNAL)
#error See compile declarations -- defined(DYNAMIC_LIB1_IMPORT) && defined(DYNAMIC_LIB1_INTERNAL)
#endif


#if defined( __WIN32__ ) || defined( _WIN32 )
    #ifdef DYNAMIC_LIB1_IMPORT
        #define DYNAMIC_LIB1_SYMBOL __declspec(dllimport)
    #else
        #define DYNAMIC_LIB1_SYMBOL __declspec(dllexport)
    #endif
#else
    #define DYNAMIC_LIB1_SYMBOL
#endif


namespace dynamic_lib1
{
    void DYNAMIC_LIB1_SYMBOL test();
}

#endif
