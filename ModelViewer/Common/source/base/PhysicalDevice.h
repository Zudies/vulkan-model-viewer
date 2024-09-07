#pragma once

namespace Graphics {

class RendererRequirements;

class PhysicalDevice {
public:
    PhysicalDevice() = default;
    PhysicalDevice(PhysicalDevice const &) = delete;
    virtual ~PhysicalDevice() = 0;
    virtual bool SupportsFeature(char const *featureName, RendererRequirements *requirements) const = 0;
    virtual bool SupportsSurface(int surfaceIndex, RendererRequirements *requirements) const = 0;

};

} // namespace Graphics
