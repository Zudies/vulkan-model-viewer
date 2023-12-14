#pragma once

#include "base/API_Base.h"
#include "VulkanPhysicalDevice.h"
#include <vector>

namespace Vulkan {

class APIImpl {
public:
    APIImpl();
    ~APIImpl();

    Graphics::GraphicsError Initialize(Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError Finalize();

    VulkanPhysicalDevice *GetDevice(size_t index);
    VulkanPhysicalDevice *FindSuitableDevice(Graphics::RendererRequirements *requirements);

private:
    uint32_t _queryInstanceVersion();
    Graphics::GraphicsError _populateFeatureList(Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError _createInstance();
    Graphics::GraphicsError _queryDevices();

private:
    friend class RendererImpl;

private:
    VkInstance m_vkInstance;
    VkApplicationInfo m_vkAppInfo;

    bool m_useValidation;
    VkDebugUtilsMessengerEXT m_vkDebugMessenger;

    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

    typedef std::vector<VulkanPhysicalDevice> DeviceList;
    DeviceList m_physicalDevices;

};

} // namespace Vulkan
