#ifndef _DYNAMIC_LIB4_
#define _DYNAMIC_LIB4_

#include <dynamic_lib2.h>
#include <dynamic_lib1.h>

#if !defined(DYNAMIC_LIB4_IMPORT) && !defined(DYNAMIC_LIB4_INTERNAL)
#error See compile declarations -- !defined(DYNAMIC_LIB4_IMPORT) && !defined(DYNAMIC_LIB4_INTERNAL)
#endif

#if defined(DYNAMIC_LIB4_IMPORT) && defined(DYNAMIC_LIB4_INTERNAL)
#error See compile declarations -- defined(DYNAMIC_LIB4_IMPORT) && defined(DYNAMIC_LIB4_INTERNAL)
#endif


#if defined( __WIN32__ ) || defined( _WIN32 )
    #ifdef DYNAMIC_LIB4_IMPORT
        #define DYNAMIC_LIB4_SYMBOL __declspec(dllimport)
    #else
        #define DYNAMIC_LIB4_SYMBOL __declspec(dllexport)
    #endif
#else
    #define DYNAMIC_LIB4_SYMBOL
#endif


namespace dynamic_lib4
{
    void DYNAMIC_LIB4_SYMBOL test();
}

#endif
