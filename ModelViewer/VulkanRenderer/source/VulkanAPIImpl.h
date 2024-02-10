#pragma once

#include "base/API_Base.h"
#include "base/WindowSurface.h"
#include "VulkanPhysicalDevice.h"
#include <vector>
#include <map>

namespace Vulkan {

class APIImpl {
public:
    APIImpl();
    ~APIImpl();

    Graphics::GraphicsError Initialize(Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError Finalize();

    VulkanPhysicalDevice *GetDevice(size_t index);
    VulkanPhysicalDevice *FindSuitableDevice(Graphics::RendererRequirements *requirements);

    std::optional<VkSurfaceKHR> GetWindowSurface(Graphics::WindowSurface *genericSurface);

private:
    uint32_t _queryInstanceVersion();
    Graphics::GraphicsError _populateFeatureList(Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError _createInstance();
    Graphics::GraphicsError _queryDevices();
    Graphics::GraphicsError _createSurfaces(Graphics::RendererRequirements *requirements);

private:
    friend class RendererImpl;

private:
    VkInstance m_vkInstance;
    VkApplicationInfo m_vkAppInfo;

    bool m_useValidation;
    VkDebugUtilsMessengerEXT m_vkDebugMessenger;

    struct StringComp {
        bool operator()(const char *a, const char *b) const {
            return strcmp(a, b) < 0;
        }
    };
    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

    typedef std::vector<VulkanPhysicalDevice*> DeviceList;
    DeviceList m_physicalDevices;

    struct SurfaceComp {
        bool operator()(Graphics::WindowSurface *a, Graphics::WindowSurface *b) const {
            return a->Compare(*b);
        }
    };
    typedef std::map<Graphics::WindowSurface*, VkSurfaceKHR, SurfaceComp> SurfaceMap;
    SurfaceMap m_windowSurfaces;

};

} // namespace Vulkan
