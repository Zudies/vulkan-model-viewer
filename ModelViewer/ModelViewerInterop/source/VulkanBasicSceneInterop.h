#pragma once

#include "GraphicsSceneInterface.h"

namespace Vulkan {

class RendererScene_Basic;

} // namespace Vulkan

public ref class VulkanBasicScene : public GraphicsSceneInterface  {
public:
    VulkanBasicScene();
    ~VulkanBasicScene();

    virtual void Initialize(GraphicsRendererInterface ^renderer);
    virtual Graphics::RendererScene_Base *GetNativeScene();

private:
    Vulkan::RendererScene_Basic *m_nativeScene;
};
