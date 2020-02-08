#include "lmpch.h"

#ifdef LUMOS_RENDER_API_OPENGL
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui/examples/imgui_impl_opengl3.cpp>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include <imgui/examples/imgui_impl_vulkan.cpp>
#endif

#include <stb/stb_vorbis.c>
#include <simplex/simplexnoise.cpp>

#ifdef LUMOS_RENDER_API_OPENGL
#include <glad/src/glad.c>
#ifdef LUMOS_PLATFORM_WINDOWS
#include <glad/src/glad_wgl.c>
#endif
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#ifdef LUMOS_VOLK
#include <volk/volk.c>
#endif
#endif
