#pragma once

#include "base/API_Base.h"
#include "VulkanPhysicalDevice.h"
#include <vector>

namespace Vulkan {

class APIImpl {
public:
    APIImpl();
    ~APIImpl();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();

    VulkanPhysicalDevice const *GetDevice(size_t index) const;
    VulkanPhysicalDevice const *FindSuitableDevice(Graphics::API_Base::FeatureList const &requiredFeatures, Graphics::API_Base::FeatureList const &optionalFeatures) const;

private:
    uint32_t _queryInstanceVersion();
    Graphics::GraphicsError _populateFeatureList();
    Graphics::GraphicsError _createInstance();
    Graphics::GraphicsError _queryDevices();

private:
    friend class RendererImpl;

private:
    VkInstance m_vkInstance;
    VkApplicationInfo m_vkAppInfo;
    VkDebugUtilsMessengerEXT m_vkDebugMessenger;

    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

    typedef std::vector<VulkanPhysicalDevice> DeviceList;
    DeviceList m_physicalDevices;

};

} // namespace Vulkan
