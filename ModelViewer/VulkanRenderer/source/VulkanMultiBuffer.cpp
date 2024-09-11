#include "pch.h"
#include "VulkanMultiBuffer.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanMultiBuffer::VulkanMultiBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_sizePerBuffer(0),
    m_vkMemory(VK_NULL_HANDLE),
    m_mappedMemory(nullptr) {
    ASSERT(renderer);
}

VulkanMultiBuffer::~VulkanMultiBuffer() {
    Clear();
}

Graphics::GraphicsError VulkanMultiBuffer::Initialize(VkDeviceSize sizePerBuffer, size_t bufferCount, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount) {
    ASSERT(m_vkBuffers.empty());

    m_vkBuffers.resize(bufferCount);

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = sizePerBuffer;
    createInfo.usage = usage;
    createInfo.sharingMode = queueFamilyCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = queueFamilyCount;
    createInfo.pQueueFamilyIndices = queueFamilies;
    for (size_t i = 0; i < bufferCount; ++i) {
        if (vkCreateBuffer(m_renderer->GetDevice(), &createInfo, nullptr, &m_vkBuffers[i]) != VK_SUCCESS) {
            return Graphics::GraphicsError::INITIALIZATION_FAILED;
        }
    }

    m_sizePerBuffer = sizePerBuffer;

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanMultiBuffer::Allocate(VkMemoryPropertyFlags properties) {
    if (m_vkMemory) {
        return Graphics::GraphicsError::OK;
    }
    if (m_vkBuffers.empty()) {
        return Graphics::GraphicsError::OK;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_renderer->GetDevice(), m_vkBuffers[0], &memRequirements);

    // Calculate real size needed by a single buffer
    // Alignment must be a power of 2
    assert((memRequirements.alignment & (memRequirements.alignment - 1)) == 0);
    m_sizePerBuffer = memRequirements.size;
    m_sizePerBuffer = (m_sizePerBuffer + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);

    // Allocate the memory for all buffers
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = m_sizePerBuffer * m_vkBuffers.size();
    if (m_renderer->GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties, 0, &allocInfo.memoryTypeIndex) != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    if (vkAllocateMemory(m_renderer->GetDevice(), &allocInfo, nullptr, &m_vkMemory) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    if (vkMapMemory(m_renderer->GetDevice(), m_vkMemory, 0, allocInfo.allocationSize, 0, &m_mappedMemory) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    // Bind each buffer
    for (size_t i = 0; i < m_vkBuffers.size(); ++i) {
        if (vkBindBufferMemory(m_renderer->GetDevice(), m_vkBuffers[i], m_vkMemory, _calculateOffset(i)) != VK_SUCCESS) {
            return Graphics::GraphicsError::INITIALIZATION_FAILED;
        }
    }

    return Graphics::GraphicsError::OK;
}

VkBuffer VulkanMultiBuffer::GetVkBuffer(size_t index) const {
    return m_vkBuffers[index];
}

size_t VulkanMultiBuffer::GetVkBufferCount() const {
    return m_vkBuffers.size();
}

VkDeviceMemory VulkanMultiBuffer::GetVkDeviceMemory() const {
    return m_vkMemory;
}

void *VulkanMultiBuffer::GetMappedMemory(size_t index) const {
    return reinterpret_cast<uint8_t*>(m_mappedMemory) + _calculateOffset(index);
}

void VulkanMultiBuffer::Clear() {
    if (m_vkMemory) {
        vkFreeMemory(m_renderer->GetDevice(), m_vkMemory, VK_NULL_HANDLE);
        m_vkMemory = VK_NULL_HANDLE;
    }
    for (auto &buffer : m_vkBuffers) {
        vkDestroyBuffer(m_renderer->GetDevice(), buffer, VK_NULL_HANDLE);
    }
    m_vkBuffers.clear();
    m_sizePerBuffer = 0;
    m_mappedMemory = nullptr;
}

VkDeviceSize VulkanMultiBuffer::_calculateOffset(size_t index) const {
    return m_sizePerBuffer * index;
}

} // namespace Vulkan
