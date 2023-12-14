#include "pch.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesList.h"

namespace Vulkan {

VulkanPhysicalDevice::VulkanPhysicalDevice()
  : m_api(nullptr),
    m_device(0),
    m_vkProperties({}),
    m_vkFeatures({}) {
}

VulkanPhysicalDevice::~VulkanPhysicalDevice() {
}

bool VulkanPhysicalDevice::SupportsFeature(char const *featureName) const {
    UNUSED_PARAM(featureName);

    // DISCRETE_GPU
    if (strcmp(featureName, FEATURE_IS_DISCRETE_GPU) == 0) {
        // Device must be of type VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        if (m_vkProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            return true;
        }
        return false;
    }

    // GRAPHICS_OPERATIONS
    if (strcmp(featureName, FEATURE_SUPPORTS_GRAPHICS_OPERATIONS) == 0) {
        // Device must have a queue family that supports the VK_QUEUE_GRAPHICS_BIT
        RequiredQueueProperties queueRequirements{ VK_QUEUE_GRAPHICS_BIT };
        auto queueIndex = _getQueueIndex(&queueRequirements);
        if (queueIndex.has_value()) {
            return true;
        }
        return false;
    }

    // Unknown feature
    ERROR_MSG(L"Unknown feature name: %hs", featureName);
    return false;
}

void VulkanPhysicalDevice::Initialize(APIImpl *api, VkPhysicalDevice *vkDevice) {
    ASSERT(api);
    ASSERT(vkDevice);

    m_api = api;
    m_device = *vkDevice;

    vkGetPhysicalDeviceProperties(m_device, &m_vkProperties);
    vkGetPhysicalDeviceFeatures(m_device, &m_vkFeatures);

    LOG_INFO("\t%s (%u) %u\n\t\tVendor: %u\n\t\tApi Version: %u\n\t\tDriver Version: %u\n",
        m_vkProperties.deviceName, m_vkProperties.deviceID, m_vkProperties.deviceType,
        m_vkProperties.vendorID,
        m_vkProperties.apiVersion,
        m_vkProperties.driverVersion
        );

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &queueFamilyCount, nullptr);

    m_queueFamilies.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &queueFamilyCount, m_queueFamilies.data());
}

std::optional<uint32_t> VulkanPhysicalDevice::_getQueueIndex(RequiredQueueProperties *requirements) const {
    std::optional<uint32_t> ret;

    // Find a queue that has all required flags
    for (size_t i = 0; i < m_queueFamilies.size(); ++i) {
        if ((m_queueFamilies[i].queueFlags & requirements->queueFlags) == requirements->queueFlags) {
            ret = static_cast<uint32_t>(i);
            return ret;
        }
    }

    return ret;
}

} // namespace Vulkan
