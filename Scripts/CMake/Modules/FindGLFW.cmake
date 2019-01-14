
#find GLFW

find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h "${CMAKE_SOURCE_DIR}/../glfw/include")
find_library(GLFW_SHARED_LIBRARY NAMES glfw glfw3dll)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW REQUIRED_VARS GLFW_INCLUDE_DIR GLFW_SHARED_LIBRARY)

mark_as_advanced(GLFW_INCLUDE_DIR GLFW_SHARED_LIBRARY)
