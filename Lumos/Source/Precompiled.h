#pragma once

#ifndef LUMOS_PLATFORM_MACOS
#ifdef __cplusplus
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <cstddef>
#include <cfloat>
#include <cstring>
#include <utility>
#include <memory>
#include <thread>
#include <math.h>

#include "Core/Reference.h"
#include "Core/LMLog.h"
#include "Core/Core.h"
#include "Core/Profiler.h"
#include "Core/Thread.h"
#include "Scene/Entity.h"

#include <cereal/cereal.hpp>
#include <imgui/imgui.h>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VK.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include <glad/glad.h>
#endif

#include <sol/sol.hpp>
#endif
#endif
