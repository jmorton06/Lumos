project(volk)
cmake_minimum_required(VERSION 3.0)

add_library(volk STATIC volk.c)

if(MSVC)
  target_compile_options(volk PRIVATE /W4 /WX)
  add_definitions("-DVK_USE_PLATFORM_WIN32_KHR")
else()
  target_compile_options(volk PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(volk PRIVATE dl)
endif()

if(UNIX)
if(APPLE)
add_definitions("-DVK_USE_PLATFORM_MACOS_MVK")
else()
add_definitions("-DVK_USE_PLATFORM_XCB_KHR")
endif(APPLE)
endif(UNIX)


include_directories(
  "../"
)
if(DEFINED ENV{VULKAN_SDK})
	target_include_directories(volk PUBLIC "$ENV{VULKAN_SDK}/include")
endif()
