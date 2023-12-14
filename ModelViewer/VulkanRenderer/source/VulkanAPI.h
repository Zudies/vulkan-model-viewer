#pragma once

#include "base/API_Base.h"

namespace Vulkan {

class APIImpl;

class API : public Graphics::API_Base {
public:
    API();
    virtual ~API();

    virtual Graphics::GraphicsError Initialize(Graphics::RendererRequirements *requirements);
    virtual Graphics::GraphicsError Finalize();

    virtual Graphics::PhysicalDevice *GetDevice(size_t index);
    virtual Graphics::PhysicalDevice *FindSuitableDevice(Graphics::RendererRequirements *requirements);

    APIImpl *GetImpl() const { return m_impl; }

private:

    APIImpl *m_impl;
};

} // namespace Vulkan
