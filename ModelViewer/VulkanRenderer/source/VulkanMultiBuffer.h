#pragma once

namespace Vulkan {

class RendererImpl;

// Multiple VulkanBuffers bound to a single memory allocation
class VulkanMultiBuffer {
public:
    VulkanMultiBuffer(RendererImpl *renderer);
    VulkanMultiBuffer(VulkanMultiBuffer const &) = delete;
    VulkanMultiBuffer &operator=(VulkanMultiBuffer const &) = delete;
    ~VulkanMultiBuffer();

    Graphics::GraphicsError Initialize(VkDeviceSize sizePerBuffer, size_t bufferCount, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount);
    Graphics::GraphicsError Allocate(VkMemoryPropertyFlags properties); // No device memory is allocated until this is called

    VkBuffer GetVkBuffer(size_t index) const;
    size_t GetVkBufferCount() const;
    VkDeviceMemory GetVkDeviceMemory() const;
    void *GetMappedMemory(size_t index) const;

    void Clear();

private:
    VkDeviceSize _calculateOffset(size_t index) const;

private:
    RendererImpl *m_renderer;

    typedef std::vector<VkBuffer> BufferArray;
    VkDeviceSize m_sizePerBuffer;
    BufferArray m_vkBuffers;
    VkDeviceMemory m_vkMemory;
    void *m_mappedMemory;
};

} // namespace Vulkan
