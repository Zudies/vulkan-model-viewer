#include "pch.h"
#include "VulkanAPIImpl.h"

namespace Vulkan {

APIImpl::APIImpl()
  : m_vkInstance(nullptr) {
}

APIImpl::~APIImpl() {

}

Graphics::GraphicsError APIImpl::Initialize() {
    ASSERT(!m_vkInstance);

    VkApplicationInfo appInfo{};


    VkInstanceCreateInfo createInfo{};

    auto result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::Finalize() {
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::Update(f32 deltaTime) {
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
