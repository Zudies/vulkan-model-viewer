#pragma once

#include "base/PhysicalDevice.h"
#include <vector>

namespace Vulkan {

class APIImpl;

class VulkanPhysicalDevice : public Graphics::PhysicalDevice {
public:
    VulkanPhysicalDevice();
    virtual ~VulkanPhysicalDevice();
    virtual bool SupportsFeature(char const *featureName) const;

    void Initialize(APIImpl *api, VkPhysicalDevice *vkDevice);

private:
    APIImpl *m_api;
    VkPhysicalDevice m_device;
    VkPhysicalDeviceFeatures m_vkFeatures;

    typedef std::vector<VkQueueFamilyProperties> QueueFamilyList;
    QueueFamilyList m_queueFamilies;
};

} // namespace Vulkan
