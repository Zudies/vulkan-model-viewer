#include "pch.h"
#include "VulkanRendererImpl.h"

#include "base/RendererScene_Base.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesDefines.h"
#include "JsonRendererRequirements.h"
#include "Win32WindowSurface.h"
#include "VulkanCommandBuffer.h"

#include <set>

namespace Vulkan {

RendererImpl::RendererImpl()
  : m_api(nullptr),
    m_physicalDevice(nullptr),
    m_device(0),
    m_requirements(nullptr),
    m_queueIndices{},
    m_queues{},
    m_commandPools{},
    m_transferCommandPools{},
    m_swapChainOutOfDate(0),
    m_useValidation(false) {
}

RendererImpl::~RendererImpl() {
}

Graphics::GraphicsError RendererImpl::Initialize(API *api, VulkanPhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements) {
    m_api = api->GetImpl();
    ASSERT(m_api->m_vkInstance);
    ASSERT(physicalDevice);
    ASSERT(requirements);

    m_physicalDevice = physicalDevice;
    m_requirements = requirements;

    auto useValidationOption = requirements->GetBoolean(JSON_REQ_USE_VALIDATION);
    m_useValidation = useValidationOption.has_value() ? useValidationOption.value() : false;

    // Get the combined list of required and optional features
    std::set<std::string> features;
    auto requiredFeatures = requirements->GetArray(JSON_REQ_FEATURES_REQUIRED);
    if (requiredFeatures.has_value()) {
        features.insert(std::make_move_iterator(requiredFeatures->begin()), std::make_move_iterator(requiredFeatures->end()));
    }

    auto optionalFeatures = requirements->GetArray(JSON_REQ_FEATURES_OPTIONAL);
    if (optionalFeatures.has_value()) {
        for (auto &optionalIt : *optionalFeatures) {
            if (m_physicalDevice->SupportsFeature(optionalIt.c_str(), requirements)) {
                features.emplace(std::move(optionalIt));
            }
        }
    }

    // Figure out the queue and device requirements and necessary logical device-level layers/extensions
    std::optional<uint32_t> queueIndices[QueueType::QUEUE_COUNT] = {};
    std::set<uint32_t> uniqueQueues;
    VkPhysicalDeviceFeatures deviceFeatures{};
    for (auto &it : features) {
        if (it == FEATURE_IS_DISCRETE_GPU) {
            // Nothing to do
        }
        else if (it == FEATURE_SUPPORTS_GRAPHICS_OPERATIONS) {
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

            m_vkExtensionsList.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        else if (it == FEATURE_SUPPORTS_TRANSFER_OPERATIONS) {
            VulkanPhysicalDevice::RequiredQueueProperties queueRequirements{};
            queueRequirements.queueFlags |= VK_QUEUE_TRANSFER_BIT;
            queueRequirements.notQueueFlags |= VK_QUEUE_COMPUTE_BIT;
            auto queueIndex = m_physicalDevice->GetQueueIndex(&queueRequirements);
            if (!queueIndex) {
                // Fall back to graphics queue if no dedicated transfer queue
                queueRequirements.queueFlags = VK_QUEUE_GRAPHICS_BIT;
                queueRequirements.notQueueFlags = 0;
                queueIndex = m_physicalDevice->GetQueueIndex(&queueRequirements);

                if (!queueIndex) {
                    LOG_ERROR("Unable to find queue that supports 'SUPPORTS_TRANSFER_OPERATIONS'\n");
                    return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
                }
            }
            queueIndices[QueueType::QUEUE_TRANSFER] = *queueIndex;
            uniqueQueues.emplace(*queueIndex);
        }
        else if (it == FEATURE_SAMPLER_ANISOTROPY) {
            if (!m_physicalDevice->SupportsFeature(FEATURE_SAMPLER_ANISOTROPY, m_requirements)) {
                LOG_ERROR("Sampler anisotropy required but not supported\n");
                return Graphics::GraphicsError::NO_SUPPORTED_DEVICE;
            }
            deviceFeatures.samplerAnisotropy = true;
        }
        else {
            ERROR_MSG(L"Unknown feature name: %hs", it.c_str());
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

    // Require synchronization2
    VkPhysicalDeviceSynchronization2Features sync2Features{};
    sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = true;
    createInfo.pNext = &sync2Features;

    // Require fillModeNonSolid
    deviceFeatures.fillModeNonSolid = true;

#if defined(_DEBUG) && _DEBUG
    LOG_VERBOSE("Creating logical vkDevice with layers:\n");
    for (auto &i : m_vkLayersList) {
        LOG_VERBOSE("    + %s\n", i);
    }
    LOG_VERBOSE("Creating logical vkDevice with extensions:\n");
    for (auto &i : m_vkExtensionsList) {
        LOG_VERBOSE("    + %s\n", i);
    }
#endif

    VkResult vkResult = vkCreateDevice(m_physicalDevice->GetDevice(), &createInfo, VK_NULL_HANDLE, &m_device);
    if (vkResult != VK_SUCCESS) {
        LOG_ERROR("vkCreateDevice failed: %d\n", vkResult);
        return VulkanErrorToGraphicsError(vkResult);
    }

    for (int i = 0; i < QueueType::QUEUE_COUNT; ++i) {
        if (queueIndices[i]) {
            m_queueIndices[i] = *queueIndices[i];
            vkGetDeviceQueue(m_device, *queueIndices[i], 0, &m_queues[i]);
        }
    }

    LOG_INFO("Vulkan logical device successfully created!\n");

    LOG_INFO("Creating swap chains\n");
    _createSwapChain(requirements);
    LOG_INFO("Swap chains created successfully\n");

    LOG_INFO("Creating command pools\n");
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueIndices[QUEUE_GRAPHICS];

    // Regular command pools
    if (vkCreateCommandPool(m_device, &poolInfo, VK_NULL_HANDLE, &m_commandPools[QUEUE_GRAPHICS]) != VK_SUCCESS) {
        LOG_ERROR("Unable to create command pool for GRAPHICS queue\n");
        return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
    }

    if (m_queueIndices[QUEUE_GRAPHICS] != m_queueIndices[QUEUE_TRANSFER]) {
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_queueIndices[QUEUE_TRANSFER];
        if (vkCreateCommandPool(m_device, &poolInfo, VK_NULL_HANDLE, &m_commandPools[QUEUE_TRANSFER]) != VK_SUCCESS) {
            LOG_ERROR("Unable to create command pool for TRANSFER queue\n");
            return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
        }
    }
    else {
        m_commandPools[QUEUE_TRANSFER] = m_commandPools[QUEUE_GRAPHICS];
    }

    // Transfer command pools
    if (_createTransferCommandPools() != Graphics::GraphicsError::OK) {
        LOG_ERROR("Unable to create transfer command pools\n");
        return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
    }

    LOG_INFO("Command pools created successfully\n");

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Finalize() {
    ASSERT(m_api->m_vkInstance);

    vkDeviceWaitIdle(m_device);

    std::set<VkCommandPool> uniquePools;
    for (int i = 0; i < QUEUE_COUNT; ++i) {
        if (m_commandPools[i]) {
            uniquePools.insert(m_commandPools[i]);
            m_commandPools[i] = VK_NULL_HANDLE;
        }
        if (m_transferCommandPools[i]) {
            uniquePools.insert(m_transferCommandPools[i]);
            m_transferCommandPools[i] = VK_NULL_HANDLE;
        }
    }
    for (auto pool : uniquePools) {
        vkDestroyCommandPool(m_device, pool, VK_NULL_HANDLE);
    }

    for (int i = 0; i < m_swapchains.size(); ++i) {
        for (auto &callback : m_swapChainFuncs) {
            callback.first(i);
        }
    }
    _cleanupSwapChain(-1);

    if (m_device) {
        vkDestroyDevice(m_device, VK_NULL_HANDLE);
        m_device = VK_NULL_HANDLE;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Update(f64 deltaTime) {
    ASSERT(m_api->m_vkInstance);

    // Reset active scenes
    if (m_curFrameActiveScenes.size() != m_activeScenes.size()) {
        m_curFrameActiveScenes.insert(m_activeScenes.begin(), m_activeScenes.end());
    }

    /* Scene early update */
    for (Graphics::RendererScene_Base *scene : m_curFrameActiveScenes) {
        auto error = scene->EarlyUpdate(deltaTime);
        if (!_handleUpdateError(error, scene)) {
            return error;
        }
    }
    if (!m_erroredScenes.empty()) {
        for (auto *scene : m_erroredScenes) {
            m_curFrameActiveScenes.erase(scene);
        }
        m_erroredScenes.clear();
    }

    /* Transfer command submission */
    // Check if any transfer commands need to be submitted
    if (!m_registeredTransfers.empty()) {
        // Reserve the required amount of memory for all command buffers
        size_t totalCommandBufferCount = 0;
        for (auto &transferFunc : m_registeredTransfers) {
            totalCommandBufferCount += transferFunc.commandBufferCount;
        }
        m_activeTransferCommandBuffers.reserve(totalCommandBufferCount);

        for (size_t i = 0; i < m_registeredTransfers.size(); ++i) {
            // Create command buffers for each transfer func
            size_t startIndex = m_activeTransferCommandBuffers.size();
            auto err = _allocateTransferCommandBuffers(m_registeredTransfers[i].queue, static_cast<uint32_t>(m_registeredTransfers[i].commandBufferCount), &m_activeTransferCommandBuffers);
            if (err != Graphics::GraphicsError::OK) {
                // Call the error func then remove this transfer func
                m_registeredTransfers[i].errorFunc();
                m_registeredTransfers.erase(m_registeredTransfers.begin() + i);
                --i;
                continue;
            }

            // Call the beginFunc of the transfer
            err = m_registeredTransfers[i].beginFunc(&m_activeTransferCommandBuffers.at(startIndex));
            if (err != Graphics::GraphicsError::OK) {
                // No additional calls to the transfer func
                m_registeredTransfers.erase(m_registeredTransfers.begin() + i);
                --i;
                continue;
            }
            m_registeredTransfers[i].commandBufferIndex = startIndex;
        }

        // Call the endFunc of each transfer after all beginFuncs have been called
        for (size_t i = 0; i < m_registeredTransfers.size(); ++i) {
            // Ignore any errors as there will be no other calls to the transfer func regardless
            m_registeredTransfers[i].endFunc(&m_activeTransferCommandBuffers.at(m_registeredTransfers[i].commandBufferIndex));
        }

        // Clean up transfer funcs and command buffers
        m_registeredTransfers.clear();
        _freeTransferCommandBuffers(&m_activeTransferCommandBuffers);

        // Try to minimize amount of device memory held in cache
        //TODO: Verify the real effect of this
        vkResetCommandPool(m_device, m_transferCommandPools[QUEUE_GRAPHICS], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        vkResetCommandPool(m_device, m_transferCommandPools[QUEUE_TRANSFER], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    }

    /* Scene update */
    for (Graphics::RendererScene_Base *scene : m_curFrameActiveScenes) {
        auto error = scene->Update(deltaTime);
        if (!_handleUpdateError(error, scene)) {
            return error;
        }
    }
    if (!m_erroredScenes.empty()) {
        for (auto *scene : m_erroredScenes) {
            m_curFrameActiveScenes.erase(scene);
        }
        m_erroredScenes.clear();
    }

    /* Scene late update */
    for (Graphics::RendererScene_Base *scene : m_curFrameActiveScenes) {
        auto error = scene->LateUpdate(deltaTime);
        if (!_handleUpdateError(error, scene)) {
            return error;
        }
    }
    if (!m_erroredScenes.empty()) {
        for (auto *scene : m_erroredScenes) {
            m_curFrameActiveScenes.erase(scene);
        }
        m_erroredScenes.clear();
    }

    /* Swap chain recreation */
    if (m_swapChainOutOfDate) {
        for (int i = 0; m_swapChainOutOfDate && i < (sizeof(m_swapChainOutOfDate) * 8); ++i) {
            if (m_swapChainOutOfDate & (1u << i)) {
                vkDeviceWaitIdle(m_device);

                // Call destroy callbacks
                for (auto &callback : m_swapChainFuncs) {
                    callback.first(i);
                }

                // Try to recreate swapchain
                auto result = _createSwapChain(m_requirements, i);

                // Call create callbacks
                if (result == Graphics::GraphicsError::OK) {
                    for (auto &callback : m_swapChainFuncs) {
                        callback.second(i);
                    }
                    m_swapChainOutOfDate ^= (1u << i);
                }
                else if (result != Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE) {
                    return result;
                }
            }
        }
    }

    return Graphics::GraphicsError::OK;
}

void RendererImpl::SetSceneActive(Graphics::RendererScene_Base *activeScene) {
    m_inactiveScenes.erase(activeScene);
    m_activeScenes.emplace(activeScene);
    m_curFrameActiveScenes.emplace(activeScene);
}

void RendererImpl::SetSceneInactive(Graphics::RendererScene_Base *inactiveScene) {
    m_activeScenes.erase(inactiveScene);
    m_inactiveScenes.emplace(inactiveScene);
    m_curFrameActiveScenes.erase(inactiveScene);
}

void RendererImpl::RegisterOnRecreateSwapChainFunc(Graphics::Renderer_Base::OnDestroySwapChainFn destroyFunc, Graphics::Renderer_Base::OnCreateSwapChainFn createFunc) {
    m_swapChainFuncs.emplace_back(std::make_pair(destroyFunc, createFunc));
}

Graphics::GraphicsError RendererImpl::AllocateCommandBuffers(QueueType queue, VkCommandBufferLevel level, size_t count, VkCommandBuffer *cmdBufferOut) {
    ASSERT(m_commandPools[queue]);

    LOG_VERBOSE("Allocate %d command buffers for queue %d\n", count, queue);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPools[queue];
    allocInfo.level = level;
    allocInfo.commandBufferCount = static_cast<uint32_t>(count);

    if (vkAllocateCommandBuffers(m_device, &allocInfo, cmdBufferOut) != VK_SUCCESS) {
        LOG_VERBOSE("  Allocate failed\n");
        return Graphics::GraphicsError::COMMAND_BUFFER_CREATE_ERROR;
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::AcquireNextSwapChainImage(int swapChainIdx, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *imageIdxOut) {
    auto &swapChain = m_swapchains[swapChainIdx];

    // Treat an empty swapchain as out of date
    if (!swapChain.IsValid()) {
        return Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE;
    }

    VkResult result = vkAcquireNextImageKHR(m_device, swapChain.GetSwapchain(), timeout, semaphore, fence, imageIdxOut);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_swapChainOutOfDate |= 1u << swapChainIdx;
        return Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE;
    }
    else if (result != VK_SUCCESS) {
        return Graphics::GraphicsError::SWAPCHAIN_INVALID;
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::_createSwapChain(Graphics::RendererRequirements *requirements, int idx) {
    // Clean up any old swap chains
    _cleanupSwapChain(idx);

    // Attempt to create swap chains for each surface in requirements
    if (idx == -1) {
        VulkanSwapChain emptySwapChain;
        Graphics::WindowSurface *curSurface = requirements->GetWindowSurface(++idx);
        while (curSurface) {
            m_swapchains.emplace_back(emptySwapChain);

            // Just go to next surface on failure so ignore any errors
            _createSingleSwapChain(requirements, idx);
            curSurface = requirements->GetWindowSurface(++idx);
        }
    }
    else {
        // Only create swap chain for the specified index
        return _createSingleSwapChain(requirements, idx);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::_createSingleSwapChain(Graphics::RendererRequirements *requirements, int idx) {
    ASSERT(idx >= 0);

    VulkanSwapChain emptySwapChain;
    emptySwapChain.SetIndex(idx);
    m_swapchains[idx] = emptySwapChain;

    Graphics::WindowSurface *curSurface = requirements->GetWindowSurface(idx);
    if (!curSurface) {
        LOG_ERROR(L"Invalid surface for index %d when creating swap chain\n", idx);
        return Graphics::GraphicsError::SWAPCHAIN_INVALID;
    }

    // Get window size
    uint32_t width = 0, height = 0;
#if defined(_WIN32)
    Graphics::Win32WindowSurface *winSurface = static_cast<Graphics::Win32WindowSurface *>(curSurface);
    RECT windowSize;
    GetWindowRect(winSurface->GetHwnd(), &windowSize);
    width = windowSize.right - windowSize.left;
    height = windowSize.bottom - windowSize.top;
#else
#error Unsupported platform
#endif

    auto supportedSurfaceDescription = m_physicalDevice->GetSupportedSurfaceDescription(idx, requirements);
    if (!supportedSurfaceDescription) {
        LOG_ERROR(L"Surface not supported for index %d when creating swap chain\n", idx);
        return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
    }

    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice->GetDevice(), supportedSurfaceDescription->GetSurface(), &capabilities) != VK_SUCCESS) {
        LOG_ERROR(L"Unable to get surface capabilities for index %d when creating swap chain\n", idx);
        return Graphics::GraphicsError::SWAPCHAIN_INVALID;
    }

    // Pick an appropriate extent
    // Prefer VK's current extent over what Windows reports
    VkExtent2D desiredExtent{ width, height };
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        desiredExtent = capabilities.currentExtent;
    }
    desiredExtent.width = std::clamp(desiredExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    desiredExtent.height = std::clamp(desiredExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    supportedSurfaceDescription->SetExtents(desiredExtent);

    // If window is minimized, skip this surface
    if (desiredExtent.width == 0 || desiredExtent.height == 0) {
        return Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE;
    }

    LOG_VERBOSE(L"Creating swapchain for surface %d\n", idx);

    // Create the VkSwapchain
    //TODO: Should this be in requirements json?
    uint32_t imageCount = std::max(capabilities.minImageCount + 1, 3u);
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    //TODO: Read override values (present mode, etc.) from UI
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = supportedSurfaceDescription->GetSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = supportedSurfaceDescription->GetFormat();
    createInfo.imageColorSpace = supportedSurfaceDescription->GetColorSpace();
    createInfo.imageExtent = supportedSurfaceDescription->GetExtents();
    createInfo.presentMode = supportedSurfaceDescription->GetPresentMode();
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (m_queueIndices[QUEUE_GRAPHICS] != m_queueIndices[QUEUE_PRESENT]) {
        uint32_t queueFamilyIndices[] = { m_queueIndices[QUEUE_GRAPHICS], m_queueIndices[QUEUE_PRESENT] };
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkSwapchainKHR vkSwapChain;
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &vkSwapChain) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to create VkSwapchain\n");
        return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
    }

    if (vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, nullptr) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to get swapchain images\n");
        vkDestroySwapchainKHR(m_device, vkSwapChain, nullptr);
        return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
    }
    VulkanSwapChain::ImageArray swapChainImages(imageCount);
    if (vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, swapChainImages.data()) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to get swapchain images\n");
        vkDestroySwapchainKHR(m_device, vkSwapChain, nullptr);
        return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
    }

    VulkanSwapChain::ImageViewArray swapChainImageViews(imageCount);
    for (size_t k = 0; k < imageCount; ++k) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapChainImages[k];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = supportedSurfaceDescription->GetFormat();
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &swapChainImageViews[k]) != VK_SUCCESS) {
            LOG_ERROR(L"Failed to create swapchain image view\n");

            // Destroy all the image views that were created up to now
            while (--k >= 0) {
                vkDestroyImageView(m_device, swapChainImageViews[k], nullptr);
            }

            vkDestroySwapchainKHR(m_device, vkSwapChain, nullptr);
            return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
        }
    }

    supportedSurfaceDescription->SetSwapchain(vkSwapChain);
    supportedSurfaceDescription->SetImages(swapChainImages);
    supportedSurfaceDescription->SetImageViews(swapChainImageViews);

    m_swapchains[idx] = *supportedSurfaceDescription;

    LOG_VERBOSE(L"  Swapchain created successfully for surface %d\n", idx);
    return Graphics::GraphicsError::OK;
}

void RendererImpl::_cleanupSwapChain(int idx) {
    vkDeviceWaitIdle(m_device);

    if (idx == -1) {
        for (int i = 0; i < m_swapchains.size(); ++i) {
            _cleanupSwapChainSingle(i);
        }
        m_swapchains.clear();
    }
    else {
        _cleanupSwapChainSingle(idx);
        m_swapchains[idx] = VulkanSwapChain();
    }
}

void RendererImpl::_cleanupSwapChainSingle(int idx) {
    auto &imageViews = m_swapchains[idx].GetImageViews();

    for (auto imageView : imageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchains[idx].GetSwapchain(), nullptr);
}

Graphics::GraphicsError RendererImpl::GetMemoryTypeIndex(uint32_t typeFilter, uint32_t typeFlags, uint32_t poolFlags, uint32_t *out) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice->GetDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & typeFlags) == typeFlags
            && (memProperties.memoryHeaps[memProperties.memoryTypes[i].heapIndex].flags & poolFlags) == poolFlags) {

            *out = i;
            return Graphics::GraphicsError::OK;
        }
    }
    return Graphics::GraphicsError::NO_SUPPORTED_MEMORY;
}

VkDevice RendererImpl::GetDevice() const {
    return m_device;
}

VulkanPhysicalDevice *RendererImpl::GetPhysicalDevice() const {
    return m_physicalDevice;
}

Graphics::RendererRequirements *RendererImpl::GetRequirements() const {
    return m_requirements;
}

uint32_t RendererImpl::GetQueueIndex(QueueType type) const {
    return m_queueIndices[type];
}

VkQueue RendererImpl::GetQueue(QueueType type) const {
    return m_queues[type];
}

void RendererImpl::RegisterTransfer(QueueType queue, size_t commandBufferCount, TransferBeginFunc beginFunc, TransferEndFunc endFunc,
    TransferErrorFunc errorFunc) {
    ASSERT(queue != QUEUE_PRESENT);

    TransferFuncDescription transferFunc{};
    transferFunc.queue = queue;
    transferFunc.commandBufferCount = commandBufferCount;
    transferFunc.beginFunc = beginFunc;
    transferFunc.endFunc = endFunc;
    transferFunc.errorFunc = errorFunc;

    m_registeredTransfers.emplace_back(transferFunc);
}

bool RendererImpl::_handleUpdateError(Graphics::GraphicsError error, Graphics::RendererScene_Base *scene) {
    switch (error) {
    case Graphics::GraphicsError::OK:
        break;

    case Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE:
    case Graphics::GraphicsError::SWAPCHAIN_INVALID:
        // Remove from active scenes for this frame
        m_erroredScenes.emplace(scene);
        break;

    default:
        return false;
    }

    return true;
}

Graphics::GraphicsError RendererImpl::_createTransferCommandPools() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_queueIndices[QUEUE_GRAPHICS];
    if (vkCreateCommandPool(m_device, &poolInfo, VK_NULL_HANDLE, &m_transferCommandPools[QUEUE_GRAPHICS]) != VK_SUCCESS) {
        LOG_ERROR("Unable to create transfer command pool for GRAPHICS queue\n");
        return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
    }
    if (m_queueIndices[QUEUE_GRAPHICS] != m_queueIndices[QUEUE_TRANSFER]) {
        poolInfo.queueFamilyIndex = m_queueIndices[QUEUE_TRANSFER];
        if (vkCreateCommandPool(m_device, &poolInfo, VK_NULL_HANDLE, &m_transferCommandPools[QUEUE_TRANSFER]) != VK_SUCCESS) {
            LOG_ERROR("Unable to create transfer command pool for TRANSFER queue\n");
            return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
        }
    }
    else {
        m_transferCommandPools[QUEUE_TRANSFER] = m_transferCommandPools[QUEUE_GRAPHICS];
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::_allocateTransferCommandBuffers(uint32_t queue, uint32_t count, CommandBufferArray *outArray) {
    // If free list doesn't contain enough elements, allocate enough elements to the free list
    if (m_freeTransferCommandBuffers.size() < count) {
        m_freeTransferCommandBuffers.reserve(count);

        while (m_freeTransferCommandBuffers.size() < count) {
            VulkanCommandBuffer newCommandBuffer(this);
            newCommandBuffer.SetSingleUse(true);
            newCommandBuffer.SetLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            newCommandBuffer.SetWaitFence(true, VK_NULL_HANDLE);
            m_freeTransferCommandBuffers.emplace_back(std::move(newCommandBuffer));
        }
    }

    // Allocate VkCommandBuffers for the command buffers
    static std::vector<VkCommandBuffer> tempCommandBuffers;
    tempCommandBuffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_transferCommandPools[queue];
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, tempCommandBuffers.data()) != VK_SUCCESS) {
        return Graphics::GraphicsError::COMMAND_BUFFER_CREATE_ERROR;
    }

    auto firstElement = m_freeTransferCommandBuffers.begin() + (m_freeTransferCommandBuffers.size() - count);
    for (uint32_t i = 0; i < count; ++i) {
        auto &commandPool = *(firstElement + i);
        commandPool.Initialize(queue, tempCommandBuffers[i]);
    }

    // Move the command buffers into the output
    outArray->insert(
        outArray->end(),
        std::make_move_iterator(firstElement),
        std::make_move_iterator(m_freeTransferCommandBuffers.end())
    );

    m_freeTransferCommandBuffers.erase(m_freeTransferCommandBuffers.begin() + (m_freeTransferCommandBuffers.size() - count), m_freeTransferCommandBuffers.end());

    return Graphics::GraphicsError::OK;
}

void RendererImpl::_freeTransferCommandBuffers(CommandBufferArray *freeCommandBuffers) {
    // Reset each command buffer then move them into the free list
    for (auto &commandBuffer : *freeCommandBuffers) {
        auto vkCommandBuffer = commandBuffer.GetVkCommandBuffer();
        if (vkCommandBuffer) {
            vkFreeCommandBuffers(m_device, m_transferCommandPools[commandBuffer.GetQueue()], 1, &vkCommandBuffer);
        }
        commandBuffer.Clear();
    }
    m_freeTransferCommandBuffers.insert(
        m_freeTransferCommandBuffers.end(),
        std::make_move_iterator(freeCommandBuffers->begin()),
        std::make_move_iterator(freeCommandBuffers->end())
    );
    freeCommandBuffers->clear();
}

void RendererImpl::_releaseTransferCommandBufferResources() {
    m_freeTransferCommandBuffers.clear();

    // Destroy and recreate the transfer command pools to ensure memory is actually released to the device
    vkDestroyCommandPool(m_device, m_transferCommandPools[QUEUE_GRAPHICS], VK_NULL_HANDLE);
    if (m_transferCommandPools[QUEUE_GRAPHICS] != m_transferCommandPools[QUEUE_TRANSFER]) {
        vkDestroyCommandPool(m_device, m_transferCommandPools[QUEUE_TRANSFER], VK_NULL_HANDLE);
    }
    _createTransferCommandPools();
}

} // namespace Vulkan
