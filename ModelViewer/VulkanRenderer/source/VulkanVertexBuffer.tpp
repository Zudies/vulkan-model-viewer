#include "VulkanVertexBuffer.h"
#include "VulkanCommandBuffer.h"

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

    m_vertexBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, nullptr, 0);
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

    VkDeviceSize bufferSize = sizeof(IndexData::value_type) * count;
    m_indexBuffer.Clear();

    m_indexBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, nullptr, 0);
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
    auto err = m_vertexBuffer.Allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    // Allocate a staging buffer
    VkDeviceSize bufferSize = sizeof(VertexType) * m_vertexData.size();
    m_vertexStagingBuffer.Clear();
    m_vertexStagingBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, nullptr, 0);
    m_vertexStagingBuffer.Allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy the host data to staging buffer
    void *mappedMemory = m_vertexStagingBuffer.GetMappedMemory();
    if (!mappedMemory) {
        m_vertexStagingBuffer.Clear();
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    memcpy(mappedMemory, m_vertexData.data(), bufferSize);

    //TODO: If this needs to update frequently, don't unmap
    m_vertexStagingBuffer.UnmapMemory();

    // Register the transfer to run on the next frame update
    m_renderer->RegisterTransfer(
        RendererImpl::QUEUE_GRAPHICS,
        1,
        std::bind(&VulkanVertexBuffer<VertexType>::_beginTransferCommand, this, bufferSize, &m_vertexStagingBuffer, &m_vertexBuffer, std::placeholders::_1),
        std::bind(&VulkanVertexBuffer<VertexType>::_endTransferCommand, this, &m_vertexStagingBuffer, std::placeholders::_1),
        std::bind(&VulkanVertexBuffer<VertexType>::_errorTransferCommand, this, &m_vertexStagingBuffer)
    );
    
    return Graphics::GraphicsError::OK;
}

template<class VertexType>
Graphics::GraphicsError VulkanVertexBuffer<VertexType>::FlushIndexToDevice() {
    // Allocate memory for the buffer if necessary
    auto err = m_indexBuffer.Allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    // Allocate a staging buffer
    VkDeviceSize bufferSize = sizeof(IndexData::value_type) * m_indexData.size();
    m_indexStagingBuffer.Clear();
    m_indexStagingBuffer.Initialize(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, nullptr, 0);
    m_indexStagingBuffer.Allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy the host data to staging buffer
    void *mappedMemory = m_indexStagingBuffer.GetMappedMemory();
    if (!mappedMemory) {
        m_indexStagingBuffer.Clear();
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    memcpy(mappedMemory, m_indexData.data(), bufferSize);
    m_indexStagingBuffer.UnmapMemory();

    // Register the transfer to run on the next frame update
    m_renderer->RegisterTransfer(
        RendererImpl::QUEUE_GRAPHICS,
        1,
        std::bind(&VulkanVertexBuffer<VertexType>::_beginTransferCommand, this, bufferSize, &m_indexStagingBuffer, &m_indexBuffer, std::placeholders::_1),
        std::bind(&VulkanVertexBuffer<VertexType>::_endTransferCommand, this, &m_indexStagingBuffer, std::placeholders::_1),
        std::bind(&VulkanVertexBuffer<VertexType>::_errorTransferCommand, this, &m_indexStagingBuffer)
    );
    
    return Graphics::GraphicsError::OK;
}

template<class VertexType>
Graphics::GraphicsError VulkanVertexBuffer<VertexType>::_beginTransferCommand(VkDeviceSize size, VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VulkanCommandBuffer *commandBuffer) {
    auto err = commandBuffer->BeginCommandBuffer();
    if (err != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"Failed to copy vertex buffer to device\n");
        srcBuffer->Clear();
        return err;
    }

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer->GetVkCommandBuffer(), srcBuffer->GetVkBuffer(), dstBuffer->GetVkBuffer(), 1, &copyRegion);

    err = commandBuffer->EndCommandBuffer();
    if (err != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"Failed to copy vertex buffer to device\n");
        srcBuffer->Clear();
        return err;
    }

    err = commandBuffer->Submit();
    if (err != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"Failed to copy vertex buffer to device\n");
        srcBuffer->Clear();
        return err;
    }

    return Graphics::GraphicsError::OK;
}

template<class VertexType>
Graphics::GraphicsError VulkanVertexBuffer<VertexType>::_endTransferCommand(VulkanBuffer *stagingBuffer, VulkanCommandBuffer *commandBuffer) {
    VkFence fence = commandBuffer->GetWaitFence();
    vkWaitForFences(m_renderer->GetDevice(), 1, &fence, true, std::numeric_limits<uint64_t>::max());
    stagingBuffer->Clear();
    return Graphics::GraphicsError::OK;
}

template<class VertexType>
void VulkanVertexBuffer<VertexType>::_errorTransferCommand(VulkanBuffer *stagingBuffer) {
    LOG_ERROR(L"Failed to copy vertex buffer to device\n");
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

template<class VertexType>
void VulkanVertexBuffer<VertexType>::ClearHostResources() {
    m_vertexData.clear();
    m_indexData.clear();
}

} // namespace Vulkan
