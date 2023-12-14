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
    virtual bool SupportsFeature(char const *featureName) const;

    void Initialize(APIImpl *api, VkPhysicalDevice *vkDevice);

private:
    struct RequiredQueueProperties {
        uint32_t queueFlags;
    };

    std::optional<uint32_t> _getQueueIndex(RequiredQueueProperties *requirements) const;

private:
    APIImpl *m_api;
    VkPhysicalDevice m_device;
    VkPhysicalDeviceProperties m_vkProperties;
    VkPhysicalDeviceFeatures m_vkFeatures;

    typedef std::vector<VkQueueFamilyProperties> QueueFamilyList;
    QueueFamilyList m_queueFamilies;
};

} // namespace Vulkan
