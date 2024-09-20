#pragma once

#include "VulkanImageBuffer.h"

namespace Vulkan {

class RendererImpl;

class VulkanDepthStencilBuffer {
public:

    VulkanDepthStencilBuffer(RendererImpl *renderer);
    VulkanDepthStencilBuffer(VulkanDepthStencilBuffer const &) = delete;
    VulkanDepthStencilBuffer &operator=(VulkanDepthStencilBuffer const &) = delete;
    ~VulkanDepthStencilBuffer();

    Graphics::GraphicsError Initialize(uint32_t width, uint32_t height, VkFormat desiredFormat);

    VkFormat GetFormat() const;
    bool HasStencilComponent() const;
    VkExtent2D GetExtents() const;

    VkImage GetDeviceImage() const;
    VkImageView GetDeviceImageView() const;

    void Clear();

private:

    RendererImpl *m_renderer;
    VulkanImageBuffer m_imageBuffer;
    VkImageView m_imageView;

};

} // namespace Vulkan
