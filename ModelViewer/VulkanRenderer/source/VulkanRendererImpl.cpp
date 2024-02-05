#include "pch.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesList.h"
#include "JsonRendererRequirements.h"

namespace Vulkan {

RendererImpl::RendererImpl()
  : m_api(nullptr),
    m_physicalDevice(nullptr),
    m_device(0),
    m_queues(),
    m_useValidation(false) {

    memset(m_queues, 0, sizeof(m_queues));
}

RendererImpl::~RendererImpl() {
}

Graphics::GraphicsError RendererImpl::Initialize(API *api, VulkanPhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements) {
    m_api = api->GetImpl();
    ASSERT(m_api->m_vkInstance);
    ASSERT(physicalDevice);
    ASSERT(requirements);

    m_physicalDevice = physicalDevice;

    auto useValidationOption = requirements->GetBoolean(JSON_REQ_USE_VALIDATION);
    m_useValidation = useValidationOption.has_value() ? useValidationOption.value() : false;

    // Get the combined list of required and optional features
    //TODO: Check optional features for support before appending
    std::vector<std::string> features;
    auto requiredFeatures = requirements->GetArray(JSON_REQ_FEATURES_REQUIRED);
    if (requiredFeatures.has_value()) {
        features = std::move(*requiredFeatures);
    }

    auto optionalFeatures = requirements->GetArray(JSON_REQ_FEATURES_OPTIONAL);
    if (optionalFeatures.has_value()) {
        size_t numFeatures = features.size();
        features.resize(numFeatures + optionalFeatures->size());
        std::move(optionalFeatures->begin(), optionalFeatures->end(), features.begin() + numFeatures);
    }

    // Figure out the queue requirements
    VulkanPhysicalDevice::RequiredQueueProperties queueRequirements{};
    if (std::find(features.begin(), features.end(), FEATURE_IS_DISCRETE_GPU) != features.end()) {
        queueRequirements.queueFlags |= VK_QUEUE_GRAPHICS_BIT;
    }

    auto queueIndex = physicalDevice->GetQueueIndex(&queueRequirements);
    float queuePriority = 1.0f;
    if (!queueIndex.has_value()) {
        return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
    }

    // Figure out the device requirements
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.queueFamilyIndex = *queueIndex;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.pEnabledFeatures = &deviceFeatures;

    if (m_useValidation) {
        // Note: This is not needed in newer versions of Vulkan but previous versions of Vulkan
        //        still need to set validation layers at the logical device level
        for (auto i : VALIDATION_LAYERS) {
            m_vkLayersList.emplace_back(i);
        }
    }

    createInfo.enabledLayerCount = static_cast<uint32_t>(m_vkLayersList.size());
    createInfo.ppEnabledLayerNames = m_vkLayersList.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_vkExtensionsList.size());
    createInfo.ppEnabledExtensionNames = m_vkExtensionsList.data();

    VkResult vkResult = vkCreateDevice(m_physicalDevice->GetDevice(), &createInfo, nullptr, &m_device);
    if (vkResult != VK_SUCCESS) {
        return VulkanErrorToGraphicsError(vkResult);
    }

    vkGetDeviceQueue(m_device, *queueIndex, 0, m_queues + 0);

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Finalize() {
    ASSERT(m_api->m_vkInstance);

    if (m_device) {
        vkDestroyDevice(m_device, nullptr);
        m_device = 0;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Update(f64 deltaTime) {
    ASSERT(m_api->m_vkInstance);
    UNUSED_PARAM(deltaTime);

    return Graphics::GraphicsError::OK;
}




} // namespace Vulkan
