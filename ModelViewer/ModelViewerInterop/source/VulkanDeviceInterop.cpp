#include "pch.h"
#include "VulkanDeviceInterop.h"

VulkanDevice::VulkanDevice()
  : m_nativeDevice(nullptr) {

}

VulkanDevice::~VulkanDevice() {

}

void VulkanDevice::SetNativeDevice(Graphics::PhysicalDevice *nativeDevice) {
    m_nativeDevice = nativeDevice;
}

Graphics::PhysicalDevice *VulkanDevice::GetNativeDevice() {
    return m_nativeDevice;
}
