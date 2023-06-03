#include "pch.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanAPIImpl.h"
#include "FeaturesList.h"

namespace Vulkan {

VulkanPhysicalDevice::VulkanPhysicalDevice()
  : m_api(nullptr),
    m_device(0),
    m_vkFeatures({}) {
}

VulkanPhysicalDevice::~VulkanPhysicalDevice() {
}

bool VulkanPhysicalDevice::SupportsFeature(char const *featureName) const {
    UNUSED_PARAM(featureName);

    // Device supports graphics commands
    if (strcmp(featureName, Graphics::FEATURE_SUPPORTS_GRAPHICS_OPERATIONS) == 0) {
        // Device must have a queue family that supports the VK_QUEUE_GRAPHICS_BIT
        for (auto &it : m_queueFamilies) {
            if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                return true;
            }
        }
    }

    return false;
}

void VulkanPhysicalDevice::Initialize(APIImpl *api, VkPhysicalDevice *vkDevice) {
    ASSERT(api);
    ASSERT(vkDevice);

    m_api = api;
    m_device = *vkDevice;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_device, &deviceProperties);

    vkGetPhysicalDeviceFeatures(m_device, &m_vkFeatures);

    LOG_INFO("\t%s (%u) %u\n\t\tVendor: %u\n\t\tApi Version: %u\n\t\tDriver Version: %u\n",
        deviceProperties.deviceName, deviceProperties.deviceID, deviceProperties.deviceType,
        deviceProperties.vendorID,
        deviceProperties.apiVersion,
        deviceProperties.driverVersion
        );

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &queueFamilyCount, nullptr);

    m_queueFamilies.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &queueFamilyCount, m_queueFamilies.data());
}

} // namespace Vulkan
