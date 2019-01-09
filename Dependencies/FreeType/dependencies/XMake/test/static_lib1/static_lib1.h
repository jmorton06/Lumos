#ifndef _STATIC_LIB1_
#define _STATIC_LIB1_


#if !defined(STATIC_LIB1_IMPORT) && !defined(STATIC_LIB1_INTERNAL)
#error See compile declarations -- !defined(STATIC_LIB1_IMPORT) && !defined(STATIC_LIB1_INTERNAL)
#endif

#if defined(STATIC_LIB1_IMPORT) && defined(STATIC_LIB1_INTERNAL)
#error See compile declarations -- defined(STATIC_LIB1_IMPORT) && defined(STATIC_LIB1_INTERNAL)
#endif


namespace static_lib1
{
    void test();
}

#endif
