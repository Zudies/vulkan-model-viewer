#include "pch.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesList.h"
#include "JsonRendererRequirements.h"
#include <set>

namespace Vulkan {

RendererImpl::RendererImpl()
  : m_api(nullptr),
    m_physicalDevice(nullptr),
    m_device(0),
    m_queues{},
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
    std::vector<std::string> features;
    auto requiredFeatures = requirements->GetArray(JSON_REQ_FEATURES_REQUIRED);
    if (requiredFeatures.has_value()) {
        features = std::move(*requiredFeatures);
    }

    auto optionalFeatures = requirements->GetArray(JSON_REQ_FEATURES_OPTIONAL);
    if (optionalFeatures.has_value()) {
        size_t numFeatures = features.size();
        features.reserve(numFeatures + optionalFeatures->size());
        for (auto &optionalIt : *optionalFeatures) {
            if (m_physicalDevice->SupportsFeature(optionalIt.c_str(), requirements)) {
                features.emplace_back(std::move(optionalIt));
            }
        }
    }

    // Figure out the queue and device requirements and necessary logical device-level layers/extensions
    std::optional<uint32_t> queueIndices[QueueType::QUEUE_COUNT] = {};
    std::set<uint32_t> uniqueQueues;
    VkPhysicalDeviceFeatures deviceFeatures{};
    for (auto &it : features) {
        if (it == FEATURE_SUPPORTS_GRAPHICS_OPERATIONS) {
            VulkanPhysicalDevice::RequiredQueueProperties queueRequirements{};
            queueRequirements.queueFlags |= VK_QUEUE_GRAPHICS_BIT;
            auto queueIndex = m_physicalDevice->GetQueueIndex(&queueRequirements);
            if (!queueIndex) {
                LOG_ERROR("Unable to find queue that supports 'SUPPORTS_GRAPHICS_OPERATIONS'\n");
                return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
            }
            queueIndices[QueueType::QUEUE_GRAPHICS] = *queueIndex;
            uniqueQueues.emplace(*queueIndex);
        }
        else if (it == FEATURE_SURFACE_WINDOW_PRESENT) {
            VulkanPhysicalDevice::RequiredQueueProperties queueRequirements{};
            queueRequirements.surfaceSupport = m_api->GetWindowSurface(requirements->GetWindowSurface(0));
            if (!queueRequirements.surfaceSupport) {
                LOG_ERROR("No valid window surface was passed when initializing renderer for surface present\n");
                return Graphics::GraphicsError::INITIALIZATION_FAILED;
            }
            auto queueIndex = m_physicalDevice->GetQueueIndex(&queueRequirements);
            if (!queueIndex) {
                LOG_ERROR("Unable to find queue that supports 'SURFACE_WINDOW_PRESENT'\n");
                return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
            }
            queueIndices[QueueType::QUEUE_PRESENT] = *queueIndex;
            uniqueQueues.emplace(*queueIndex);
        }
    }

    // Add validation layers if necessary
    if (m_useValidation) {
        // Note: This is not needed in newer versions of Vulkan but previous versions of Vulkan
        //        still need to set validation layers at the logical device level
        for (auto i : VALIDATION_LAYERS) {
            m_vkLayersList.emplace_back(i);
        }
    }

    // Setup CreateInfo for necessary queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfoList;
    for (auto uniqueQueueIndex : uniqueQueues) {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = uniqueQueueIndex;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfoList.emplace_back(std::move(queueCreateInfo));
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfoList.size());
    createInfo.pQueueCreateInfos = queueCreateInfoList.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_vkLayersList.size());
    createInfo.ppEnabledLayerNames = m_vkLayersList.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_vkExtensionsList.size());
    createInfo.ppEnabledExtensionNames = m_vkExtensionsList.data();

    VkResult vkResult = vkCreateDevice(m_physicalDevice->GetDevice(), &createInfo, nullptr, &m_device);
    if (vkResult != VK_SUCCESS) {
        LOG_ERROR("vkCreateDevice failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    for (int i = 0; i < QueueType::QUEUE_COUNT; ++i) {
        if (queueIndices[i]) {
            vkGetDeviceQueue(m_device, *queueIndices[i], 0, &m_queues[i]);
        }
    }

    LOG_INFO("Vulkan logical device successfully created!\n");
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
