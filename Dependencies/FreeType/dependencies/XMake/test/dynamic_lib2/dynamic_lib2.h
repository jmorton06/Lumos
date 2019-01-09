#ifndef _DYNAMIC_LIB2_
#define _DYNAMIC_LIB2_

#if !defined(DYNAMIC_LIB2_IMPORT) && !defined(DYNAMIC_LIB2_INTERNAL)
#error See compile declarations -- !defined(DYNAMIC_LIB2_IMPORT) && !defined(DYNAMIC_LIB2_INTERNAL)
#endif

#if defined(DYNAMIC_LIB2_IMPORT) && defined(DYNAMIC_LIB2_INTERNAL)
#error See compile declarations -- defined(DYNAMIC_LIB2_IMPORT) && defined(DYNAMIC_LIB2_INTERNAL)
#endif


#if defined( __WIN32__ ) || defined( _WIN32 )
    #ifdef DYNAMIC_LIB2_IMPORT
        #define DYNAMIC_LIB2_SYMBOL __declspec(dllimport)
    #else
        #define DYNAMIC_LIB2_SYMBOL __declspec(dllexport)
    #endif
#else
    #define DYNAMIC_LIB2_SYMBOL
#endif


namespace dynamic_lib2
{
    void DYNAMIC_LIB2_SYMBOL test();
}

#endif
