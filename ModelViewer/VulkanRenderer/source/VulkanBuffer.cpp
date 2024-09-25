#include "pch.h"
#include "VulkanBuffer.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanBuffer::VulkanBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_vkBuffer(VK_NULL_HANDLE),
    m_vkMemory(VK_NULL_HANDLE),
    m_mappedMemory(nullptr) {
    ASSERT(renderer);
}

VulkanBuffer::~VulkanBuffer() {
    Clear();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
  : m_renderer(other.m_renderer),
    m_vkBuffer(other.m_vkBuffer),
    m_vkMemory(other.m_vkMemory),
    m_mappedMemory(other.m_mappedMemory) {
    other.m_vkBuffer = VK_NULL_HANDLE;
    other.m_vkMemory = VK_NULL_HANDLE;
    other.m_mappedMemory = nullptr;
}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept {
    m_renderer = other.m_renderer;
    m_vkBuffer = other.m_vkBuffer;
    m_vkMemory = other.m_vkMemory;
    m_mappedMemory = other.m_mappedMemory;
    other.m_vkBuffer = VK_NULL_HANDLE;
    other.m_vkMemory = VK_NULL_HANDLE;
    other.m_mappedMemory = nullptr;

    return *this;
}

Graphics::GraphicsError VulkanBuffer::Initialize(VkDeviceSize size, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount) {
    ASSERT(!m_vkBuffer);

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = queueFamilyCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = queueFamilyCount;
    createInfo.pQueueFamilyIndices = queueFamilies;

    if (vkCreateBuffer(m_renderer->GetDevice(), &createInfo, nullptr, &m_vkBuffer) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanBuffer::Allocate(VkMemoryPropertyFlags properties) {
    if (m_vkMemory) {
        return Graphics::GraphicsError::OK;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_renderer->GetDevice(), m_vkBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    if (m_renderer->GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties, 0, &allocInfo.memoryTypeIndex) != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    if (vkAllocateMemory(m_renderer->GetDevice(), &allocInfo, nullptr, &m_vkMemory) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    if (vkBindBufferMemory(m_renderer->GetDevice(), m_vkBuffer, m_vkMemory, 0) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    return Graphics::GraphicsError::OK;
}

void *VulkanBuffer::GetMappedMemory() {
    if (!m_mappedMemory) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_renderer->GetDevice(), m_vkBuffer, &memRequirements);
        if (vkMapMemory(m_renderer->GetDevice(), m_vkMemory, 0, memRequirements.size, 0, &m_mappedMemory) != VK_SUCCESS) {
            return nullptr;
        }
    }

    return m_mappedMemory;
}

void VulkanBuffer::UnmapMemory() {
    if (m_mappedMemory) {
        vkUnmapMemory(m_renderer->GetDevice(), m_vkMemory);
        m_mappedMemory = nullptr;
    }
}

VkBuffer &VulkanBuffer::GetVkBuffer() {
    return m_vkBuffer;
}

VkDeviceMemory &VulkanBuffer::GetVkDeviceMemory() {
    return m_vkMemory;
}

void VulkanBuffer::Clear() {
    UnmapMemory();

    if (m_vkMemory) {
        vkFreeMemory(m_renderer->GetDevice(), m_vkMemory, VK_NULL_HANDLE);
        m_vkMemory = VK_NULL_HANDLE;
    }
    if (m_vkBuffer) {
        vkDestroyBuffer(m_renderer->GetDevice(), m_vkBuffer, VK_NULL_HANDLE);
        m_vkBuffer = VK_NULL_HANDLE;
    }
}

} // namespace Vulkan
