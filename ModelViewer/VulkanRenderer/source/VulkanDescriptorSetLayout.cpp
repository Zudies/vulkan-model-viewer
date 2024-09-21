#include "pch.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(RendererImpl *renderer)
  : m_renderer(renderer),
    m_vkLayout(VK_NULL_HANDLE) {
    ASSERT(renderer);
    memset(m_key.data(), 0, m_key.size());
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

int VulkanDescriptorSetLayout::_descriptorTypeToKeyIndex(VkDescriptorType descriptorType) {
    switch (descriptorType) {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
        return 0;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return 1;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return 2;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return 3;
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return 4;
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return 5;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return 6;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return 7;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return 8;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return 9;
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return 10;
    case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
        return 11;
    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return 12;
    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
        return 13;
    case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
        return 14;
    default:
        ERROR_MSG(L"Unknown VK descriptor type %u\n", descriptorType);
        return -1;
    }
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

    for (auto &binding : m_bindings) {
        // Assuming no more than 255 bindings of the same type
        ASSERT(binding.descriptorCount < std::numeric_limits<uint8_t>::max());

        // Update the unique key for this layout
        ++m_key[_descriptorTypeToKeyIndex(binding.descriptorType)];

        // Create the memory requirements for this layout
        m_memoryRequirements.push_back({ binding.descriptorType, binding.descriptorCount });
    }

    return Graphics::GraphicsError::OK;
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::GetVkLayout() const {
    return m_vkLayout;
}

const VulkanDescriptorSetLayout::KeyType &VulkanDescriptorSetLayout::GetLayoutKey() const {
    return m_key;
}

const VulkanDescriptorSetLayout::MemoryRequirements &VulkanDescriptorSetLayout::GetMemoryRequirements() const {
    return m_memoryRequirements;
}

const VulkanDescriptorSetLayout::BindingsArray &VulkanDescriptorSetLayout::GetBindings() const {
    return m_bindings;
}

} // namespace Vulkan
