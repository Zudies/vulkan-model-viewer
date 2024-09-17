#include "pch.h"
#include "VulkanDepthStencilBuffer.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanDepthStencilBuffer::VulkanDepthStencilBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_imageBuffer(renderer),
    m_imageView(VK_NULL_HANDLE) {
    ASSERT(renderer);
}

VulkanDepthStencilBuffer::~VulkanDepthStencilBuffer() {
    Clear();
}

Graphics::GraphicsError VulkanDepthStencilBuffer::Initialize(uint32_t width, uint32_t height) {
    static const VkFormat FORMAT_CANDIDATES[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    m_imageBuffer.SetExtents(width, height, 1);
    m_imageBuffer.SetFormatBestCandidate(FORMAT_CANDIDATES, countof(FORMAT_CANDIDATES), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    auto err = m_imageBuffer.Initialize(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, nullptr, 0);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    err = m_imageBuffer.Allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_imageBuffer.GetVkImage();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_imageBuffer.GetFormat();
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_renderer->GetDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    return Graphics::GraphicsError::OK;
}

VkFormat VulkanDepthStencilBuffer::GetFormat() const {
    return m_imageBuffer.GetFormat();
}

bool VulkanDepthStencilBuffer::HasStencilComponent() const {
    switch (m_imageBuffer.GetFormat()) {
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return true;
    default:
        return false;
    }
}

VkExtent2D VulkanDepthStencilBuffer::GetExtents() const {
    auto extents = m_imageBuffer.GetExtents();
    return { extents.width, extents.height };
}

VkImage VulkanDepthStencilBuffer::GetDeviceImage() const {
    return m_imageBuffer.GetVkImage();
}

VkImageView VulkanDepthStencilBuffer::GetDeviceImageView() const {
    return m_imageView;
}

void VulkanDepthStencilBuffer::Clear() {
    if (m_imageView) {
        vkDestroyImageView(m_renderer->GetDevice(), m_imageView, VK_NULL_HANDLE);
        m_imageView = VK_NULL_HANDLE;
    }

    m_imageBuffer.Clear();
}

} // namespace Vulkan
