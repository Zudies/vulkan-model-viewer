// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include "Common.h"
#include "ErrorCodes.h"
#include "Version.h"
#include <vulkan.h>
#include "VulkanErrorToGraphicsError.h"

#include <optional>
#include <string>
#include <limits>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <algorithm>

#endif //PCH_H
