#pragma once

#include "base/API_Base.h"

namespace Vulkan {

class APIImpl;

class API : public Graphics::API_Base {
public:
    API();
    virtual ~API();

    virtual Graphics::GraphicsError Initialize();
    virtual Graphics::GraphicsError Finalize();
    virtual Graphics::GraphicsError Update(f32 deltaTime);

private:

    APIImpl *m_impl;
};

} // namespace Vulkan
