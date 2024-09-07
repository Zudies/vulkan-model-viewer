#include "pch.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesDefines.h"
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

bool VulkanPhysicalDevice::SupportsSurface(int surfaceIndex, Graphics::RendererRequirements *requirements) const {
    return GetSupportedSurfaceDescription(surfaceIndex, requirements).has_value();
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

std::optional<VulkanSwapChain> VulkanPhysicalDevice::GetSupportedSurfaceDescription(int surfaceIndex, Graphics::RendererRequirements *requirements) const {
    std::optional<VulkanSwapChain> ret;

    Graphics::WindowSurface *surface = requirements->GetWindowSurface(surfaceIndex);
    if (surface == nullptr) {
        return ret;
    }

    // Find the corresponding surface description in the requirements
    char jsonPointerBuffer[64];
    int i = 0;
    do {
        sprintf_s(jsonPointerBuffer, JSON_REQ_SURFACES_INDEX, i);
        auto descriptionIndex = requirements->GetNumber(jsonPointerBuffer);
        if (!descriptionIndex) {
            // Failure to get index means there are no more elements
            return ret;
        }

        if (*descriptionIndex == surfaceIndex) {
            auto vkSurface = m_api->GetWindowSurface(requirements->GetWindowSurface(surfaceIndex));
            if (!vkSurface) {
                // No surface at index
                return ret;
            }

            VulkanSwapChain supportedSurface;
            supportedSurface.SetIndex(static_cast<u32>(*descriptionIndex));
            supportedSurface.SetSurface(*vkSurface);

            VkSurfaceCapabilitiesKHR capabilities;
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device, *vkSurface, &capabilities) != VK_SUCCESS) {
                return ret;
            }

            // Check for preferred format support
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, *vkSurface, &formatCount, nullptr);
            if (formatCount != 0) {
                std::vector<VkSurfaceFormatKHR> formats(formatCount);
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, *vkSurface, &formatCount, formats.data()) != VK_SUCCESS) {
                    return ret;
                }

                sprintf_s(jsonPointerBuffer, JSON_REQ_SURFACES_FORMATS, i);
                auto preferredFormats = requirements->GetArray(jsonPointerBuffer);

                sprintf_s(jsonPointerBuffer, JSON_REQ_SURFACES_COLORSPACES, i);
                auto preferredColorSpaces = requirements->GetArray(jsonPointerBuffer);

                // Match preferred format in order
                if (!preferredFormats || !preferredColorSpaces) {
                    ERROR_MSG(L"Requirements file does not contain surface Formats or Color Spaces for index %d", surfaceIndex);
                    return ret;
                }

                // Formats and color spaces should be same size
                if (preferredFormats->size() != preferredColorSpaces->size()) {
                    ERROR_MSG(L"Requirements file does not have matching Formats and Color Spaces entries for index %d", surfaceIndex);
                    return ret;
                }

                for (size_t k = 0; k < preferredFormats->size(); ++k) {
                    VkSurfaceFormatKHR desiredFormat{
                        VulkanSwapChain::StringToFormat(preferredFormats->at(k)),
                        VulkanSwapChain::StringToColorSpace(preferredColorSpaces->at(k)) };
                    auto foundFormat = std::find_if(formats.begin(), formats.end(), [desiredFormat](VkSurfaceFormatKHR val) { return val.format == desiredFormat.format && val.colorSpace == desiredFormat.colorSpace; });
                    if (foundFormat != formats.end()) {
                        supportedSurface.SetFormat(desiredFormat.format);
                        supportedSurface.SetColorSpace(desiredFormat.colorSpace);
                        break;
                    }
                }

                if (supportedSurface.GetFormat() == VK_FORMAT_UNDEFINED) {
                    return ret;
                }
            }

            // Check for preferred present mode support
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_device, *vkSurface, &presentModeCount, nullptr);
            if (presentModeCount == 0) {
                return ret;
            }

            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_device, *vkSurface, &presentModeCount, presentModes.data());

            sprintf_s(jsonPointerBuffer, JSON_REQ_SURFACES_PRESENT_MODE, i);
            auto preferredPresentModes = requirements->GetArray(jsonPointerBuffer);

            // Match preferred format in order
            if (!preferredPresentModes) {
                ERROR_MSG(L"Requirements file does not contain surface Present Modes for index %d", surfaceIndex);
                return ret;
            }

            for (size_t k = 0; k < preferredPresentModes->size(); ++k) {
                VkPresentModeKHR desiredPresentMode = VulkanSwapChain::StringToPresentMode(preferredPresentModes->at(k));
                auto foundPresentMode = std::find(presentModes.begin(), presentModes.end(), desiredPresentMode);
                if (foundPresentMode != presentModes.end()) {
                    supportedSurface.SetPresentMode(desiredPresentMode);
                    break;
                }
            }

            if (supportedSurface.GetPresentMode() == VK_PRESENT_MODE_MAX_ENUM_KHR) {
                return ret;
            }

            return supportedSurface;
        }

        ++i;
    } while (true);

    return ret;
}

} // namespace Vulkan
