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

    virtual Graphics::PhysicalDevice const *GetDevice(size_t index) const;
    virtual Graphics::PhysicalDevice const *FindSuitableDevice(Graphics::API_Base::FeatureList const &requiredFeatures, Graphics::API_Base::FeatureList const &optionalFeatures) const;

    APIImpl *GetImpl() const { return m_impl; }

private:

    APIImpl *m_impl;
};

} // namespace Vulkan
