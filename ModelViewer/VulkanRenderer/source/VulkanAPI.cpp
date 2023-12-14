#include "pch.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"

namespace Vulkan {

API::API()
  : API_Base(),
    m_impl(nullptr) {
}

API::~API() {
    delete m_impl;
}

Graphics::GraphicsError API::Initialize(Graphics::RendererRequirements *requirements) {
    ASSERT(!m_impl);

    m_impl = new APIImpl;
    return m_impl->Initialize(requirements);
}

Graphics::GraphicsError API::Finalize() {
    ASSERT(m_impl);

    return m_impl->Finalize();
}

Graphics::PhysicalDevice *API::GetDevice(size_t index) {
    ASSERT(m_impl);

    return m_impl->GetDevice(index);
}

Graphics::PhysicalDevice *API::FindSuitableDevice(Graphics::RendererRequirements *requirements) {
    ASSERT(m_impl);

    return m_impl->FindSuitableDevice(requirements);
}

} // namespace Vulkan
