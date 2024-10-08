#pragma once

#include "VulkanBuffer.h"

namespace Vulkan {

class RendererImpl;

class VulkanImageBuffer {
public:

    VulkanImageBuffer(RendererImpl *renderer);
    VulkanImageBuffer(VulkanImageBuffer const &) = delete;
    VulkanImageBuffer &operator=(VulkanImageBuffer const &) = delete;
    ~VulkanImageBuffer();

    VulkanImageBuffer(VulkanImageBuffer &&other) noexcept;
    VulkanImageBuffer &operator=(VulkanImageBuffer &&other) noexcept;

    // Required
    void SetFormat(VkFormat format);
    VkFormat GetFormat() const;

    // Sets the first supported format based on the current tiling mode
    // Returns false if no format was set
    bool SetFormatBestCandidate(const VkFormat *formats, size_t formatCount, VkImageUsageFlags usage);

    // Required
    void SetExtents(uint32_t width, uint32_t height, uint32_t depth);
    VkExtent3D GetExtents() const;

    // Default: VK_IMAGE_TYPE_2D
    void SetImageType(VkImageType imageType);
    VkImageType GetImageType() const;

    // Default: 1
    void SetMipLevels(uint32_t mipLevels);
    uint32_t GetMipLevels() const;

    // Default: 1
    void SetArrayLayers(uint32_t arrayLayers);
    uint32_t GetArrayLayers() const;

    // Default VK_SAMPLE_COUNT_1_BIT
    void SetSamples(VkSampleCountFlagBits samples);
    VkSampleCountFlagBits GetSamples() const;

    // Default: VK_IMAGE_TILING_OPTIMAL
    void SetTiling(VkImageTiling tiling);
    VkImageTiling GetTiling() const;

    // Default: VK_IMAGE_LAYOUT_UNDEFINED
    void SetInitialLayout(VkImageLayout initialLayout);

    Graphics::GraphicsError Initialize(VkImageUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount);
    Graphics::GraphicsError Allocate(VkMemoryPropertyFlags properties); // No device memory is allocated until this is called

    VkImage GetVkImage() const;
    VkDeviceMemory GetVkDeviceMemory() const;

    void Clear();

    bool IsFormatSupported(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);

private:

    RendererImpl *m_renderer;
    VkImageCreateInfo m_imageProperties;
    VkImage m_vkImage;
    VkDeviceMemory m_vkMemory;

};

} // namespace Vulkan
