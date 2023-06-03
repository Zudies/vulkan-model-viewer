#pragma once

namespace Graphics {

class PhysicalDevice {
public:
    virtual ~PhysicalDevice() = 0;
    virtual bool SupportsFeature(char const *featureName) const = 0;

};

} // namespace Graphics
