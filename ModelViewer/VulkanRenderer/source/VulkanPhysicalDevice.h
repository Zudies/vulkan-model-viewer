#pragma once

#include "base/PhysicalDevice.h"
#include "VulkanSwapChain.h"
#include <vector>
#include <optional>

namespace Graphics {
class RendererRequirements;
}

namespace Vulkan {

class APIImpl;

class VulkanPhysicalDevice : public Graphics::PhysicalDevice {
public:
    VulkanPhysicalDevice();
    virtual ~VulkanPhysicalDevice();
    virtual bool SupportsFeature(char const *featureName, Graphics::RendererRequirements *requirements) const override;
    virtual bool SupportsSurface(int surfaceIndex, Graphics::RendererRequirements *requirements) const override;

    Graphics::GraphicsError Initialize(APIImpl *api, VkPhysicalDevice vkDevice);
    Graphics::GraphicsError Finalize();
    std::optional<uint32_t> GetQueueIndex(Graphics::RendererRequirements *requirements) const;

    struct RequiredQueueProperties {
        uint32_t queueFlags;
        uint32_t notQueueFlags;
        std::optional<VkSurfaceKHR> surfaceSupport;
    };

    std::optional<uint32_t> GetQueueIndex(RequiredQueueProperties *requirements) const;

    VkPhysicalDevice GetDevice() const { return m_device; }

    std::optional<VulkanSwapChain> GetSupportedSurfaceDescription(int surfaceIndex, Graphics::RendererRequirements *requirements) const;

    const VkPhysicalDeviceLimits &GetDeviceLimits() const;

private:
    APIImpl *m_api;
    VkPhysicalDevice m_device;
    VkPhysicalDeviceProperties m_vkProperties;
    VkPhysicalDeviceFeatures m_vkFeatures;

    typedef std::vector<VkExtensionProperties> ExtensionList;
    ExtensionList m_supportedExtensions;

    typedef std::vector<VkQueueFamilyProperties> QueueFamilyList;
    QueueFamilyList m_queueFamilies;
};

} // namespace Vulkan
