#include <dynamic_lib4.h>
#include <dynamic_lib3.h>

#ifndef EXECUTABLE3_INTERNAL
#error See compile declarations -- EXECUTABLE3_INTERNAL
#endif


#ifndef DYNAMIC_LIB4_IMPORT
#error See compile declarations -- DYNAMIC_LIB4_IMPORT
#endif


#ifndef DYNAMIC_LIB3_IMPORT
#error See compile declarations -- DYNAMIC_LIB3_IMPORT
#endif


#ifndef DYNAMIC_LIB2_IMPORT
#error See compile declarations -- DYNAMIC_LIB2_IMPORT
#endif


#ifndef DYNAMIC_LIB1_IMPORT
#error See compile declarations -- DYNAMIC_LIB1_IMPORT
#endif


#ifndef STATIC_LIB1_IMPORT
#error See compile declarations -- STATIC_LIB1_IMPORT
#endif


int main(int argc, char** argv)
{
    dynamic_lib4::test();
    dynamic_lib3::test();
    dynamic_lib2::test();
    dynamic_lib1::test();
    static_lib1::test();

    return 0;
}
