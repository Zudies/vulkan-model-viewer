#include "pch.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(RendererImpl *renderer)
  : m_renderer(renderer),
    m_vkLayout(VK_NULL_HANDLE) {
    ASSERT(renderer);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
    if (m_vkLayout) {
        vkDestroyDescriptorSetLayout(m_renderer->GetDevice(), m_vkLayout, VK_NULL_HANDLE);
    }
}

void VulkanDescriptorSetLayout::AddUniformBuffer(uint32_t binding, uint32_t count, VkShaderStageFlags shaderStages) {
    auto &newBinding = m_bindings.emplace_back(VkDescriptorSetLayoutBinding{});
    newBinding.binding = binding;
    newBinding.descriptorCount = count;
    newBinding.stageFlags = shaderStages;
    newBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

void VulkanDescriptorSetLayout::AddCombinedImageSampler(uint32_t binding, uint32_t count, VkShaderStageFlags shaderStages) {
    auto &newBinding = m_bindings.emplace_back(VkDescriptorSetLayoutBinding{});
    newBinding.binding = binding;
    newBinding.descriptorCount = count;
    newBinding.stageFlags = shaderStages;
    newBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
}

Graphics::GraphicsError VulkanDescriptorSetLayout::Initialize() {
    ASSERT(!m_vkLayout);

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
    createInfo.pBindings = m_bindings.data();

    if (vkCreateDescriptorSetLayout(m_renderer->GetDevice(), &createInfo, VK_NULL_HANDLE, &m_vkLayout) != VK_SUCCESS) {
        return Graphics::GraphicsError::DESCRIPTOR_SET_CREATE_ERROR;
    }

    // Layout can no longer change so there is no need to keep the bindings anymore
    m_bindings.clear();

    return Graphics::GraphicsError::OK;
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::GetVkLayout() const {
    return m_vkLayout;
}

} // namespace Vulkan
