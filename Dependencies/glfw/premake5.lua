project "glfw"
  kind "StaticLib"
  language "C"
  systemversion "latest"

  removeplatforms "emscripten"

  filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

	filter "configurations:Dist"
    optimize "On"

  filter {}

  files {
    "src/context.c",
    "src/init.c",
    "src/input.c",
    "src/monitor.c",
    "src/vulkan.c",
    "src/window.c",
    "src/glfw_config.h",
    "include/GLFW/glfw3.h",
    "include/GLFW/glfw3native.h"
  }

  includedirs {
    "glfw/include"
  }

  filter "system:macosx"
    defines { "_GLFW_COCOA", "_GLFW_USE_RETINA" }

    files {
      "src/cocoa_platform.h",
      "src/cocoa_joystick.h",
      "src/nsgl_context.h",
      "src/cocoa_init.m",
      "src/cocoa_joystick.m",
      "src/cocoa_monitor.m",
      "src/cocoa_window.m",
      "src/cocoa_time.c",
      "src/nsgl_context.m",
      "src/posix_thread.h",
      "src/posix_thread.c",
      "src/egl_context.h",
      "src/egl_context.c",
      "src/osmesa_context.h",
      "src/osmesa_context.c"
    }

  filter "system:win32 or win64 or mingw32"
    defines {
      "_GLFW_WIN32",
      "_CRT_SECURE_NO_WARNINGS"
    }

   	buildoptions { "/MP" }

    files {
      "src/win32_platform.h",
      "src/win32_joystick.h",
      "src/wgl_context.h",
      "src/egl_context.h",
      "src/osmesa_context.h",
      "src/win32_init.c",
      "src/win32_joystick.c",
      "src/win32_monitor.c",
      "src/win32_time.c",
      "src/win32_window.c",
      "src/win32_thread.c",
      "src/wgl_context.c",
      "src/egl_context.c",
      "src/osmesa_context.c"
    }

  filter "system:linux"
    defines {
      "_GLFW_X11",
      "_GLFW_HAS_XF86VM"
    }

    files {
      "src/egl_context.h",
      "src/egl_context.c",
      "src/glx_context.h",
      "src/glx_context.c",
      "src/linux_joystick.h",
      "src/linux_joystick.c",
      "src/posix_time.h",
      "src/posix_time.c",
      "src/x11_init.c",
      "src/x11_platform.h",
      "src/x11_monitor.c",
      "src/x11_window.c",
      "src/xkb_unicode.h",
      "src/xkb_unicode.c",
      "src/posix_thread.h",
      "src/posix_thread.c",
      "src/osmesa_context.c",
      "src/osmesa_context.h"
    }

    buildoptions
    {
      "-fPIC"
    }