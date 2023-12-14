#pragma once

namespace Vulkan {

class API;
class APIImpl;
class VulkanPhysicalDevice;

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();

    Graphics::GraphicsError Initialize(API *api, VulkanPhysicalDevice *physicalDevice);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

private:
    APIImpl *m_api;
    VkDevice m_device;

};

} // namespace Vulkan
