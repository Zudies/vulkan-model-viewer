#pragma once

#include "GraphicsDeviceInterface.h"

public ref class VulkanDevice : public GraphicsDeviceInterface {
public:
    VulkanDevice();
    ~VulkanDevice();

    virtual void SetNativeDevice(Graphics::PhysicalDevice *nativeDevice);
    virtual Graphics::PhysicalDevice *GetNativeDevice();

private:
    Graphics::PhysicalDevice *m_nativeDevice;
};
