#pragma once

#include "base/API_Base.h"
#include <vector>

namespace Vulkan {

class APIImpl {
public:
    APIImpl();
    ~APIImpl();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();

private:
    uint32_t _queryInstanceVersion();
    Graphics::GraphicsError _populateFeatureList();

private:
    friend class RendererImpl;

private:
    VkInstance m_vkInstance;
    VkApplicationInfo m_vkAppInfo;

    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

};

} // namespace Vulkan
