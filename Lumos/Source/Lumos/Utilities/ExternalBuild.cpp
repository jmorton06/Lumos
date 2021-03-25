#include "Precompiled.h"

#pragma warning(push, 0)
#ifdef LUMOS_RENDER_API_OPENGL
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui/backends/imgui_impl_opengl3.cpp>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include <imgui/backends/imgui_impl_vulkan.cpp>
#endif

#include <imgui/misc/freetype/imgui_freetype.cpp>

#include <stb/stb_vorbis.c>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb/stb_perlin.h>

#ifdef LUMOS_RENDER_API_OPENGL
#include <glad/src/glad.c>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#ifdef LUMOS_VOLK
#include <volk/volk.c>
#endif
#endif

#include <OpenFBX/miniz.c>
#include <OpenFBX/ofbx.cpp>
#include <Tracy/TracyClient.cpp>
#pragma warning(pop)
