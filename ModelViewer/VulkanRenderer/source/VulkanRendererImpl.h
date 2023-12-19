#pragma once

#include "VulkanPhysicalDevice.h"
#include <vector>

namespace Graphics {
class JsonRendererRequirements;
}

namespace Vulkan {

class API;
class APIImpl;
class VulkanPhysicalDevice;

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();

    Graphics::GraphicsError Initialize(API *api, VulkanPhysicalDevice *physicalDevice, Graphics::JsonRendererRequirements *requirements);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

private:
    APIImpl *m_api;
    VulkanPhysicalDevice *m_physicalDevice;
    VkDevice m_device;
    VkQueue m_queues[1]; //TODO: Increase as (if) additional queues are needed

    bool m_useValidation;

    typedef std::vector<char const *> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

};

} // namespace Vulkan
