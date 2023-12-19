#include "pch.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesList.h"
#include "JsonRendererRequirements.h"

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
    Graphics::JsonRendererRequirements *vulkanRequirements = static_cast<Graphics::JsonRendererRequirements *>(requirements);
    auto useValidationOption = vulkanRequirements->GetBoolean(JSON_REQ_USE_VALIDATION);
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
            i.Finalize();
        }

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
        return &m_physicalDevices[index];
    }
    return nullptr;
}

VulkanPhysicalDevice *APIImpl::FindSuitableDevice(Graphics::RendererRequirements *requirements) {
    VulkanPhysicalDevice *bestCandidate = nullptr;
    i32 bestCandidateScore = -1;

    Graphics::JsonRendererRequirements *vulkanRequirements = static_cast<Graphics::JsonRendererRequirements*>(requirements);
    auto requiredFeatures = vulkanRequirements->GetArray(JSON_REQ_FEATURES_REQUIRED);
    if (!requiredFeatures.has_value()) {
        return m_physicalDevices.empty() ? nullptr : &m_physicalDevices.front();
    }

    for (auto &device : m_physicalDevices) {
        // Check that all required features are supported
        bool allFeaturesSupported = true;
        if (requiredFeatures.has_value()) {
            for (auto curFeature : requiredFeatures.value()) {
                if (!device.SupportsFeature(curFeature.c_str())) {
                    allFeaturesSupported = false;
                    break;
                }
            }
            if (!allFeaturesSupported) {
                continue;
            }
        }

        // Count the number of optional features supported
        auto optionalFeatures = vulkanRequirements->GetArray(JSON_REQ_FEATURES_OPTIONAL);
        i32 supportedFeatureCount = 0;
        if (optionalFeatures.has_value()) {
            for (auto curFeature : optionalFeatures.value()) {
                if (device.SupportsFeature(curFeature.c_str())) {
                    ++supportedFeatureCount;
                }
            }
        }

        // Check if this is the new best candidate
        if (supportedFeatureCount > bestCandidateScore) {
            bestCandidate = &device;
            bestCandidateScore = supportedFeatureCount;
        }
    }

    return bestCandidate;
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
    //      Update a table of capabilities for features if not fatal
    if (m_vkAppInfo.apiVersion < VK_MAKE_API_VERSION(0, 1, 0, 0)) {
        return Graphics::GraphicsError::UNSUPPORTED_API_VERSION;
    }

    // Check list of available layers
    uint32_t count;
    std::vector<VkLayerProperties> supportedLayers;
    VkResult vkResult;
    do {
        vkResult = vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (vkResult != VK_SUCCESS) {
            return VulkanErrorToGraphicsError(vkResult);
        }

        supportedLayers.resize(count);
        vkResult = vkEnumerateInstanceLayerProperties(&count, supportedLayers.data());
    } while (vkResult == VK_INCOMPLETE);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Supported Instance Layers:\n");
    for (auto &it : supportedLayers) {
        LOG_INFO("  %s : %u : %s\n", it.layerName, it.specVersion, it.description);
    }

    // Enable validation layers in debug builds
    if (m_useValidation) {
        for (auto i : VALIDATION_LAYERS) {
            m_vkLayersList.emplace_back(i);
        }
    }

    // Set additional desired layers based on build and features
    Graphics::JsonRendererRequirements *vulkanRequirements = static_cast<Graphics::JsonRendererRequirements*>(requirements);
    (void)vulkanRequirements;
    //TODO:

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

    // Check list of available instance extensions
    std::vector<VkExtensionProperties> supportedExtensions;
    do {
        vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        if (vkResult != VK_SUCCESS) {
            return VulkanErrorToGraphicsError(vkResult);
        }

        supportedExtensions.resize(count);
        vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &count, supportedExtensions.data());
    } while (vkResult == VK_INCOMPLETE);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Supported Instance Extensions:\n");
    for (auto &it : supportedExtensions) {
        LOG_INFO("  %s : %u\n", it.extensionName, it.specVersion);
    }

    // Enable validation extensions in debug builds
    if (m_useValidation) {
        for (auto i : VALIDATION_EXTENSIONS) {
            m_vkExtensionsList.emplace_back(i);
        }
    }

    // Determine the required (and optional) extensions
    //m_vkExtensionsList.

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

    auto vkResult = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (vkResult != VK_SUCCESS) {
        //TODO: Which errors can be handled?
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
            return Graphics::GraphicsError::NO_SUCH_EXTENSION;
        }
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::_queryDevices() {
    uint32_t deviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;
    VkResult vkResult;

    do {
        vkResult = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
        if (vkResult != VK_SUCCESS) {
            return VulkanErrorToGraphicsError(vkResult);
        }
        if (deviceCount == 0) {
            return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
        }

        physicalDevices.resize(deviceCount);
        vkResult = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDevices.data());
    } while (vkResult == VK_INCOMPLETE);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Found available devices:\n");
    m_physicalDevices.resize(physicalDevices.size());
    for (size_t i = 0; i < physicalDevices.size(); ++i) {
        // Create VulkanPhysicalDevice and populate the physical device with queue info and features
        m_physicalDevices[i].Initialize(this, &physicalDevices[i]);
    }

    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
