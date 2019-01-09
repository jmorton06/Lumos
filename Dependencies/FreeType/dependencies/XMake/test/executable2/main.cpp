#include <dynamic_lib1.h>

#ifndef EXECUTABLE2_INTERNAL
#error See compile declarations -- EXECUTABLE2_INTERNAL
#endif


#ifndef DYNAMIC_LIB1_IMPORT
#error See compile declarations -- DYNAMIC_LIB1_IMPORT
#endif


#ifndef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


int main(int argc, char** argv)
{
    dynamic_lib1::test();
    static_lib1::test();

    return 0;
}
