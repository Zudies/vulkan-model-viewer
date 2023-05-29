#include "pch.h"
#include "VulkanAPIImpl.h"

namespace Vulkan {

APIImpl::APIImpl()
  : m_vkInstance(0),
    m_vkAppInfo({}) {
}

APIImpl::~APIImpl() {
}

Graphics::GraphicsError APIImpl::Initialize() {
    ASSERT(m_vkInstance == 0);

    // This is where the API would normally be loaded, but we're using the static lib
    //  this time instead of the dll so no extra work needs to be done to load function
    //  pointers

    LOG_INFO("** Creating Vulkan Instance **\n");

    m_vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_vkAppInfo.pApplicationName = "Model Viewer";
    m_vkAppInfo.applicationVersion = VK_MAKE_API_VERSION(0, MV_VERSION_MAJOR, MV_VERSION_MINOR, MV_VERSION_PATCH);
    m_vkAppInfo.pEngineName = "Super amazing engine";
    m_vkAppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    m_vkAppInfo.apiVersion = _queryInstanceVersion();

    auto internalResult = _populateFeatureList();
    if (internalResult != Graphics::GraphicsError::OK) {
        return internalResult;
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &m_vkAppInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_vkExtensionsList.size());
    createInfo.ppEnabledExtensionNames = m_vkExtensionsList.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_vkLayersList.size());
    createInfo.ppEnabledLayerNames = m_vkLayersList.data();

    auto vkResult = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (vkResult != VK_SUCCESS) {
        //TODO: Which errors can be handled?
        return VulkanErrorToGraphicsError(vkResult);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::Finalize() {
    ASSERT(m_vkInstance != 0);

    vkDestroyInstance(m_vkInstance, nullptr);

    return Graphics::GraphicsError::OK;
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

Graphics::GraphicsError APIImpl::_populateFeatureList() {
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
    auto vkResult = vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    std::vector<VkLayerProperties> supportedLayers(count);
    do {
        vkResult = vkEnumerateInstanceLayerProperties(&count, supportedLayers.data());
    } while (vkResult == VK_INCOMPLETE);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Supported Instance Layers:\n");
    for (auto &it : supportedLayers) {
        LOG_INFO("  %s : %u : %s\n", it.layerName, it.specVersion, it.description);
    }

    // Set the desired layers based on build and features
    //m_vkLayersList.

    // Check list of available instance extensions
    vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    std::vector<VkExtensionProperties> supportedExtensions(count);
    do {
        vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &count, supportedExtensions.data());
    } while (vkResult == VK_INCOMPLETE);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    LOG_INFO("Supported Instance Extensions:\n");
    for (auto &it : supportedExtensions) {
        LOG_INFO("  %s : %u\n", it.extensionName, it.specVersion);
    }

    // Determine the required extensions
    //m_vkExtensionsList.

    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
