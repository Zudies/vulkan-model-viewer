#include "pch.h"
#include "VulkanRenderer.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"

namespace Vulkan {

Renderer::Renderer()
  : Graphics::Renderer_Base(),
    m_impl(nullptr) {
}

Renderer::~Renderer() {
    delete m_impl;
}

Graphics::GraphicsError Renderer::Initialize(Graphics::API_Base *api) {
    ASSERT(!m_impl);

    m_impl = new RendererImpl;
    return m_impl->Initialize(static_cast<API*>(api));
}

Graphics::GraphicsError Renderer::Finalize() {
    ASSERT(m_impl);
    return m_impl->Finalize();
}

Graphics::GraphicsError Renderer::Update(f32 deltaTime) {
    ASSERT(m_impl);
    return m_impl->Update(deltaTime);
}

} // namespace Vulkan
