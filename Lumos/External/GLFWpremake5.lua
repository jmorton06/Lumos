project "glfw"
  kind "StaticLib"
  language "C"

  removeplatforms "emscripten"

  filter "configurations:Debug"
		symbols "On"

  filter "configurations:Release"
    runtime "Release"
		optimize "On"

  filter "configurations:Production"
    runtime "Release"
    optimize "On"

  filter "system:windows"
    systemversion "latest"

  filter {}

  files {
    "glfw/src/context.c",
    "glfw/src/init.c",
    "glfw/src/input.c",
    "glfw/src/monitor.c",
    "glfw/src/vulkan.c",
    "glfw/src/window.c",
    "glfw/src/glfw_config.h",
    "glfw/include/GLFW/glfw3.h",
    "glfw/include/GLFW/glfw3native.h"
  }

  includedirs {
    "glfw/include"
  }

  filter "system:macosx"
    defines { "_GLFW_COCOA", "_GLFW_USE_RETINA" }
    
    files {
      "glfw/src/cocoa_platform.h",
      "glfw/src/cocoa_joystick.h",
      "glfw/src/nsgl_context.h",
      "glfw/src/cocoa_init.m",
      "glfw/src/cocoa_joystick.m",
      "glfw/src/cocoa_monitor.m",
      "glfw/src/cocoa_window.m",
      "glfw/src/cocoa_time.c",
      "glfw/src/nsgl_context.m",
      "glfw/src/posix_thread.h",
      "glfw/src/posix_thread.c",
      "glfw/src/egl_context.h",
      "glfw/src/egl_context.c",
      "glfw/src/osmesa_context.h",
      "glfw/src/osmesa_context.c"
    }

  filter "system:win32 or win64 or mingw32"
    defines {
      "_GLFW_WIN32",
      "_CRT_SECURE_NO_WARNINGS"
    }

	staticruntime "On"

   	buildoptions { "/MP" }

    files {
      "glfw/src/win32_platform.h",
      "glfw/src/win32_joystick.h",
      "glfw/src/wgl_context.h",
      "glfw/src/egl_context.h",
      "glfw/src/osmesa_context.h",
      "glfw/src/win32_init.c",
      "glfw/src/win32_joystick.c",
      "glfw/src/win32_monitor.c",
      "glfw/src/win32_time.c",
      "glfw/src/win32_window.c",
      "glfw/src/win32_thread.c",
      "glfw/src/wgl_context.c",
      "glfw/src/egl_context.c",
      "glfw/src/osmesa_context.c"
    }

  filter "system:linux"
    defines {
      "_GLFW_X11",
      "_GLFW_HAS_XF86VM"
    }

    files {
      "glfw/src/egl_context.h",
      "glfw/src/egl_context.c",
      "glfw/src/glx_context.h",
      "glfw/src/glx_context.c",
      "glfw/src/linux_joystick.h",
      "glfw/src/linux_joystick.c",
      "glfw/src/posix_time.h",
      "glfw/src/posix_time.c",
      "glfw/src/x11_init.c",
      "glfw/src/x11_platform.h",
      "glfw/src/x11_monitor.c",
      "glfw/src/x11_window.c",
      "glfw/src/xkb_unicode.h",
      "glfw/src/xkb_unicode.c",
      "glfw/src/posix_thread.h",
      "glfw/src/posix_thread.c",
      "glfw/src/osmesa_context.c",
      "glfw/src/osmesa_context.h"
    }

    buildoptions
    {
      "-fPIC"
    }
