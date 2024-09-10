#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanBuffer {
public:
    VulkanBuffer(RendererImpl *renderer);
    VulkanBuffer(VulkanBuffer const &) = delete;
    VulkanBuffer &operator=(VulkanBuffer const &) = delete;
    ~VulkanBuffer();

    Graphics::GraphicsError Initialize(VkDeviceSize size, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount);
    Graphics::GraphicsError Allocate(VkMemoryPropertyFlags properties); // No device memory is allocated until this is called

    VkBuffer GetVkBuffer() const;
    VkDeviceMemory GetVkDeviceMemory() const;

    void Clear();

private:
    RendererImpl *m_renderer;
    VkBuffer m_vkBuffer;
    VkDeviceMemory m_vkMemory;
};

} // namespace Vulkan
