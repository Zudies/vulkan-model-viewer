// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// Set to have staging buffers transfer exclusively with the transfer queue
#define VK_BUFFERS_FORCE_NO_TRANSFER_QUEUE 0

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"

#include "Common.h"
#include "ErrorCodes.h"
#include "Version.h"
#include <vulkan.h>
#include "VulkanErrorToGraphicsError.h"

#include <optional>
#include <string>
#include <limits>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <functional>

#endif //PCH_H
