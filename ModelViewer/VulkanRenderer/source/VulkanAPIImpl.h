#pragma once

#include "base/API_Base.h"

namespace Vulkan {

class APIImpl {
public:
    APIImpl();
    ~APIImpl();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();

private:
    friend class RendererImpl;

};

} // namespace Vulkan
