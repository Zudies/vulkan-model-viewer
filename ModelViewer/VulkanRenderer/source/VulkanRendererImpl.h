#pragma once

#include "VulkanPhysicalDevice.h"
#include <vector>

namespace Graphics {
class RendererRequirements;
}

namespace Vulkan {

class API;
class APIImpl;
class VulkanPhysicalDevice;

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();

    Graphics::GraphicsError Initialize(API *api, VulkanPhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

private:
    enum QueueType {
        QUEUE_GRAPHICS = 0,
        QUEUE_PRESENT = 1,

        QUEUE_COUNT
    };

    APIImpl *m_api;
    VulkanPhysicalDevice *m_physicalDevice;
    VkDevice m_device;
    VkQueue m_queues[QueueType::QUEUE_COUNT];

    bool m_useValidation;

    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

};

} // namespace Vulkan
