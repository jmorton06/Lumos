#ifndef _DYNAMIC_LIB3_
#define _DYNAMIC_LIB3_

#include <dynamic_lib2.h>

#if !defined(DYNAMIC_LIB3_IMPORT) && !defined(DYNAMIC_LIB3_INTERNAL)
#error See compile declarations -- !defined(DYNAMIC_LIB3_IMPORT) && !defined(DYNAMIC_LIB3_INTERNAL)
#endif

#if defined(DYNAMIC_LIB3_IMPORT) && defined(DYNAMIC_LIB3_INTERNAL)
#error See compile declarations -- defined(DYNAMIC_LIB3_IMPORT) && defined(DYNAMIC_LIB3_INTERNAL)
#endif


#if defined( __WIN32__ ) || defined( _WIN32 )
    #ifdef DYNAMIC_LIB3_IMPORT
        #define DYNAMIC_LIB3_SYMBOL __declspec(dllimport)
    #else
        #define DYNAMIC_LIB3_SYMBOL __declspec(dllexport)
    #endif
#else
    #define DYNAMIC_LIB3_SYMBOL
#endif


namespace dynamic_lib3
{
    void DYNAMIC_LIB3_SYMBOL test();
}

#endif
