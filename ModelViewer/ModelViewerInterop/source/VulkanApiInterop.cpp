#include "pch.h"
#include "VulkanApiInterop.h"
#include "JsonRequirementsInterop.h"
#include "VulkanDeviceInterop.h"

#include "VulkanAPI.h"

VulkanApi::VulkanApi() 
  : m_nativeApi(new Vulkan::API) {

}

VulkanApi::~VulkanApi() {
    if (m_nativeApi) {
        m_nativeApi->Finalize();
        delete m_nativeApi;
    }
}

void VulkanApi::Initialize(RendererRequirementsInterface ^requirements) {
    m_nativeApi->Initialize(requirements->GetNativeRequirements());
}

GraphicsDeviceInterface ^VulkanApi::FindSuitableDevice(RendererRequirementsInterface ^requirements) {
    Graphics::PhysicalDevice *nativeDevice = m_nativeApi->FindSuitableDevice(requirements->GetNativeRequirements());
    VulkanDevice ^managedDevice = gcnew VulkanDevice();
    managedDevice->SetNativeDevice(nativeDevice);
    return managedDevice;
}

Graphics::API_Base *VulkanApi::GetNativeApi() {
    return m_nativeApi;
}
