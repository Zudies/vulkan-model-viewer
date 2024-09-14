#pragma once

#include "GraphicsRendererInterface.h"

namespace Vulkan {

class Renderer;

} // namespace Vulkan

public ref class VulkanRenderer : public GraphicsRendererInterface {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    virtual void Initialize(GraphicsApiInterface ^api, GraphicsDeviceInterface ^device, RendererRequirementsInterface ^requirements);
    virtual void SetSceneActive(GraphicsSceneInterface ^scene);

    virtual Graphics::Renderer_Base *GetNativeRenderer();

    virtual void Update(float dt);

private:
    Vulkan::Renderer *m_nativeRenderer;
};
