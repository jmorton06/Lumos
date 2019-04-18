#include "LM.h"

#ifdef LUMOS_RENDER_API_OPENGL
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui/examples/imgui_impl_opengl3.cpp>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include <imgui/examples/imgui_impl_vulkan.cpp>
#endif

#include <imgui/imgui.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_widgets.cpp>

#include <imgui/plugins/ImGuizmo.cpp>
#include <imgui/plugins/ImSequencer.cpp>
#include <imgui/plugins/ImGradient.cpp>
//#include <imgui/plugins/ImCurveEdit.cpp>

#include <stb/stb_vorbis.c>

#include <simplex/simplexnoise.cpp>