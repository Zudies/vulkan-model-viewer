#pragma once

#include "BitFlag.h"

namespace Vulkan {

class RendererImpl;

class VulkanCommandBuffer {
public:
    VulkanCommandBuffer(RendererImpl *renderer);
    VulkanCommandBuffer(VulkanCommandBuffer const &) = delete;
    VulkanCommandBuffer &operator=(VulkanCommandBuffer const &) = delete;
    ~VulkanCommandBuffer();

    VulkanCommandBuffer(VulkanCommandBuffer &&other);
    VulkanCommandBuffer &operator=(VulkanCommandBuffer &&other);

    // Default: Not single use
    void SetSingleUse(bool singleUse);

    // Default: Primary buffer
    void SetLevel(VkCommandBufferLevel level);

    // If useFence is true, Submit() will include a fence that can be waited on
    // If useFence is true and the provided fence is a null pointer, then the command buffer will maintain its own fence
    // If Submit() is not called, then the fence will not be automatically signaled
    void SetWaitFence(bool useFence, VkFence fence);

    // Passing null pointer will cause this to maintain its own command buffer internally
    // queue must be of value QueueType
    Graphics::GraphicsError Initialize(uint32_t queue, VkCommandBuffer vkCommandBuffer);

    // Reset the command buffer but maintain any settings prior to Initialize()
    void Clear();

    // Command buffer submission
    void AddSignalSemaphore(VkSemaphore semaphore);
    void AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage);
    VkSubmitInfo MakeSubmitInfo();

    // Submits using the same submitInfo from MakeSubmitInfo
    Graphics::GraphicsError Submit();

    // Single use bit will be set automatically if needed
    // Any other desired flag should be passed through flags
    Graphics::GraphicsError BeginCommandBuffer(VkCommandBufferUsageFlagBits flags = static_cast<VkCommandBufferUsageFlagBits>(0));
    Graphics::GraphicsError EndCommandBuffer();
    void ResetWaitFence();

    VkCommandBuffer GetVkCommandBuffer() const;
    VkFence GetWaitFence() const;
    uint32_t GetQueue() const;

private:
    RendererImpl *m_renderer;
    VkCommandBuffer m_commandBuffer;
    VkFence m_fence;

    std::vector<VkSemaphore> *m_waitSemaphores;
    std::vector<VkPipelineStageFlags> *m_waitSemaphoreStages;
    std::vector<VkSemaphore> *m_signalSemaphores;

    Graphics::BitFlag<uint32_t> m_flags;
    uint32_t m_queue;

};

} // namespace Vulkan
