#pragma once

#ifdef __cplusplus
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>

#include <vector>
#include <list>
#include <array>
#include <map>
#include <unordered_map>
#include <cstddef>
#include <fstream>
#include <cfloat>
#include <cstring>
#include <utility>
#include <memory>
#include <thread>

#include <stdio.h>
#include <math.h>

#include "Core/Reference.h"
#include "Core/LMLog.h"
#include "Core/Core.h"
#include "Core/Profiler.h"
#include "Core/Thread.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cereal/cereal.hpp>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VK.h"
#endif
#endif
