#include "pch.h"
#include "VulkanSampler.h"

#include "VulkanRendererImpl.h"
#include "VulkanFeaturesDefines.h"

namespace Vulkan {

VulkanSampler::VulkanSampler(RendererImpl *renderer)
  : m_renderer(renderer),
    m_sampler(VK_NULL_HANDLE),
    m_samplerProperties{} {
    ASSERT(renderer);

    auto &limits = renderer->GetPhysicalDevice()->GetDeviceLimits();

    m_samplerProperties.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    m_samplerProperties.magFilter = VK_FILTER_LINEAR;
    m_samplerProperties.minFilter = VK_FILTER_LINEAR;
    m_samplerProperties.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    m_samplerProperties.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_samplerProperties.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_samplerProperties.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_samplerProperties.anisotropyEnable = VK_FALSE;
    m_samplerProperties.maxAnisotropy = limits.maxSamplerAnisotropy;
    m_samplerProperties.compareEnable = VK_FALSE;
    m_samplerProperties.compareOp = VK_COMPARE_OP_ALWAYS;
    m_samplerProperties.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    m_samplerProperties.unnormalizedCoordinates = VK_FALSE;
}

VulkanSampler::~VulkanSampler() {
    if (m_sampler) {
        vkDestroySampler(m_renderer->GetDevice(), m_sampler, VK_NULL_HANDLE);
    }
}

void VulkanSampler::SetMagFilter(VkFilter magFilter) {
    m_samplerProperties.magFilter = magFilter;
}

VkFilter VulkanSampler::GetMagFilter() const {
    return m_samplerProperties.magFilter;
}


void VulkanSampler::SetMinFilter(VkFilter minFilter) {
    m_samplerProperties.minFilter = minFilter;
}

VkFilter VulkanSampler::GetMinFilter() const {
    return m_samplerProperties.minFilter;
}


void VulkanSampler::SetMipmapMode(VkSamplerMipmapMode mipmapMode) {
    m_samplerProperties.mipmapMode = mipmapMode;
}


VkSamplerMipmapMode VulkanSampler::GetMipmapMode() const {
    return m_samplerProperties.mipmapMode;
}


void VulkanSampler::SetAddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w) {
    m_samplerProperties.addressModeU = u;
    m_samplerProperties.addressModeV = v;
    m_samplerProperties.addressModeW = w;
}

VkSamplerAddressMode VulkanSampler::GetAddressModeU() const {
    return m_samplerProperties.addressModeU;
}

VkSamplerAddressMode VulkanSampler::GetAddressModeV() const {
    return m_samplerProperties.addressModeV;
}

VkSamplerAddressMode VulkanSampler::GetAddressModeW() const {
    return m_samplerProperties.addressModeW;
}


void VulkanSampler::SetMipmapLod(float mipLodBias, float minLod, float maxLod) {
    m_samplerProperties.mipLodBias = mipLodBias;
    m_samplerProperties.minLod = minLod;
    m_samplerProperties.maxLod = maxLod;
}

float VulkanSampler::GetMipLodBias() const {
    return m_samplerProperties.mipLodBias;
}

float VulkanSampler::GetMinLod() const {
    return m_samplerProperties.minLod;
}

float VulkanSampler::GetMaxLod() const {
    return m_samplerProperties.maxLod;
}

void VulkanSampler::SetAnisotropy(bool anisotropyEnable, float maxAnisotropy) {
    if (anisotropyEnable) {
        // Disallow anisotropy if it's not supported
        if (!m_renderer->GetPhysicalDevice()->SupportsFeature(FEATURE_SAMPLER_ANISOTROPY, m_renderer->GetRequirements())) {
            anisotropyEnable = false;
            LOG_ERROR("Sampler Anisotropy enabled but not supported by device\n");
        }
    }

    m_samplerProperties.anisotropyEnable = anisotropyEnable;
    m_samplerProperties.maxAnisotropy = maxAnisotropy;
}

bool VulkanSampler::GetAnisotropyEnable() const {
    return m_samplerProperties.anisotropyEnable;
}

float VulkanSampler::GetMaxAnisotropy() const {
    return m_samplerProperties.maxAnisotropy;
}

void VulkanSampler::SetCompare(bool compareEnable, VkCompareOp compareOp) {
    m_samplerProperties.compareEnable = compareEnable;
    m_samplerProperties.compareOp = compareOp;
}

bool VulkanSampler::GetCompareEnable() const {
    return m_samplerProperties.compareEnable;
}

VkCompareOp VulkanSampler::GetCompareOp() const {
    return m_samplerProperties.compareOp;
}

void VulkanSampler::SetBorderColor(VkBorderColor borderColor) {
    m_samplerProperties.borderColor = borderColor;
}

VkBorderColor VulkanSampler::GetBorderColor() const {
    return m_samplerProperties.borderColor;
}

void VulkanSampler::SetUnnormalizedCoordinates(bool unnormalizedCoordinates) {
    m_samplerProperties.unnormalizedCoordinates = unnormalizedCoordinates;
}

bool VulkanSampler::GetUnnormalizedCoordinates() const {
    return m_samplerProperties.unnormalizedCoordinates;
}

Graphics::GraphicsError VulkanSampler::Initialize() {
    if (vkCreateSampler(m_renderer->GetDevice(), &m_samplerProperties, VK_NULL_HANDLE, &m_sampler) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    return Graphics::GraphicsError::OK;
}

VkSampler VulkanSampler::GetVkSampler() const {
    return m_sampler;
}

} // namespace Vulkan
