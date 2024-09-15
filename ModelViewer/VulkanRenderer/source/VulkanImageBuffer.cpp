#include "pch.h"
#include "VulkanImageBuffer.h"
#include "VulkanRendererImpl.h"

#include "ImageLoader.h"

namespace Vulkan {

VulkanImageBuffer::VulkanImageBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_imageProperties{},
    m_vkImage(VK_NULL_HANDLE),
    m_vkMemory(VK_NULL_HANDLE) {
    ASSERT(renderer);

    m_imageProperties.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_imageProperties.imageType = VK_IMAGE_TYPE_2D;
    m_imageProperties.mipLevels = 1;
    m_imageProperties.arrayLayers = 1;
    m_imageProperties.samples = VK_SAMPLE_COUNT_1_BIT;
    m_imageProperties.tiling = VK_IMAGE_TILING_OPTIMAL;
    m_imageProperties.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

VulkanImageBuffer::~VulkanImageBuffer() {
    Clear();
}

void VulkanImageBuffer::SetFormat(VkFormat format) {
    m_imageProperties.format = format;
}

VkFormat VulkanImageBuffer::GetFormat() const {
    return m_imageProperties.format;
}

void VulkanImageBuffer::SetExtents(uint32_t width, uint32_t height, uint32_t depth) {
    m_imageProperties.extent.width = width;
    m_imageProperties.extent.height = height;
    m_imageProperties.extent.depth = depth;
}

VkExtent3D VulkanImageBuffer::GetExtents() const {
    return m_imageProperties.extent;
}

void VulkanImageBuffer::SetImageType(VkImageType imageType) {
    m_imageProperties.imageType = imageType;
}

VkImageType VulkanImageBuffer::GetImageType() const {
    return m_imageProperties.imageType;
}

void VulkanImageBuffer::SetMipLevels(uint32_t mipLevels) {
    m_imageProperties.mipLevels = mipLevels;
}

uint32_t VulkanImageBuffer::GetMipLevels() const {
    return m_imageProperties.mipLevels;
}

void VulkanImageBuffer::SetArrayLayers(uint32_t arrayLayers) {
    m_imageProperties.arrayLayers = arrayLayers;
}

uint32_t VulkanImageBuffer::GetArrayLayers() const {
    return m_imageProperties.arrayLayers;
}

void VulkanImageBuffer::SetSamples(VkSampleCountFlagBits samples) {
    m_imageProperties.samples = samples;
}

VkSampleCountFlagBits VulkanImageBuffer::GetSamples() const {
    return m_imageProperties.samples;
}

void VulkanImageBuffer::SetTiling(VkImageTiling tiling) {
    m_imageProperties.tiling = tiling;
}

VkImageTiling VulkanImageBuffer::GetTiling() const {
    return m_imageProperties.tiling;
}

void VulkanImageBuffer::SetInitialLayout(VkImageLayout initialLayout) {
    m_imageProperties.initialLayout = initialLayout;
}

Graphics::GraphicsError VulkanImageBuffer::Initialize(VkImageUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount) {
    ASSERT(!m_vkImage);

    m_imageProperties.usage = usage;
    m_imageProperties.sharingMode = queueFamilyCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    m_imageProperties.queueFamilyIndexCount = queueFamilyCount;
    m_imageProperties.pQueueFamilyIndices = queueFamilies;

    if (vkCreateImage(m_renderer->GetDevice(), &m_imageProperties, VK_NULL_HANDLE, &m_vkImage) != VK_SUCCESS) {
        return Graphics::GraphicsError::IMAGE_CREATE_ERROR;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanImageBuffer::Allocate(VkMemoryPropertyFlags properties) {
    if (m_vkMemory) {
        return Graphics::GraphicsError::OK;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_renderer->GetDevice(), m_vkImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    if (m_renderer->GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties, 0, &allocInfo.memoryTypeIndex) != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    if (vkAllocateMemory(m_renderer->GetDevice(), &allocInfo, nullptr, &m_vkMemory) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    if (vkBindImageMemory(m_renderer->GetDevice(), m_vkImage, m_vkMemory, 0) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    return Graphics::GraphicsError::OK;
}

VkImage VulkanImageBuffer::GetVkImage() const {
    return m_vkImage;
}

VkDeviceMemory VulkanImageBuffer::GetVkDeviceMemory() const {
    return m_vkMemory;
}

void VulkanImageBuffer::Clear() {
    if (m_vkMemory) {
        vkFreeMemory(m_renderer->GetDevice(), m_vkMemory, VK_NULL_HANDLE);
        m_vkMemory = VK_NULL_HANDLE;
    }
    if (m_vkImage) {
        vkDestroyImage(m_renderer->GetDevice(), m_vkImage, VK_NULL_HANDLE);
        m_vkImage = VK_NULL_HANDLE;
    }
}


} // namespace Vulkan
