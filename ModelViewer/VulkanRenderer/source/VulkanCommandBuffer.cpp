#include "pch.h"
#include "VulkanCommandBuffer.h"

#include "VulkanRendererImpl.h"

namespace Vulkan {

// Local flags to control command buffer state
static const uint32_t COMMAND_BUFFER_IS_SINGLE_USE       = 0x0001;
static const uint32_t COMMAND_BUFFER_OWNS_COMMAND_BUFFER = 0x0002;
static const uint32_t COMMAND_BUFFER_OWNS_FENCE          = 0x0004;
static const uint32_t COMMAND_BUFFER_IS_SECONDARY_BUFFER = 0x0008;

VulkanCommandBuffer::VulkanCommandBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_commandBuffer(VK_NULL_HANDLE), 
    m_fence(VK_NULL_HANDLE),
    m_fenceCreateFlags(0),
    m_waitSemaphores(nullptr),
    m_waitSemaphoreStages(nullptr),
    m_signalSemaphores(nullptr),
    m_queue(static_cast<uint32_t>(-1)) {
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
    Clear();
    if (m_flags.GetFlag(COMMAND_BUFFER_OWNS_FENCE) && m_fence) {
        vkDestroyFence(m_renderer->GetDevice(), m_fence, VK_NULL_HANDLE);
    }

    delete m_waitSemaphores;
    delete m_waitSemaphoreStages;
    delete m_signalSemaphores;
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer &&other) noexcept
  : m_renderer(other.m_renderer),
    m_commandBuffer(other.m_commandBuffer),
    m_fence(other.m_fence),
    m_fenceCreateFlags(other.m_fenceCreateFlags),
    m_waitSemaphores(other.m_waitSemaphores),
    m_waitSemaphoreStages(other.m_waitSemaphoreStages),
    m_signalSemaphores(other.m_signalSemaphores),
    m_flags(other.m_flags),
    m_queue(other.m_queue) {

    other.m_flags = Graphics::BitFlag<uint32_t>();
    other.m_waitSemaphores = nullptr;
    other.m_waitSemaphoreStages = nullptr;
    other.m_signalSemaphores = nullptr;
}

VulkanCommandBuffer &VulkanCommandBuffer::operator=(VulkanCommandBuffer &&other) noexcept {
    m_renderer = other.m_renderer;
    m_commandBuffer = other.m_commandBuffer;
    m_fence = other.m_fence;
    m_fenceCreateFlags = other.m_fenceCreateFlags;
    m_waitSemaphores = other.m_waitSemaphores;
    m_waitSemaphoreStages = other.m_waitSemaphoreStages;
    m_signalSemaphores = other.m_signalSemaphores;
    m_flags = other.m_flags;
    m_queue = other.m_queue;

    other.m_flags = Graphics::BitFlag<uint32_t>();
    other.m_waitSemaphores = nullptr;
    other.m_waitSemaphoreStages = nullptr;
    other.m_signalSemaphores = nullptr;

    return *this;
}

void VulkanCommandBuffer::SetSingleUse(bool singleUse) {
    m_flags.SetFlag(COMMAND_BUFFER_IS_SINGLE_USE, singleUse);
}

void VulkanCommandBuffer::SetLevel(VkCommandBufferLevel level) {
    m_flags.SetFlag(COMMAND_BUFFER_IS_SECONDARY_BUFFER, level & VK_COMMAND_BUFFER_LEVEL_SECONDARY);
}

void VulkanCommandBuffer::SetWaitFence(bool useFence, VkFence fence, VkFenceCreateFlags fenceFlags) {
    if (useFence) {
        if (fence) {
            // Clear own fence if needed
            if (m_flags.GetFlag(COMMAND_BUFFER_OWNS_FENCE) && m_fence) {
                vkDestroyFence(m_renderer->GetDevice(), m_fence, VK_NULL_HANDLE);
            }
            m_flags.ClearFlag(COMMAND_BUFFER_OWNS_FENCE);
            m_fence = fence;
        }
        else {
            // Create fence if we don't already own one
            m_fenceCreateFlags = fenceFlags;
            if (!m_flags.GetFlag(COMMAND_BUFFER_OWNS_FENCE) || !m_fence) {
                m_flags.SetFlag(COMMAND_BUFFER_OWNS_FENCE);
                m_fence = VK_NULL_HANDLE; // Fence creation will happen in Initialize()
            }
        }
    }
    else {
        if (m_flags.GetFlag(COMMAND_BUFFER_OWNS_FENCE) && m_fence) {
            vkDestroyFence(m_renderer->GetDevice(), m_fence, VK_NULL_HANDLE);
        }
        m_flags.ClearFlag(COMMAND_BUFFER_OWNS_FENCE);
        m_fence = VK_NULL_HANDLE;
    }
}

Graphics::GraphicsError VulkanCommandBuffer::Initialize(uint32_t queue, VkCommandBuffer vkCommandBuffer) {
    ASSERT(!m_commandBuffer);

    m_queue = queue;

    // Determine if we need to allocate a command buffer
    if (vkCommandBuffer) {
        m_commandBuffer = vkCommandBuffer;
        m_flags.ClearFlag(COMMAND_BUFFER_OWNS_COMMAND_BUFFER);
    }
    else {
        auto err = m_renderer->AllocateCommandBuffers(
            static_cast<RendererImpl::QueueType>(m_queue),
            m_flags.GetFlag(COMMAND_BUFFER_IS_SECONDARY_BUFFER) ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1,
            &m_commandBuffer
        );
        if (err != Graphics::GraphicsError::OK) {
            m_flags.ClearFlag(COMMAND_BUFFER_OWNS_COMMAND_BUFFER);
            return err;
        }
        m_flags.SetFlag(COMMAND_BUFFER_OWNS_COMMAND_BUFFER);
    }

    // Determine if we need to allocate a fence
    if (m_flags.GetFlag(COMMAND_BUFFER_OWNS_FENCE) && !m_fence) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = m_fenceCreateFlags;
        if (vkCreateFence(m_renderer->GetDevice(), &fenceInfo, VK_NULL_HANDLE, &m_fence) != VK_SUCCESS) {
            Clear();
            return Graphics::GraphicsError::INITIALIZATION_FAILED;
        }
    }

