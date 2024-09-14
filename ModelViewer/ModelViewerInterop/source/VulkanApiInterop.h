#pragma once

#include "GraphicsApiInterface.h"

namespace Vulkan {

class API;

} // namespace Vulkan

public ref class VulkanApi : public GraphicsApiInterface {
public:
    VulkanApi();
    ~VulkanApi();

    virtual void Initialize(RendererRequirementsInterface ^requirements);
    virtual GraphicsDeviceInterface ^FindSuitableDevice(RendererRequirementsInterface ^requirements);

    virtual Graphics::API_Base *GetNativeApi();

private:
    Vulkan::API *m_nativeApi;
};
