#include "pch.h"
#include "VulkanDescriptorSetAllocator.h"
#include "VulkanRendererImpl.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorSetInstance.h"

namespace Vulkan {

VulkanDescriptorSetAllocator::VulkanDescriptorSetAllocator(RendererImpl *renderer)
  : m_renderer(renderer),
    m_expectedAllocs(0) {
    ASSERT(renderer);
}

VulkanDescriptorSetAllocator::~VulkanDescriptorSetAllocator() {
    Clear();
}

void VulkanDescriptorSetAllocator::AddDescriptorLayout(VulkanDescriptorSetLayout *layout, uint32_t allocCount) {
    auto &requirements = layout->GetMemoryRequirements();
    size_t firstIndex = m_descriptorPoolSizes.size();
    m_descriptorPoolSizes.insert(m_descriptorPoolSizes.end(), requirements.begin(), requirements.end());
    m_expectedAllocs += allocCount;

    for (size_t i = firstIndex; i < m_descriptorPoolSizes.size(); ++i) {
        m_descriptorPoolSizes[i].descriptorCount *= allocCount;
    }
}

Graphics::GraphicsError VulkanDescriptorSetAllocator::Initialize() {
    // Skip if pool is not being used
    if (m_expectedAllocs == 0) {
        return Graphics::GraphicsError::OK;
    }

    auto err = _getFreePool();
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanDescriptorSetAllocator::AllocateDescriptorSet(uint32_t count, VulkanDescriptorSetInstance *inOutDescriptorSets) {
    m_setLayouts.resize(count);
    for (size_t i = 0; i < count; ++i) {
        m_setLayouts[i] = inOutDescriptorSets[i].GetDescriptorSetLayout()->GetVkLayout();
    }

    auto err = _allocateVkDescriptorSets(count);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    for (size_t i = 0; i < count; ++i) {
        err = inOutDescriptorSets[i].SetInternalDescriptorSet(m_allocatedSets[i]);
        if (err != Graphics::GraphicsError::OK) {
            return err;
        }
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanDescriptorSetAllocator::AllocateDescriptorSet(uint32_t count, VulkanDescriptorSetInstance **inOutDescriptorSets) {
    m_setLayouts.resize(count);
    for (size_t i = 0; i < count; ++i) {
        m_setLayouts[i] = inOutDescriptorSets[i]->GetDescriptorSetLayout()->GetVkLayout();
    }

    auto err = _allocateVkDescriptorSets(count);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    for (size_t i = 0; i < count; ++i) {
        err = inOutDescriptorSets[i]->SetInternalDescriptorSet(m_allocatedSets[i]);
        if (err != Graphics::GraphicsError::OK) {
            return err;
        }
    }

    return Graphics::GraphicsError::OK;
}

void VulkanDescriptorSetAllocator::Reset() {
    for (auto &pool : m_activePools) {
        vkResetDescriptorPool(m_renderer->GetDevice(), pool, 0);
    }
    m_freePools.insert(m_freePools.end(), m_activePools.begin(), m_activePools.end());
    m_activePools.clear();
}

void VulkanDescriptorSetAllocator::Clear() {
    Reset();
    for (auto &pool : m_freePools) {
        vkDestroyDescriptorPool(m_renderer->GetDevice(), pool, VK_NULL_HANDLE);
    }
    m_freePools.clear();
}

Graphics::GraphicsError VulkanDescriptorSetAllocator::_getFreePool() {
    if (!m_freePools.empty()) {
        m_activePools.emplace_back(m_freePools.back());
        m_freePools.pop_back();
        return Graphics::GraphicsError::OK;
    }

    VkDescriptorPool newPool;
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = m_expectedAllocs;
    createInfo.poolSizeCount = static_cast<uint32_t>(m_descriptorPoolSizes.size());
    createInfo.pPoolSizes = m_descriptorPoolSizes.data();

    if (vkCreateDescriptorPool(m_renderer->GetDevice(), &createInfo, VK_NULL_HANDLE, &newPool) != VK_SUCCESS) {
        return Graphics::GraphicsError::DESCRIPTOR_POOL_CREATE_ERROR;
    }

    m_activePools.emplace_back(newPool);

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanDescriptorSetAllocator::_allocateVkDescriptorSets(uint32_t count) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = m_setLayouts.data();

    m_allocatedSets.resize(count);

    // Try first with the most recent pool
    if (!m_activePools.empty()) {
        allocInfo.descriptorPool = m_activePools.back();
        auto vkError = vkAllocateDescriptorSets(m_renderer->GetDevice(), &allocInfo, m_allocatedSets.data());
        if (vkError == VK_SUCCESS) {
            return Graphics::GraphicsError::OK;
        }
        else if (vkError != VK_ERROR_FRAGMENTED_POOL && vkError != VK_ERROR_OUT_OF_POOL_MEMORY) {
            return Graphics::GraphicsError::DESCRIPTOR_SET_CREATE_ERROR;
        }
    }

    // Try again after fetching a new pool
    auto err = _getFreePool();
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }
    allocInfo.descriptorPool = m_activePools.back();
    if (vkAllocateDescriptorSets(m_renderer->GetDevice(), &allocInfo, m_allocatedSets.data()) != VK_SUCCESS) {
        return Graphics::GraphicsError::DESCRIPTOR_SET_CREATE_ERROR;
    }

    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
