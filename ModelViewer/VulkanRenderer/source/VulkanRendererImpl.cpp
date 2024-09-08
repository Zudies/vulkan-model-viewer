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
    m_queueIndices{},
    m_queues{},
    m_commandPools{},
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

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Finalize() {
    ASSERT(m_api->m_vkInstance);

    for (int i = 0; i < QUEUE_COUNT; ++i) {
        if (m_commandPools[i]) {
            vkDestroyCommandPool(m_device, m_commandPools[i], nullptr);
        }
    }

    if (m_device) {
        vkDestroyDevice(m_device, nullptr);
        m_device = 0;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Update(f64 deltaTime) {
    ASSERT(m_api->m_vkInstance);

    // Update all active scenes
    for (auto *scene : m_activeScenes) {
        auto error = scene->Update(deltaTime);
        if (error != Graphics::GraphicsError::OK) {
            return error;
        }
    }

    //TODO: ? Update all inactive scenes
    //for (auto *scene : m_inactiveScenes) {

    //}

    return Graphics::GraphicsError::OK;
}

void RendererImpl::SetSceneActive(Graphics::RendererScene_Base *activeScene) {
    m_inactiveScenes.erase(activeScene);
    m_activeScenes.emplace(activeScene);
}

void RendererImpl::SetSceneInactive(Graphics::RendererScene_Base *inactiveScene) {
    m_activeScenes.erase(inactiveScene);
    m_inactiveScenes.emplace(inactiveScene);
}

Graphics::GraphicsError RendererImpl::_createSwapChain(Graphics::RendererRequirements *requirements) {
    // Clean up any old swap chains
    _cleanupSwapChain();

    // Attempt to create swap chains for each surface in requirements
    int i = 0;
    Graphics::WindowSurface *curSurface = requirements->GetWindowSurface(i);
    while (curSurface) {
        LOG_VERBOSE(L"Creating swapchain for surface %d\n", i);
        // Get window size
        uint32_t width = 0, height = 0;
#if defined(_WIN32)
        Graphics::Win32WindowSurface *winSurface = static_cast<Graphics::Win32WindowSurface*>(curSurface);
        RECT windowSize;
        GetWindowRect(winSurface->GetHwnd(), &windowSize);
        width = windowSize.right - windowSize.left;
        height = windowSize.bottom - windowSize.top;
#else
#error Unsupported platform
#endif

        // If window is minimized, skip this surface
        if (width == 0 || height == 0) {
            VulkanSwapChain emptySwapChain;
            emptySwapChain.SetIndex(i);
            m_swapchains.push_back(emptySwapChain);

            curSurface = requirements->GetWindowSurface(++i);
            continue;
        }

        auto supportedSurfaceDescription = m_physicalDevice->GetSupportedSurfaceDescription(i, requirements);
        if (supportedSurfaceDescription) {
            VkSurfaceCapabilitiesKHR capabilities{};
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice->GetDevice(), supportedSurfaceDescription->GetSurface(), &capabilities) != VK_SUCCESS) {
                LOG_ERROR(L"  Unable to get surface capabilities\n");
                VulkanSwapChain emptySwapChain;
                emptySwapChain.SetIndex(i);
                m_swapchains.push_back(emptySwapChain);

                curSurface = requirements->GetWindowSurface(++i);
                continue;
            }

            // Pick an appropriate extent
            // Prefer VK's current extent over what Windows reports
            VkExtent2D desiredExtent { width, height };
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                desiredExtent = capabilities.currentExtent;
            }
            desiredExtent.width = std::clamp(desiredExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            desiredExtent.height = std::clamp(desiredExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            supportedSurfaceDescription->SetExtents(desiredExtent);

            // Create the VkSwapchain
            //TODO: Should this be in requirements json?
            uint32_t imageCount = capabilities.minImageCount + 1;
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
                LOG_ERROR(L"Failed to create VkSwapchain\n");
                return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
            }

            if (vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, nullptr) != VK_SUCCESS) {
                LOG_ERROR(L"Failed to get swapchain images\n");
                vkDestroySwapchainKHR(m_device, vkSwapChain, nullptr);
                return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
            }
            VulkanSwapChain::ImageArray swapChainImages(imageCount);
            if (vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, swapChainImages.data()) != VK_SUCCESS) {
                LOG_ERROR(L"Failed to get swapchain images\n");
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

            m_swapchains.emplace_back(*supportedSurfaceDescription);

            LOG_VERBOSE(L"  Swapchain created successfully for surface %d\n", i);
        }

        curSurface = requirements->GetWindowSurface(++i);
    }

    return Graphics::GraphicsError::OK;
}

void RendererImpl::_cleanupSwapChain() {
    vkDeviceWaitIdle(m_device);
    for (VulkanSwapChain &curSwapChain : m_swapchains) {
        auto &imageViews = curSwapChain.GetImageViews();

        for (auto imageView : imageViews) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_device, curSwapChain.GetSwapchain(), nullptr);
    }
    m_swapchains.clear();
}




} // namespace Vulkan
