#include <framework1/framework1.h>

#ifndef EXECUTABLE4_INTERNAL
#error See compile declarations -- EXECUTABLE4_INTERNAL
#endif


#ifndef FRAMEWORK1_IMPORT
#error See compile declarations -- FRAMEWORK1_IMPORT
#endif


#ifndef DYNAMIC_LIB2_IMPORT
#error See compile declarations -- DYNAMIC_LIB2_IMPORT
#endif


#ifndef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


int main(int argc, char** argv)
{
    framework1::test();
    dynamic_lib2::test();
    static_lib1::test();

    return 0;
}
