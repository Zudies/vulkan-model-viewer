#include "pch.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesDefines.h"
#include "JsonRendererRequirements.h"
#include <set>

#if defined(_WIN32)
#include "Win32WindowSurface.h"
#endif

namespace {

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

    UNUSED_PARAM(pUserData);

    char const *severity = "Unknown";
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        LOG_VERBOSE("!! Vulkan Validation: %s  Type: %x  Name: %s !!\n\t\t%s\n",
            "Verbose",
            messageType,
            pCallbackData->pMessageIdName,
            pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        LOG_ERROR("!! Vulkan Validation: %s  Type: %x  Name: %s !!\n\t\t%s\n",
            "Error",
            messageType,
            pCallbackData->pMessageIdName,
            pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        [[fallthrough]];
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            severity = "Info";
        }
        else {
            severity = "Warning";
        }
        [[fallthrough]];
    default:
        LOG_INFO("!! Vulkan Validation: %s  Type: %x  Name: %s !!\n\t\t%s\n",
            severity,
            messageType,
            pCallbackData->pMessageIdName,
            pCallbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

}

namespace Vulkan {

APIImpl::APIImpl()
  : m_vkInstance(0),
    m_vkAppInfo({}),
    m_useValidation(false),
    m_vkDebugMessenger(0) {
}

APIImpl::~APIImpl() {
}

Graphics::GraphicsError APIImpl::Initialize(Graphics::RendererRequirements *requirements) {
    ASSERT(m_vkInstance == 0);

    // This is where the API would normally be loaded, but we're using the static lib
    //  this time instead of the dll so no extra work needs to be done to load function
    //  pointers

    LOG_INFO("** Creating Vulkan Instance **\n");

    // Check if validation layers should be enabled
    auto useValidationOption = requirements->GetBoolean(JSON_REQ_USE_VALIDATION);
    m_useValidation = useValidationOption.has_value() ? useValidationOption.value() : false;

    m_vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_vkAppInfo.pApplicationName = "Model Viewer";
    m_vkAppInfo.applicationVersion = VK_MAKE_API_VERSION(0, MV_VERSION_MAJOR, MV_VERSION_MINOR, MV_VERSION_PATCH);
    m_vkAppInfo.pEngineName = "Super amazing engine";
    m_vkAppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    m_vkAppInfo.apiVersion = _queryInstanceVersion();

    // Determine features that can be supported
    auto internalResult = _populateFeatureList(requirements);
    if (internalResult != Graphics::GraphicsError::OK) {
        return internalResult;
    }

    // Create the vk instance
    internalResult = _createInstance();
    if (internalResult != Graphics::GraphicsError::OK) {
        return internalResult;
    }

    // Create all vk surfaces necessary
    internalResult = _createSurfaces(requirements);
    if (internalResult != Graphics::GraphicsError::OK) {
        return internalResult;
    }

    // Iterate the physical devices and determine their capabilities
    internalResult = _queryDevices();
    if (internalResult != Graphics::GraphicsError::OK) {
        return internalResult;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::Finalize() {
    if (m_vkInstance) {
        for (auto i : m_physicalDevices) {
            i->Finalize();
            delete i;
        }
        m_physicalDevices.clear();

        for (auto i : m_windowSurfaces) {
            vkDestroySurfaceKHR(m_vkInstance, i.second, nullptr);
        }
        m_windowSurfaces.clear();

        if (m_useValidation) {
            auto destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
            if (destroyDebugMessengerFn) {
                destroyDebugMessengerFn(m_vkInstance, m_vkDebugMessenger, nullptr);
            }
        }

        vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = 0;
    }

    return Graphics::GraphicsError::OK;
}

VulkanPhysicalDevice *APIImpl::GetDevice(size_t index) {
    if (index < m_physicalDevices.size()) {
        return m_physicalDevices[index];
    }
    return nullptr;
}

VulkanPhysicalDevice *APIImpl::FindSuitableDevice(Graphics::RendererRequirements *requirements) {
    VulkanPhysicalDevice *bestCandidate = nullptr;
    i32 bestCandidateScore = -1;

    auto requiredFeatures = requirements->GetArray(JSON_REQ_FEATURES_REQUIRED);
    if (!requiredFeatures.has_value()) {
        return m_physicalDevices.empty() ? nullptr : m_physicalDevices.front();
    }

    for (auto &device : m_physicalDevices) {
        // Check that all required features are supported
        bool allFeaturesSupported = true;
        if (requiredFeatures.has_value()) {
            for (auto curFeature : requiredFeatures.value()) {
                if (!device->SupportsFeature(curFeature.c_str(), requirements)) {
                    allFeaturesSupported = false;
                    break;
                }
            }
            if (!allFeaturesSupported) {
                continue;
            }
        }

        // Count the number of optional features supported
        auto optionalFeatures = requirements->GetArray(JSON_REQ_FEATURES_OPTIONAL);
        i32 supportedFeatureCount = 0;
        if (optionalFeatures.has_value()) {
            for (auto curFeature : optionalFeatures.value()) {
                if (device->SupportsFeature(curFeature.c_str(), requirements)) {
                    ++supportedFeatureCount;
                }
            }
        }

        // Check if this is the new best candidate
        if (supportedFeatureCount > bestCandidateScore) {
            bestCandidate = device;
            bestCandidateScore = supportedFeatureCount;
        }
    }

    return bestCandidate;
}

std::optional<VkSurfaceKHR> APIImpl::GetWindowSurface(Graphics::WindowSurface *genericSurface) {
    auto found = m_windowSurfaces.find(genericSurface);
    if (found != m_windowSurfaces.end()) {
        return found->second;
    }
    return std::optional<VkSurfaceKHR>();
}

uint32_t APIImpl::_queryInstanceVersion() {
    // If the vkGetInstanceProcAddr returns NULL for vkEnumerateInstanceVersion,
    //  it is a Vulkan 1.0 implementation
    if (vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion")) {
        // Otherwise, the application can call vkEnumerateInstanceVersion
        //  to determine the version of Vulkan.
        uint32_t apiVersion;
        auto result = vkEnumerateInstanceVersion(&apiVersion);
        ASSERT(result == VK_SUCCESS);
        return apiVersion;
    }
    return VK_VERSION_1_0;
}

Graphics::GraphicsError APIImpl::_populateFeatureList(Graphics::RendererRequirements *requirements) {
    LOG_INFO("Supported Vulkan version: %d.%d.%d\n",
        VK_API_VERSION_MAJOR(m_vkAppInfo.apiVersion),
        VK_API_VERSION_MINOR(m_vkAppInfo.apiVersion), 
        VK_API_VERSION_PATCH(m_vkAppInfo.apiVersion));

    // Check API version to determine a list of features that can be supported
    //TODO: Verify minimum api version required
    if (m_vkAppInfo.apiVersion < VK_MAKE_API_VERSION(0, 1, 0, 0)) {
        LOG_ERROR("Unsupported Vulkan version\n");
        return Graphics::GraphicsError::UNSUPPORTED_API_VERSION;
    }

    // Get list of available layers
    uint32_t count;
    std::vector<VkLayerProperties> supportedLayers;
    VkResult vkResult = vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (vkResult != VK_SUCCESS) {
        LOG_ERROR("vkEnumerateInstanceLayerProperties failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }
    
    supportedLayers.resize(count);
    vkResult = vkEnumerateInstanceLayerProperties(&count, supportedLayers.data());
    if (vkResult != VK_SUCCESS && vkResult != VK_INCOMPLETE) {
        LOG_ERROR("vkEnumerateInstanceLayerProperties failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Supported Instance Layers:\n");
    for (auto &it : supportedLayers) {
        LOG_INFO("  %s : %u : %s\n", it.layerName, it.specVersion, it.description);
    }

    // Get list of available instance extensions
    std::vector<VkExtensionProperties> supportedExtensions;
    vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (vkResult != VK_SUCCESS) {
        LOG_ERROR("vkEnumerateInstanceExtensionProperties failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    supportedExtensions.resize(count);
    vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &count, supportedExtensions.data());
    if (vkResult != VK_SUCCESS && vkResult != VK_INCOMPLETE) {
        LOG_ERROR("vkEnumerateInstanceExtensionProperties failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Supported Instance Extensions:\n");
    for (auto &it : supportedExtensions) {
        LOG_INFO("  %s : %u\n", it.extensionName, it.specVersion);
    }

    // Enable validation layers and extensions if necessary
    if (m_useValidation) {
        for (auto i : VALIDATION_LAYERS) {
            m_vkLayersList.emplace_back(i);
        }

        for (auto i : VALIDATION_EXTENSIONS) {
            m_vkExtensionsList.emplace_back(i);
        }
    }

    // Set additional desired layers and extensions based on build and features
    auto requiredFeatures = requirements->GetArray(JSON_REQ_FEATURES_REQUIRED);
    auto optionalFeatures = requirements->GetArray(JSON_REQ_FEATURES_OPTIONAL);
    std::set<const char*, StringComp> featureLayers, featureExtensions, optionalLayers, optionalExtensions;
    auto processFeaturesFunc = [&](std::vector<std::string> &featuresList,
                                   auto &extensionsOut,
                                   auto &layersOut) {
        for (auto &feature : featuresList) {
            if (feature == FEATURE_IS_DISCRETE_GPU) {
                // Nothing to do
            }
            else if (feature == FEATURE_SUPPORTS_GRAPHICS_OPERATIONS) {
            }
            else if (feature == FEATURE_SURFACE_WINDOW_PRESENT) {
                extensionsOut.emplace("VK_KHR_surface");
#if defined(_WIN32)
                extensionsOut.emplace("VK_KHR_win32_surface");
#else
#error Unsupported platform
#endif
            }
            else if (feature == FEATURE_SUPPORTS_TRANSFER_OPERATIONS) {
            }
            else if (feature == FEATURE_SAMPLER_ANISOTROPY) {
            }
            else {
                ERROR_MSG(L"Unknown feature name: %hs", feature.c_str());
            }
        }
    };
    if (requiredFeatures.has_value()) {
        processFeaturesFunc(*requiredFeatures, featureExtensions, featureLayers);
    }
    if (optionalFeatures.has_value()) {
        processFeaturesFunc(*optionalFeatures, optionalExtensions, optionalLayers);
    }

    // Filter out optional layers and extensions that are not supported
    // Note: Assuming supported layers/extensions does not grow too large or it would be faster
    //        to sort the list and do a set_intersection
    for (auto &layer : supportedLayers) {
        auto requestedLayer = optionalLayers.find(layer.layerName);
        if (requestedLayer != optionalLayers.end()) {
            featureLayers.emplace(*requestedLayer);
        }
    }
    for (auto &extension : supportedExtensions) {
        auto requestedExtension = optionalExtensions.find(extension.extensionName);
        if (requestedExtension != optionalExtensions.end()) {
            featureExtensions.emplace(*requestedExtension);
        }
    }

    // Update the layer and extension lists to be used
    m_vkLayersList.insert(m_vkLayersList.end(), featureLayers.begin(), featureLayers.end());
    m_vkExtensionsList.insert(m_vkExtensionsList.end(), featureExtensions.begin(), featureExtensions.end());

    // Verify all requested layers are supported
    for (auto i : m_vkLayersList) {
        bool layerFound = false;
        for (auto &k : supportedLayers) {
            if (strcmp(k.layerName, i) == 0) {
                layerFound = true;
                break;
            }
        }
        ASSERT_MSG(layerFound, L"Required Vulkan Layer %hs not supported\n", i);
    }

    // Verify all requested extensions are supported
    for (auto i : m_vkExtensionsList) {
        bool extFound = false;
        for (auto &k : supportedExtensions) {
            if (strcmp(k.extensionName, i) == 0) {
                extFound = true;
                break;
            }
        }
        ASSERT_MSG(extFound, L"Required Vulkan Extension %hs not supported\n", i);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::_createInstance() {
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &m_vkAppInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_vkExtensionsList.size());
    createInfo.ppEnabledExtensionNames = m_vkExtensionsList.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_vkLayersList.size());
    createInfo.ppEnabledLayerNames = m_vkLayersList.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_useValidation) {
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        createInfo.pNext = &debugCreateInfo;
    }

#if defined(_DEBUG) && _DEBUG
    LOG_VERBOSE("Creating vkInstance with layers:\n");
    for (auto &i : m_vkLayersList) {
        LOG_VERBOSE("    + %s\n", i);
    }
    LOG_VERBOSE("Creating vkInstance with extensions:\n");
    for (auto &i : m_vkExtensionsList) {
        LOG_VERBOSE("    + %s\n", i);
    }
#endif

    auto vkResult = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (vkResult != VK_SUCCESS) {
        //TODO: Which errors can be handled?
        LOG_ERROR("vkCreateInstance failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("VkInstance successfully created!\n");

    if (m_useValidation) {
        auto createDebugMessengerFn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
        if (createDebugMessengerFn) {
            vkResult = createDebugMessengerFn(
                m_vkInstance,
                &debugCreateInfo,
                nullptr,
                &m_vkDebugMessenger);
        }
        else {
            LOG_ERROR("Unable to load extension function 'vkCreateDebugUtilsMessengerEXT'\n");
            return Graphics::GraphicsError::NO_SUCH_EXTENSION;
        }
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::_queryDevices() {
    uint32_t deviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;

    VkResult vkResult = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
    if (vkResult != VK_SUCCESS) {
        LOG_ERROR("vkEnumeratePhysicalDevices failed %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }
    if (deviceCount == 0) {
        LOG_ERROR("Failed to fetch physical device list\n");
        return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
    }

    physicalDevices.resize(deviceCount);
    vkResult = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDevices.data());
    if (vkResult != VK_SUCCESS && vkResult != VK_INCOMPLETE) {
        LOG_ERROR("vkEnumeratePhysicalDevices failed %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Found available devices:\n");
    m_physicalDevices.resize(physicalDevices.size());
    for (size_t i = 0; i < physicalDevices.size(); ++i) {
        // Create VulkanPhysicalDevice and populate the physical device with queue info and features
        m_physicalDevices[i] = new VulkanPhysicalDevice;
        m_physicalDevices[i]->Initialize(this, physicalDevices[i]);

#if defined(_DEBUG) && _DEBUG
        int k = 0;
        for (auto &surface : m_windowSurfaces) {
            LOG_VERBOSE("    Supported surface formats at index %d:\n", k);
            uint32_t formatCount = 0;
            vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], surface.second, &formatCount, nullptr);

            if (vkResult == VkResult::VK_SUCCESS) {
                std::vector<VkSurfaceFormatKHR> formats(formatCount);
                vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], surface.second, &formatCount, formats.data());
                if (vkResult == VkResult::VK_SUCCESS) {
                    for (auto &format : formats) {
                        LOG_VERBOSE("        %u %u\n", format.format, format.colorSpace);
                    }
                }
            }

            LOG_VERBOSE("    Supported present modes at index %d:\n", k);
            uint32_t presentModeCount = 0;
            vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i], surface.second, &presentModeCount, nullptr);

            if (vkResult == VkResult::VK_SUCCESS) {
                std::vector<VkPresentModeKHR> presentModes(presentModeCount);
                vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i], surface.second, &presentModeCount, presentModes.data());
                if (vkResult == VkResult::VK_SUCCESS) {
                    for (auto &presentMode : presentModes) {
                        LOG_VERBOSE("        %u\n", presentMode);
                    }
                }
            }

            VkSurfaceCapabilitiesKHR capabilities;
            vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[i], surface.second, &capabilities);
            if (vkResult == VkResult::VK_SUCCESS) {
                LOG_VERBOSE("    Capabilities:\n");
                LOG_VERBOSE("        ImageCount: %u - %u\n", capabilities.minImageCount, capabilities.maxImageCount);
                LOG_VERBOSE("        MinExtent: %u x %u\n", capabilities.minImageExtent.width, capabilities.minImageExtent.height);
                LOG_VERBOSE("        MaxExtent: %u x %u\n", capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
                LOG_VERBOSE("        CurExtent: %u x %u\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
            }

            ++k;
        }
#endif
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::_createSurfaces(Graphics::RendererRequirements *requirements) {
    int i = 0;
    Graphics::WindowSurface *requiredSurface = requirements->GetWindowSurface(i);
    while (requiredSurface != nullptr) {
        VkSurfaceKHR newSurface;

#if defined(_WIN32)
        ASSERT(requiredSurface->GetType() == Graphics::WindowSurfaceType::SURFACE_WIN32);
        Graphics::Win32WindowSurface *win32Surface = static_cast<Graphics::Win32WindowSurface*>(requiredSurface);

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = win32Surface->GetHwnd();
        createInfo.hinstance = win32Surface->GetHinstance();

        VkResult vkResult = vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &newSurface);
#else
#error Unsupported platform
#endif
        if (vkResult != VK_SUCCESS) {
            LOG_ERROR("Unable to create win32 surface: %d\n", vkResult);
            return VulkanErrorToGraphicsError(vkResult);
        }

        m_windowSurfaces.emplace(requiredSurface, newSurface);
        requiredSurface = requirements->GetWindowSurface(++i);
    }
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
