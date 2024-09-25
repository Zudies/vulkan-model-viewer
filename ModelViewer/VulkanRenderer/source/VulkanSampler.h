#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanSampler {
public:

    VulkanSampler(RendererImpl *renderer);
    VulkanSampler(VulkanSampler const &) = delete;
    VulkanSampler &operator=(VulkanSampler const &) = delete;
    ~VulkanSampler();

    VulkanSampler(VulkanSampler &&other) noexcept;
    VulkanSampler &operator=(VulkanSampler &&other) noexcept;

    // Default: VK_FILTER_LINEAR
    void SetMagFilter(VkFilter magFilter);
    VkFilter GetMagFilter() const;

    // Default: VK_FILTER_LINEAR
    void SetMinFilter(VkFilter minFilter);
    VkFilter GetMinFilter() const;

    // Default: VK_SAMPLER_MIPMAP_MODE_LINEAR
    void SetMipmapMode(VkSamplerMipmapMode mipmapMode);
    VkSamplerMipmapMode GetMipmapMode() const;

    // Default: VK_SAMPLER_ADDRESS_MODE_REPEAT
    void SetAddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
    VkSamplerAddressMode GetAddressModeU() const;
    VkSamplerAddressMode GetAddressModeV() const;
    VkSamplerAddressMode GetAddressModeW() const;

    // Default: 0.0f
    void SetMipmapLod(float mipLodBias, float minLod, float maxLod);
    float GetMipLodBias() const;
    float GetMinLod() const;
    float GetMaxLod() const;

    // Default: false, physical device limit maxSamplerAnisotropy
    void SetAnisotropy(bool anisotropyEnable, float maxAnisotropy);
    bool GetAnisotropyEnable() const;
    float GetMaxAnisotropy() const;

    // Default: false, VK_COMPARE_OP_ALWAYS
    void SetCompare(bool compareEnable, VkCompareOp compareOp);
    bool GetCompareEnable() const;
    VkCompareOp GetCompareOp() const;

    // Default: VK_BORDER_COLOR_INT_OPAQUE_BLACK
    void SetBorderColor(VkBorderColor borderColor);
    VkBorderColor GetBorderColor() const;

    // Default: false
    void SetUnnormalizedCoordinates(bool unnormalizedCoordinates);
    bool GetUnnormalizedCoordinates() const;

    Graphics::GraphicsError Initialize();

    VkSampler GetVkSampler() const;

private:

    RendererImpl *m_renderer;
    VkSampler m_sampler;
    VkSamplerCreateInfo m_samplerProperties;

};

} // namespace Vulkan
