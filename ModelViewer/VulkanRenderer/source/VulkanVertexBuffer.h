#pragma once

#include "VulkanBuffer.h"

namespace Vulkan {

class RendererImpl;
class VulkanCommandBuffer;

template<class VertexType>
class VulkanVertexBuffer {
public:
    VulkanVertexBuffer(RendererImpl *renderer);
    VulkanVertexBuffer(VulkanVertexBuffer const &) = delete;
    VulkanVertexBuffer &operator=(VulkanVertexBuffer const &) = delete;
    ~VulkanVertexBuffer();

    void SetVertexCount(size_t count);
    void *GetVertexData();
    size_t GetVertexCount() const;

    void SetIndexCount(size_t count);
    void *GetIndexData();
    size_t GetIndexCount() const;

    Graphics::GraphicsError FlushVertexToDevice();
    Graphics::GraphicsError FlushIndexToDevice();

    VkVertexInputBindingDescription GetBindingDescription() const;
    const std::vector<VkVertexInputAttributeDescription> &GetAttributeDescription() const;
    VkBuffer GetVertexDeviceBuffer();
    VkBuffer GetIndexDeviceBuffer();

    // Frees host memory usage once no more device flushes are needed
    void ClearHostResources();

private:
    Graphics::GraphicsError _beginTransferCommand(VkDeviceSize size, VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VulkanCommandBuffer *commandBuffer);
    Graphics::GraphicsError _endTransferCommand(VulkanBuffer *stagingBuffer, VulkanCommandBuffer *commandBuffer);
    void _errorTransferCommand(VulkanBuffer *stagingBuffer);

private:
    typedef std::vector<VertexType> VertexData;
    typedef std::vector<uint32_t> IndexData;

    RendererImpl *m_renderer;

    VertexData m_vertexData;
    IndexData m_indexData;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_vertexStagingBuffer;
    VulkanBuffer m_indexBuffer;
    VulkanBuffer m_indexStagingBuffer;
};

} // namespace Vulkan

#include "VulkanVertexBuffer.tpp"
