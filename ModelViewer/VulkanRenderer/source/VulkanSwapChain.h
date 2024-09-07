#pragma once

#include <string>

namespace Vulkan {

class VulkanSwapChain {
public:
    static VkFormat StringToFormat(std::string const &format);
    static VkColorSpaceKHR StringToColorSpace(std::string const &colorSpace);
    static VkPresentModeKHR StringToPresentMode(std::string const &presentMode);

    typedef std::vector<VkImage> ImageArray;
    typedef std::vector<VkImageView> ImageViewArray;
public:
    VulkanSwapChain();

    VkSurfaceKHR GetSurface() const;
    u32 GetIndex() const;
    VkFormat GetFormat() const;
    VkColorSpaceKHR GetColorSpace() const;
    VkPresentModeKHR GetPresentMode() const;
    VkExtent2D GetExtents() const;

    VkSwapchainKHR GetSwapchain() const;
    const ImageArray &GetImages() const;
    const ImageViewArray &GetImageViews() const;

    void SetSurface(VkSurfaceKHR surface);
    void SetIndex(u32 index);
    void SetFormat(VkFormat format);
    void SetColorSpace(VkColorSpaceKHR colorSpace);
    void SetPresentMode(VkPresentModeKHR presentMode);
    void SetExtents(VkExtent2D extents);

    void SetSwapchain(VkSwapchainKHR swapchain);
    void SetImages(ImageArray &images); // Note: Will swap with internal
    void SetImageViews(ImageViewArray &imageViews); // Note: Will swap with internal

private:

    VkSurfaceKHR m_vkSurface;
    u32 m_index;
    VkFormat m_format;
    VkColorSpaceKHR m_colorSpace;
    VkPresentModeKHR m_presentMode;
    VkExtent2D m_extents;

    VkSwapchainKHR m_vkSwapchain;
    ImageArray m_vkImages;
    ImageViewArray m_vkImageViews;

};

} // namespace Vulkan
