#include "pch.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesList.h"
#include "base/RendererRequirements_Base.h"

namespace Vulkan {

VulkanPhysicalDevice::VulkanPhysicalDevice()
    : m_api(nullptr),
    m_device(0),
    m_vkProperties({}),
    m_vkFeatures({}) {
}

VulkanPhysicalDevice::~VulkanPhysicalDevice() {
}

bool VulkanPhysicalDevice::SupportsFeature(char const *featureName, Graphics::RendererRequirements *requirements) const {
    // DISCRETE_GPU
    if (strcmp(featureName, FEATURE_IS_DISCRETE_GPU) == 0) {
        // Device must be of type VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        return m_vkProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    }

    // GRAPHICS_OPERATIONS
    if (strcmp(featureName, FEATURE_SUPPORTS_GRAPHICS_OPERATIONS) == 0) {
        // Device must have a queue family that supports the VK_QUEUE_GRAPHICS_BIT
        RequiredQueueProperties queueRequirements{ VK_QUEUE_GRAPHICS_BIT };
        auto queueIndex = GetQueueIndex(&queueRequirements);
        return queueIndex.has_value();
    }

    // SURFACE_WINDOW_PRESENT
    if (strcmp(featureName, FEATURE_SURFACE_WINDOW_PRESENT) == 0) {
        // Check that the device can present to a surface
        auto vkSurface = m_api->GetWindowSurface(requirements->GetWindowSurface(0));
        if (!vkSurface || vkSurface.value() == nullptr) {
            return false;
        }
        RequiredQueueProperties queueRequirements{ {}, vkSurface };
        auto queueIndex = GetQueueIndex(&queueRequirements);
        if (!queueIndex) {
            return false;
        }

        // Check that the device supports swapchain
        return std::find_if(m_supportedExtensions.begin(), m_supportedExtensions.end(),
                [](VkExtensionProperties const &ext) {
                    return strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
            }) != m_supportedExtensions.end();
    }

    // Unknown feature
    ERROR_MSG(L"Unknown feature name: %hs", featureName);
    return false;
}

Graphics::GraphicsError VulkanPhysicalDevice::Initialize(APIImpl *api, VkPhysicalDevice vkDevice) {
    ASSERT(api);

    m_api = api;
    m_device = vkDevice;

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

    uint32_t extensionCount;
    VkResult vkResult = vkEnumerateDeviceExtensionProperties(m_device, nullptr, &extensionCount, nullptr);
    if (vkResult != VK_SUCCESS) {
        LOG_ERROR("vkEnumerateDeviceExtensionProperties failed %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    m_supportedExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_device, nullptr, &extensionCount, m_supportedExtensions.data());
    if (vkResult != VK_SUCCESS && vkResult != VK_INCOMPLETE) {
        LOG_ERROR("vkEnumerateDeviceExtensionProperties failed %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanPhysicalDevice::Finalize() {
    return Graphics::GraphicsError::OK;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetQueueIndex(RequiredQueueProperties *requirements) const {
    std::optional<uint32_t> ret;

    // Find a queue that has all required flags
    for (size_t i = 0; i < m_queueFamilies.size(); ++i) {
        if ((m_queueFamilies[i].queueFlags & requirements->queueFlags) == requirements->queueFlags) {
            // If surface support is required, check if the queue has present support
            if (requirements->surfaceSupport) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_device, static_cast<uint32_t>(i), *requirements->surfaceSupport, &presentSupport);
                if (!presentSupport) {
                    continue;
                }
            }

            ret = static_cast<uint32_t>(i);
            return ret;
        }
    }

    return ret;
}

} // namespace Vulkan
