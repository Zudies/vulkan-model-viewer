#pragma once

#include "base/API_Base.h"

namespace Vulkan {

class APIImpl {
public:
    APIImpl();
    ~APIImpl();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f32 deltaTime);

private:

    VkInstance m_vkInstance;
};

} // namespace Vulkan
