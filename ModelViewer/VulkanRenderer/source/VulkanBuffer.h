#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanBuffer {
public:
    VulkanBuffer(RendererImpl *renderer);
    VulkanBuffer(VulkanBuffer const &) = delete;
    VulkanBuffer &operator=(VulkanBuffer const &) = delete;
    ~VulkanBuffer();

    VulkanBuffer(VulkanBuffer &&other) noexcept;
    VulkanBuffer &operator=(VulkanBuffer &&other) noexcept;

    Graphics::GraphicsError Initialize(VkDeviceSize size, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount);
    Graphics::GraphicsError Allocate(VkMemoryPropertyFlags properties); // No device memory is allocated until this is called

    void *GetMappedMemory() const;
    void UnmapMemory();

    VkBuffer GetVkBuffer() const;
    VkDeviceMemory GetVkDeviceMemory() const;

    void Clear();

private:
    RendererImpl *m_renderer;
    VkBuffer m_vkBuffer;
    VkDeviceMemory m_vkMemory;
    mutable void *m_mappedMemory;
};

} // namespace Vulkan
