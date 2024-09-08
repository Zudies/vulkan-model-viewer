#include "pch.h"
#include "VulkanSwapChain.h"
#include "VulkanFeaturesDefines.h"

namespace Vulkan {

VulkanSwapChain::VulkanSwapChain()
  : m_vkSurface(VK_NULL_HANDLE),
    m_index(std::numeric_limits<u32>::max()),
    m_format(VK_FORMAT_UNDEFINED),
    m_colorSpace(VK_COLOR_SPACE_MAX_ENUM_KHR),
    m_presentMode(VK_PRESENT_MODE_MAX_ENUM_KHR),
    m_extents{},
    m_vkSwapchain(VK_NULL_HANDLE) {
}

VkSurfaceKHR VulkanSwapChain::GetSurface() const {
    return m_vkSurface;
}

u32 VulkanSwapChain::GetIndex() const {
    return m_index;
}

VkFormat VulkanSwapChain::GetFormat() const {
    return m_format;
}

VkColorSpaceKHR VulkanSwapChain::GetColorSpace() const {
    return m_colorSpace;
}

VkPresentModeKHR VulkanSwapChain::GetPresentMode() const {
    return m_presentMode;
}

VkExtent2D VulkanSwapChain::GetExtents() const {
    return m_extents;
}

VkSwapchainKHR VulkanSwapChain::GetSwapchain() const {
    return m_vkSwapchain;
}

const VulkanSwapChain::ImageArray &VulkanSwapChain::GetImages() const {
    return m_vkImages;
}

const VulkanSwapChain::ImageViewArray &VulkanSwapChain::GetImageViews() const {
    return m_vkImageViews;
}

void VulkanSwapChain::SetSurface(VkSurfaceKHR surface) {
    m_vkSurface = surface;
}

void VulkanSwapChain::SetIndex(u32 index) {
    m_index = index;
}

void VulkanSwapChain::SetFormat(VkFormat format) {
    m_format = format;
}

void VulkanSwapChain::SetColorSpace(VkColorSpaceKHR colorSpace) {
    m_colorSpace = colorSpace;
}

void VulkanSwapChain::SetPresentMode(VkPresentModeKHR presentMode) {
    m_presentMode = presentMode;
}

void VulkanSwapChain::SetExtents(VkExtent2D extents) {
    m_extents = extents;
}

void VulkanSwapChain::SetSwapchain(VkSwapchainKHR swapchain) {
    m_vkSwapchain = swapchain;
}

void VulkanSwapChain::SetImages(ImageArray &images) {
    m_vkImages.swap(images);
}

void VulkanSwapChain::SetImageViews(ImageViewArray &imageViews) {
    m_vkImageViews.swap(imageViews);
}

VkFormat VulkanSwapChain::StringToFormat(std::string const &format) {
    if (format == SURFACE_FORMAT_R8G8B8_UINT) {
        return VK_FORMAT_R8G8B8_UINT;
    }
    else if (format == SURFACE_FORMAT_R8G8B8_SINT) {
        return VK_FORMAT_R8G8B8_SINT;
    }
    else if (format == SURFACE_FORMAT_B8G8R8_UNORM) {
        return VK_FORMAT_B8G8R8_UNORM;
    }
    else if (format == SURFACE_FORMAT_B8G8R8_SNORM) {
        return VK_FORMAT_B8G8R8_SNORM;
    }
    else if (format == SURFACE_FORMAT_B8G8R8_SRGB) {
        return VK_FORMAT_B8G8R8_SRGB;
    }
    else if (format == SURFACE_FORMAT_R8G8B8A8_UINT) {
        return VK_FORMAT_R8G8B8A8_UINT;
    }
    else if (format == SURFACE_FORMAT_R8G8B8A8_SINT) {
        return VK_FORMAT_R8G8B8A8_SINT;
    }
    else if (format == SURFACE_FORMAT_B8G8R8A8_UNORM) {
        return VK_FORMAT_B8G8R8A8_UNORM;
    }
    else if (format == SURFACE_FORMAT_B8G8R8A8_SNORM) {
        return VK_FORMAT_B8G8R8A8_SNORM;
    }
    else if (format == SURFACE_FORMAT_B8G8R8A8_SRGB) {
        return VK_FORMAT_B8G8R8A8_SRGB;
    }

    ERROR_MSG(L"Unknown format: %hs", format.c_str());
    return VK_FORMAT_UNDEFINED;
}

VkColorSpaceKHR VulkanSwapChain::StringToColorSpace(std::string const &colorSpace) {
    if (colorSpace == SURFACE_COLOR_SPACE_SRGB_NONLINEAR) {
        return VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    }

    ERROR_MSG(L"Unknown color space: %hs", colorSpace.c_str());
    return VK_COLOR_SPACE_MAX_ENUM_KHR;
}

VkPresentModeKHR VulkanSwapChain::StringToPresentMode(std::string const &presentMode) {
    if (presentMode == PRESENT_MODE_IMMEDIATE) {
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    else if (presentMode == PRESENT_MODE_FIFO) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    else if (presentMode == PRESENT_MODE_FIFO_RELAXED) {
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    else if (presentMode == PRESENT_MODE_MAILBOX) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    ERROR_MSG(L"Unknown present mode: %hs", presentMode.c_str());
    return VK_PRESENT_MODE_MAX_ENUM_KHR;
}

} // namespace Vulkan
