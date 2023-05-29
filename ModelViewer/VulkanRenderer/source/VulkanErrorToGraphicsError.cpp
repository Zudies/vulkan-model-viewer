#include "pch.h"
#include "VulkanErrorToGraphicsError.h"

using namespace Graphics;

namespace Vulkan {

Graphics::GraphicsError VulkanErrorToGraphicsError(VkResult result) {
    switch (result) {
    case VK_SUCCESS:
        return GraphicsError::OK;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return GraphicsError::OUT_OF_HOST_MEMORY;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return GraphicsError::OUT_OF_DEVICE_MEMORY;
    case VK_ERROR_INITIALIZATION_FAILED:
        return GraphicsError::INITIALIZATION_FAILED;
    case VK_ERROR_LAYER_NOT_PRESENT:
        return GraphicsError::NO_SUCH_LAYER;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return GraphicsError::NO_SUCH_EXTENSION;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return GraphicsError::UNSUPPORTED_API_VERSION;

    default:
        return GraphicsError::UNKNOWN;
    }
}

} // namespace Vulkan
