#include "pch.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"
#include "VulkanFeaturesDefines.h"
#include "JsonRendererRequirements.h"
#include "Win32WindowSurface.h"

#include "VulkanRendererScene_Basic.h"

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
    m_swapChainOutOfDate(0),
    m_useValidation(false),
    m_transferFence{},
    m_transferCommandBuffer{} {
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

    VkResult vkResult = vkCreateDevice(m_physicalDevice->GetDevice(), &createInfo, nullptr, &m_device);
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

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPools[QUEUE_GRAPHICS]) != VK_SUCCESS) {
        LOG_ERROR("Unable to create command pool for GRAPHICS queue\n");
        return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
    }

    // Note: This can technically fail if it's the graphics queue and max pools already created?
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_queueIndices[QUEUE_TRANSFER];
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPools[QUEUE_TRANSFER]) != VK_SUCCESS) {
        LOG_ERROR("Unable to create command pool for TRANSFER queue\n");
        return Graphics::GraphicsError::COMMAND_POOL_CREATE_ERROR;
    }
    LOG_INFO("Command pools created successfully\n");

    // Create transfer sync objects
    VkFenceCreateInfo transferFenceInfo{};
    transferFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    for (int i = 0; i < QUEUE_COUNT; ++i) {
        vkCreateFence(m_device, &transferFenceInfo, nullptr, &m_transferFence[i]);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Finalize() {
    ASSERT(m_api->m_vkInstance);

    vkDeviceWaitIdle(m_device);

    for (int i = 0; i < QUEUE_COUNT; ++i) {
        vkDestroyFence(m_device, m_transferFence[i], VK_NULL_HANDLE);
    }

    for (int i = 0; i < QUEUE_COUNT; ++i) {
        if (m_commandPools[i]) {
            vkDestroyCommandPool(m_device, m_commandPools[i], nullptr);
        }
    }

    for (int i = 0; i < m_swapchains.size(); ++i) {
        for (auto &callback : m_swapChainFuncs) {
            callback.first(i);
        }
    }
    _cleanupSwapChain(-1);

    if (m_device) {
        vkDestroyDevice(m_device, nullptr);
        m_device = 0;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Update(f64 deltaTime) {
    ASSERT(m_api->m_vkInstance);
    
    // Reset active scenes as necessary
    if (m_curFrameActiveScenes.size() != m_activeScenes.size()) {
        m_curFrameActiveScenes.insert(m_activeScenes.begin(), m_activeScenes.end());
    }

    // Early update step
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

    // Check if any transfer commands need to be submitted
    for (int i = 0; i < QUEUE_COUNT; ++i) {
        if (!m_registeredTransfers[i].empty()) {
            auto err = AllocateCommandBuffers(static_cast<QueueType>(i), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &m_transferCommandBuffer[i]);
            if (err != Graphics::GraphicsError::OK) {
                return err;
            }

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            if (vkBeginCommandBuffer(m_transferCommandBuffer[i], &beginInfo) != VK_SUCCESS) {
                vkFreeCommandBuffers(m_device, m_commandPools[QUEUE_TRANSFER], 1, &m_transferCommandBuffer[i]);
                return Graphics::GraphicsError::COMMAND_BUFFER_CREATE_ERROR;
            }

            for (auto &transferFunc : m_registeredTransfers[i]) {
                transferFunc.first(m_transferCommandBuffer[i]);
            }

            // Submit the command buffer and setup fences
            vkEndCommandBuffer(m_transferCommandBuffer[i]);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_transferCommandBuffer[i];

            vkResetFences(m_device, 1, &m_transferFence[i]);
            auto vkErr = vkQueueSubmit(m_queues[i], 1, &submitInfo, m_transferFence[i]);
            if (vkErr == VK_ERROR_DEVICE_LOST) {
                //TODO: Need to recreate device
                return Graphics::GraphicsError::DEVICE_LOST;
            }
            else if (vkErr != VK_SUCCESS) {
                return Graphics::GraphicsError::QUEUE_ERROR;
            }
        }
    }

    // If there were transfer operations, wait for them to finish
    for (int i = 0; i < QUEUE_COUNT; ++i) {
        if (!m_registeredTransfers[i].empty()) {
            vkWaitForFences(m_device, 1, &m_transferFence[i], VK_TRUE, UINT64_MAX);
            vkFreeCommandBuffers(m_device, m_commandPools[i], 1, &m_transferCommandBuffer[i]);

            for (auto &transferFunc : m_registeredTransfers[i]) {
                transferFunc.second();
            }
            m_registeredTransfers[i].clear();
        }
    }

    // Update all active scenes
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

    // Late update step
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

    // Check for any swap chain that is out of date
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

void RendererImpl::RegisterTransfer(QueueType queue, TransferBeginFunc beginFunc, TransferEndFunc endFunc) {
    m_registeredTransfers[queue].emplace_back(std::make_pair(beginFunc, endFunc));
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

} // namespace Vulkan
