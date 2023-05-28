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

Graphics::GraphicsError API::Initialize() {
    ASSERT(!m_impl);

    m_impl = new APIImpl;
    return m_impl->Initialize();
}

Graphics::GraphicsError API::Finalize() {
    ASSERT(m_impl);

    return m_impl->Finalize();
}

Graphics::GraphicsError API::Update(f32 deltaTime) {
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
