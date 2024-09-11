#include "VulkanVertexBuffer.h"

namespace Vulkan {

template<class VertexType>
VulkanVertexBuffer<VertexType>::VulkanVertexBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_vertexBuffer(renderer),
    m_vertexStagingBuffer(renderer),
    m_indexBuffer(renderer),
    m_indexStagingBuffer(renderer) {
    ASSERT(renderer);
}

template<class VertexType>
VulkanVertexBuffer<VertexType>::~VulkanVertexBuffer() {
}

template<class VertexType>
void VulkanVertexBuffer<VertexType>::SetVertexCount(size_t count) {
    m_vertexData.resize(count);

    VkDeviceSize bufferSize = sizeof(VertexType) * count;
    m_vertexBuffer.Clear();

    uint32_t queueFamilies[] = { m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS), m_renderer->GetQueueIndex(RendererImpl::QUEUE_TRANSFER) };
    m_vertexBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, queueFamilies, queueFamilies[0] == queueFamilies[1] ? 1 : 2);
}

template<class VertexType>
void *VulkanVertexBuffer<VertexType>::GetVertexData() {
    return m_vertexData.data();
}

template<class VertexType>
size_t VulkanVertexBuffer<VertexType>::GetVertexCount() const {
    return m_vertexData.size();
}

template<class VertexType>
void VulkanVertexBuffer<VertexType>::SetIndexCount(size_t count) {
    m_indexData.resize(count);

    VkDeviceSize bufferSize = sizeof(uint16_t) * count;
    m_indexBuffer.Clear();

    uint32_t queueFamilies[] = { m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS), m_renderer->GetQueueIndex(RendererImpl::QUEUE_TRANSFER) };
    m_indexBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueFamilies, queueFamilies[0] == queueFamilies[1] ? 1 : 2);
}

template<class VertexType>
void *VulkanVertexBuffer<VertexType>::GetIndexData() {
    return m_indexData.data();
}

template<class VertexType>
size_t VulkanVertexBuffer<VertexType>::GetIndexCount() const {
    return m_indexData.size();
}

template<class VertexType>
Graphics::GraphicsError VulkanVertexBuffer<VertexType>::FlushVertexToDevice() {
    // Allocate memory for the buffer if necessary
    m_vertexBuffer.Allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Allocate a staging buffer
    VkDeviceSize bufferSize = sizeof(VertexType) * m_vertexData.size();
    m_vertexStagingBuffer.Clear();
    m_vertexStagingBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, nullptr, 0);
    m_vertexStagingBuffer.Allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy the host data to staging buffer
    void *data;
    if (vkMapMemory(m_renderer->GetDevice(), m_vertexStagingBuffer.GetVkDeviceMemory(), 0, bufferSize, 0, &data) != VK_SUCCESS) {
        m_vertexStagingBuffer.Clear();
        return Graphics::GraphicsError::TRANSFER_FAILED;
    }
    memcpy(data, m_vertexData.data(), (size_t)bufferSize);
    vkUnmapMemory(m_renderer->GetDevice(), m_vertexStagingBuffer.GetVkDeviceMemory());

    // Register the transfer to run on the next frame start
    m_renderer->RegisterTransfer(
        std::bind(&VulkanVertexBuffer<VertexType>::_beginTransferCommand, this, bufferSize, &m_vertexStagingBuffer, &m_vertexBuffer, std::placeholders::_1),
        std::bind(&VulkanVertexBuffer<VertexType>::_endTransferCommand, this, &m_vertexStagingBuffer)
    );
    
    return Graphics::GraphicsError::OK;
}

template<class VertexType>
Graphics::GraphicsError VulkanVertexBuffer<VertexType>::FlushIndexToDevice() {
    // Allocate memory for the buffer if necessary
    m_indexBuffer.Allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Allocate a staging buffer
    VkDeviceSize bufferSize = sizeof(uint16_t) * m_indexData.size();
    m_indexStagingBuffer.Clear();
    m_indexStagingBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, nullptr, 0);
    m_indexStagingBuffer.Allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy the host data to staging buffer
    void *data;
    if (vkMapMemory(m_renderer->GetDevice(), m_indexStagingBuffer.GetVkDeviceMemory(), 0, bufferSize, 0, &data) != VK_SUCCESS) {
        m_indexStagingBuffer.Clear();
        return Graphics::GraphicsError::TRANSFER_FAILED;
    }
    memcpy(data, m_indexData.data(), (size_t)bufferSize);
    vkUnmapMemory(m_renderer->GetDevice(), m_indexStagingBuffer.GetVkDeviceMemory());

    // Register the transfer to run on the next frame start
    m_renderer->RegisterTransfer(
        std::bind(&VulkanVertexBuffer<VertexType>::_beginTransferCommand, this, bufferSize, &m_indexStagingBuffer, &m_indexBuffer, std::placeholders::_1),
        std::bind(&VulkanVertexBuffer<VertexType>::_endTransferCommand, this, &m_indexStagingBuffer)
    );
    
    return Graphics::GraphicsError::OK;
}

template<class VertexType>
void VulkanVertexBuffer<VertexType>::_beginTransferCommand(VkDeviceSize size, VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkCommandBuffer commandBuffer) {
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer->GetVkBuffer(), dstBuffer->GetVkBuffer(), 1, &copyRegion);
}

template<class VertexType>
void VulkanVertexBuffer<VertexType>::_endTransferCommand(VulkanBuffer *stagingBuffer) {
    stagingBuffer->Clear();
}

template<class VertexType>
VkVertexInputBindingDescription VulkanVertexBuffer<VertexType>::GetBindingDescription() const {
    return VertexType::GetBindingDescription();
}

template<class VertexType>
const std::vector<VkVertexInputAttributeDescription> &VulkanVertexBuffer<VertexType>::GetAttributeDescription() const {
    return VertexType::GetAttributeDescription();
}

template<class VertexType>
VkBuffer VulkanVertexBuffer<VertexType>::GetVertexDeviceBuffer() {
    return m_vertexBuffer.GetVkBuffer();
}

template<class VertexType>
VkBuffer VulkanVertexBuffer<VertexType>::GetIndexDeviceBuffer() {
    return m_indexBuffer.GetVkBuffer();
}

} // namespace Vulkan
