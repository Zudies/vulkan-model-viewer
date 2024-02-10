#pragma once

#include "base/PhysicalDevice.h"
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

    Graphics::GraphicsError Initialize(APIImpl *api, VkPhysicalDevice vkDevice);
    Graphics::GraphicsError Finalize();
    std::optional<uint32_t> GetQueueIndex(Graphics::RendererRequirements *requirements) const;

    struct RequiredQueueProperties {
        uint32_t queueFlags;
        std::optional<VkSurfaceKHR> surfaceSupport;
    };

    std::optional<uint32_t> GetQueueIndex(RequiredQueueProperties *requirements) const;

    VkPhysicalDevice GetDevice() const { return m_device; }

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