    return Graphics::GraphicsError::OK;
}

void VulkanCommandBuffer::Clear() {
    if (m_flags.GetFlag(COMMAND_BUFFER_OWNS_COMMAND_BUFFER)) {
        vkFreeCommandBuffers(m_renderer->GetDevice(), m_renderer->m_commandPools[m_queue], 1, &m_commandBuffer);
    }
    m_flags.ClearFlag(COMMAND_BUFFER_OWNS_COMMAND_BUFFER);
    m_commandBuffer = VK_NULL_HANDLE;
    m_queue = static_cast<uint32_t>(-1);

    if (m_waitSemaphores) {
        m_waitSemaphores->clear();
        m_waitSemaphoreStages->clear();
    }
    if (m_signalSemaphores) {
        m_signalSemaphores->clear();
    }
}

void VulkanCommandBuffer::AddSignalSemaphore(VkSemaphore semaphore) {
    if (!m_signalSemaphores) {
        m_signalSemaphores = new std::vector<VkSemaphore>;
    }
    m_signalSemaphores->emplace_back(semaphore);
}

void VulkanCommandBuffer::AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage) {
    if (!m_waitSemaphores) {
        m_waitSemaphores = new std::vector<VkSemaphore>;
        m_waitSemaphoreStages = new std::vector<VkPipelineStageFlags>;
    }
    m_waitSemaphores->emplace_back(semaphore);
    m_waitSemaphoreStages->emplace_back(waitStage);
}

VkSubmitInfo VulkanCommandBuffer::MakeSubmitInfo() {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    if (m_waitSemaphores) {
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores->size());
        submitInfo.pWaitSemaphores = m_waitSemaphores->data();
        submitInfo.pWaitDstStageMask = m_waitSemaphoreStages->data();
    }
    if (m_signalSemaphores) {
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(m_signalSemaphores->size());
        submitInfo.pSignalSemaphores = m_signalSemaphores->data();
    }

    return submitInfo;
}

Graphics::GraphicsError VulkanCommandBuffer::Submit() {
    VkSubmitInfo submitInfo = MakeSubmitInfo();

    if (m_fence) {
        vkResetFences(m_renderer->GetDevice(), 1, &m_fence);
    }
    auto vkErr = vkQueueSubmit(m_renderer->GetQueue(static_cast<RendererImpl::QueueType>(m_queue)), 1, &submitInfo, m_fence);
    if (vkErr == VK_ERROR_DEVICE_LOST) {
        return Graphics::GraphicsError::DEVICE_LOST;
    }
    else if (vkErr != VK_SUCCESS) {
        return Graphics::GraphicsError::QUEUE_ERROR;
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanCommandBuffer::BeginCommandBuffer(VkCommandBufferUsageFlagBits flags) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    if (m_flags.GetFlag(COMMAND_BUFFER_IS_SINGLE_USE)) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
        return Graphics::GraphicsError::QUEUE_ERROR;
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanCommandBuffer::EndCommandBuffer() {
    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
        return Graphics::GraphicsError::QUEUE_ERROR;
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanCommandBuffer::ResetCommandBuffer(VkCommandBufferResetFlags flags) {
    vkResetCommandBuffer(m_commandBuffer, flags);
    if (m_waitSemaphores) {
        m_waitSemaphores->clear();
        m_waitSemaphoreStages->clear();
    }
    if (m_signalSemaphores) {
        m_signalSemaphores->clear();
    }
    return Graphics::GraphicsError::OK;
}

void VulkanCommandBuffer::ResetWaitFence() {
    if (m_fence) {
        vkResetFences(m_renderer->GetDevice(), 1, &m_fence);
    }
}

VkCommandBuffer VulkanCommandBuffer::GetVkCommandBuffer() const {
    return m_commandBuffer;
}

const VkFence &VulkanCommandBuffer::GetWaitFence() const {
    return m_fence;
}

uint32_t VulkanCommandBuffer::GetQueue() const {
    return m_queue;
}

} // namespace Vulkan
