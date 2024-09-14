#pragma once

namespace Graphics {

class PhysicalDevice;

} // namespace Graphics

public interface class GraphicsDeviceInterface {
public:
    void SetNativeDevice(Graphics::PhysicalDevice *nativeDevice);
    Graphics::PhysicalDevice *GetNativeDevice();
};
