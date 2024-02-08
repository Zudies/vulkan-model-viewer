#pragma once

#include "base/API_Base.h"

namespace Vulkan {

class APIImpl;

class API : public Graphics::API_Base {
public:
    API();
    virtual ~API();

    virtual Graphics::GraphicsError Initialize(Graphics::RendererRequirements *requirements) override;
    virtual Graphics::GraphicsError Finalize() override;

    virtual Graphics::PhysicalDevice *GetDevice(size_t index) override;
    virtual Graphics::PhysicalDevice *FindSuitableDevice(Graphics::RendererRequirements *requirements) override;

    APIImpl *GetImpl() const { return m_impl; }

private:

    APIImpl *m_impl;
};

} // namespace Vulkan
