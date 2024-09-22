#include "pch.h"
#include "VulkanRendererScene_Basic.h"
#include "VulkanRendererSceneImpl_Basic.h"
#include "VulkanRenderer.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

RendererScene_Basic::RendererScene_Basic()
  : m_impl(nullptr) {
}

RendererScene_Basic::~RendererScene_Basic() {
    delete m_impl;
}

Graphics::GraphicsError RendererScene_Basic::Initialize(Graphics::Renderer_Base *parentRenderer) {
    ASSERT(!m_impl);

    m_impl = new RendererSceneImpl_Basic(static_cast<Renderer*>(parentRenderer)->GetImpl());
    return m_impl->Initialize();
}

Graphics::GraphicsError RendererScene_Basic::Finalize() {
    ASSERT(m_impl);
    return m_impl->Finalize();
}

Graphics::GraphicsError RendererScene_Basic::EarlyUpdate(f64 deltaTime) {
    ASSERT(m_impl);
    return m_impl->EarlyUpdate(deltaTime);
}

Graphics::GraphicsError RendererScene_Basic::Update(f64 deltaTime) {
    ASSERT(m_impl);
    return m_impl->Update(deltaTime);
}

Graphics::GraphicsError RendererScene_Basic::LateUpdate(f64 deltaTime) {
    ASSERT(m_impl);
    return m_impl->LateUpdate(deltaTime);
}

std::string RendererScene_Basic::GetPipelineStateValue(const std::string &pipelineState) {
    ASSERT(m_impl);
    return m_impl->GetPipelineStateValue(pipelineState);
}

void RendererScene_Basic::SetPipelineStateValue(const std::string &pipelineState, const std::string &pipelineStateValue) {
    ASSERT(m_impl);
    return m_impl->SetPipelineStateValue(pipelineState, pipelineStateValue);
}

} // namespace Vulkan
