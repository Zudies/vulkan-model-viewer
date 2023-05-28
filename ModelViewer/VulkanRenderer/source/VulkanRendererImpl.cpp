#include "pch.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"

namespace Vulkan {

RendererImpl::RendererImpl()
  : m_api(nullptr),
    m_vkInstance(0) {
}

RendererImpl::~RendererImpl() {
}

Graphics::GraphicsError RendererImpl::Initialize(API *api) {
    ASSERT(!m_vkInstance);
    m_api = api->GetImpl();

    VkApplicationInfo appInfo{};


    VkInstanceCreateInfo createInfo{};

    auto result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Finalize() {
    ASSERT(m_vkInstance);

    return Graphics::GraphicsError::OK;

}

Graphics::GraphicsError RendererImpl::Update(f32 deltaTime) {
    ASSERT(m_vkInstance);

    return Graphics::GraphicsError::OK;
}




} // namespace Vulkan
