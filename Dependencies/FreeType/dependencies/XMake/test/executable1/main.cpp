#include <static_lib2.h>

#ifndef EXECUTABLE1_INTERNAL
#error See compile declarations -- EXECUTABLE1_INTERNAL
#endif


#ifndef STATIC_LIB2_IMPORT
#error See compile declarations -- STATIC_LIB2_IMPORT
#endif


#ifndef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


int main(int argc, char** argv)
{
    static_lib2::test();
    static_lib1::test();

    return 0;
}
