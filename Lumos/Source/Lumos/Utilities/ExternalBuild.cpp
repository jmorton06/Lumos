#include "Precompiled.h"

#pragma warning(push, 0)
#include <Tracy/TracyClient.cpp>

#ifdef LUMOS_RENDER_API_OPENGL
#include <glad/src/glad.c>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#ifdef LUMOS_VOLK
#define VOLK_IMPLEMENTATION
#include <vulkan/volk/volk.h>
#endif
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <imgui/backends/imgui_impl_opengl3.cpp>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#undef IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#undef VK_NO_PROTOTYPES
#include <imgui/backends/imgui_impl_vulkan.cpp>
#endif

#include <imgui/misc/freetype/imgui_freetype.cpp>

#define STB_DEFINE
#include <stb/deprecated/stb.h>

#include <stb/stb_vorbis.c>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb/stb_perlin.h>

#include <OpenFBX/miniz.c>
#include <OpenFBX/ofbx.cpp>
#pragma warning(pop)
