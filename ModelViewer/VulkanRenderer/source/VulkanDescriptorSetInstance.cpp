#include "pch.h"
#include "VulkanDescriptorSetInstance.h"
#include "VulkanRendererImpl.h"
#include "VulkanDescriptorSetLayout.h"

namespace Vulkan {

VulkanDescriptorSetInstance::VulkanDescriptorSetInstance(RendererImpl *renderer)
  : m_renderer(renderer),
    m_vkDescriptorSet(VK_NULL_HANDLE),
    m_descriptorSetLayout(nullptr) {
    ASSERT(renderer);
}

VulkanDescriptorSetInstance::~VulkanDescriptorSetInstance() {
    for (auto *info : m_writeInfos) {
        delete[] info;
    }
}

void VulkanDescriptorSetInstance::SetDescriptorSetLayout(VulkanDescriptorSetLayout *layout) {
    m_descriptorSetLayout = layout;

    m_writeDescriptors.clear();
    auto &layoutBindings = m_descriptorSetLayout->GetBindings();

    for (auto &binding : layoutBindings) {
        auto &newWriteDescriptor = m_writeDescriptors.emplace_back(VkWriteDescriptorSet{});
        newWriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        newWriteDescriptor.dstBinding = binding.binding;
        newWriteDescriptor.dstArrayElement = 0;
        newWriteDescriptor.descriptorCount = 1;
        newWriteDescriptor.descriptorType = binding.descriptorType;
    }
}

#pragma warning( push )
#pragma warning( disable : 6011 )
void VulkanDescriptorSetInstance::AddDescriptorWrite(uint32_t binding, const VkDescriptorBufferInfo *bufferInfo, uint32_t arrayElement, uint32_t descriptorCount) {
    auto *layoutBinding = _findBinding(binding);
    ASSERT(layoutBinding);

    auto newInfo = m_writeInfos.emplace_back(new uint8_t[sizeof(VkDescriptorBufferInfo) * descriptorCount]);
    memcpy(newInfo, bufferInfo, sizeof(VkDescriptorBufferInfo) * descriptorCount);

    auto &newWriteDescriptor = m_writeDescriptors.emplace_back(VkWriteDescriptorSet{});
    newWriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWriteDescriptor.dstBinding = layoutBinding->binding;
    newWriteDescriptor.dstArrayElement = arrayElement;
    newWriteDescriptor.descriptorCount = descriptorCount;
    newWriteDescriptor.descriptorType = layoutBinding->descriptorType;
    newWriteDescriptor.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(newInfo);
}

void VulkanDescriptorSetInstance::AddDescriptorWrite(uint32_t binding, const VkDescriptorImageInfo *imageInfo, uint32_t arrayElement, uint32_t descriptorCount) {
    auto *layoutBinding = _findBinding(binding);
    ASSERT(layoutBinding);

    auto newInfo = m_writeInfos.emplace_back(new uint8_t[sizeof(VkDescriptorImageInfo) * descriptorCount]);
    memcpy(newInfo, imageInfo, sizeof(VkDescriptorImageInfo) * descriptorCount);

    auto &newWriteDescriptor = m_writeDescriptors.emplace_back(VkWriteDescriptorSet{});
    newWriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWriteDescriptor.dstBinding = layoutBinding->binding;
    newWriteDescriptor.dstArrayElement = arrayElement;
    newWriteDescriptor.descriptorCount = descriptorCount;
    newWriteDescriptor.descriptorType = layoutBinding->descriptorType;
    newWriteDescriptor.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(newInfo);
}

void VulkanDescriptorSetInstance::AddDescriptorWrite(uint32_t binding, const VkBufferView *bufferViewInfo, uint32_t arrayElement, uint32_t descriptorCount) {
    auto *layoutBinding = _findBinding(binding);
    ASSERT(layoutBinding);

    auto newInfo = m_writeInfos.emplace_back(new uint8_t[sizeof(VkBufferView) * descriptorCount]);
    memcpy(newInfo, bufferViewInfo, sizeof(VkBufferView) * descriptorCount);

    auto &newWriteDescriptor = m_writeDescriptors.emplace_back(VkWriteDescriptorSet{});
    newWriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWriteDescriptor.dstBinding = layoutBinding->binding;
    newWriteDescriptor.dstArrayElement = arrayElement;
    newWriteDescriptor.descriptorCount = descriptorCount;
    newWriteDescriptor.descriptorType = layoutBinding->descriptorType;
    newWriteDescriptor.pTexelBufferView = reinterpret_cast<VkBufferView*>(newInfo);
}
#pragma warning( pop ) 

void VulkanDescriptorSetInstance::UpdateDescriptorWrite(uint32_t writeIndex, const VkDescriptorBufferInfo *bufferInfo, uint32_t arrayElement, uint32_t descriptorCount) {
    auto newInfo = m_writeInfos.emplace_back(new uint8_t[sizeof(VkDescriptorBufferInfo) * descriptorCount]);
    memcpy(newInfo, bufferInfo, sizeof(VkDescriptorBufferInfo) * descriptorCount);

    auto &writeDescriptor = m_writeDescriptors[writeIndex];
    writeDescriptor.dstArrayElement = arrayElement;
    writeDescriptor.descriptorCount = descriptorCount;
    writeDescriptor.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(newInfo);
}

void VulkanDescriptorSetInstance::UpdateDescriptorWrite(uint32_t writeIndex, const VkDescriptorImageInfo *imageInfo, uint32_t arrayElement, uint32_t descriptorCount) {
    auto newInfo = m_writeInfos.emplace_back(new uint8_t[sizeof(VkDescriptorImageInfo) * descriptorCount]);
    memcpy(newInfo, imageInfo, sizeof(VkDescriptorImageInfo) * descriptorCount);

    auto &writeDescriptor = m_writeDescriptors[writeIndex];
    writeDescriptor.dstArrayElement = arrayElement;
    writeDescriptor.descriptorCount = descriptorCount;
    writeDescriptor.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(newInfo);
}

void VulkanDescriptorSetInstance::UpdateDescriptorWrite(uint32_t writeIndex, const VkBufferView *bufferViewInfo, uint32_t arrayElement, uint32_t descriptorCount) {
    auto newInfo = m_writeInfos.emplace_back(new uint8_t[sizeof(VkBufferView) * descriptorCount]);
    memcpy(newInfo, bufferViewInfo, sizeof(VkBufferView) * descriptorCount);

    auto &writeDescriptor = m_writeDescriptors[writeIndex];
    writeDescriptor.dstArrayElement = arrayElement;
    writeDescriptor.descriptorCount = descriptorCount;
    writeDescriptor.pTexelBufferView = reinterpret_cast<VkBufferView*>(newInfo);
}

Graphics::GraphicsError VulkanDescriptorSetInstance::SetInternalDescriptorSet(VkDescriptorSet vkDescriptorSet) {
    m_vkDescriptorSet = vkDescriptorSet;

    for (auto &writeDescriptor : m_writeDescriptors) {
        writeDescriptor.dstSet = m_vkDescriptorSet;
    }

    vkUpdateDescriptorSets(m_renderer->GetDevice(), static_cast<uint32_t>(m_writeDescriptors.size()), m_writeDescriptors.data(), 0, VK_NULL_HANDLE);

    return Graphics::GraphicsError::OK;
}

VulkanDescriptorSetLayout *VulkanDescriptorSetInstance::GetDescriptorSetLayout() const {
    return m_descriptorSetLayout;
}

const VkDescriptorSet &VulkanDescriptorSetInstance::GetVkDescriptorSet() const {
    return m_vkDescriptorSet;
}

const VkDescriptorSetLayoutBinding *VulkanDescriptorSetInstance::_findBinding(uint32_t binding) {
    auto &layoutBindings = m_descriptorSetLayout->GetBindings();
    auto foundBinding = std::find_if(layoutBindings.begin(), layoutBindings.end(),
        [binding](auto &layoutBinding) {
            return layoutBinding.binding == binding;
        });

    if (foundBinding != layoutBindings.end()) {
        return &*foundBinding;
    }
    return nullptr;
}

} // namespace Vulkan
