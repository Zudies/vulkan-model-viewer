#include "pch.h"
#include "VulkanRenderer.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"
#include "VulkanPhysicalDevice.h"
#include "JsonRendererRequirements.h"

namespace Vulkan {

Renderer::Renderer()
  : Graphics::Renderer_Base(),
    m_impl(nullptr) {
}

Renderer::~Renderer() {
    delete m_impl;
}

Graphics::GraphicsError Renderer::Initialize(Graphics::API_Base *api, Graphics::PhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements) {
    ASSERT(!m_impl);

    m_impl = new RendererImpl;
    return m_impl->Initialize(static_cast<API*>(api), static_cast<VulkanPhysicalDevice*>(physicalDevice), requirements);
}

Graphics::GraphicsError Renderer::Finalize() {
    ASSERT(m_impl);
    return m_impl->Finalize();
}

Graphics::GraphicsError Renderer::Update(f64 deltaTime) {
    ASSERT(m_impl);
    return m_impl->Update(deltaTime);
}

void Renderer::SetSceneActive(Graphics::RendererScene_Base *activeScene) {
    ASSERT(m_impl);
    m_impl->SetSceneActive(activeScene);
}

void Renderer::SetSceneInactive(Graphics::RendererScene_Base *inactiveScene) {
    ASSERT(m_impl);
    m_impl->SetSceneInactive(inactiveScene);
}

void Renderer::RegisterOnRecreateSwapChainFunc(OnDestroySwapChainFn destroyFunc, OnCreateSwapChainFn createFunc) {
    ASSERT(m_impl);
    m_impl->RegisterOnRecreateSwapChainFunc(destroyFunc, createFunc);
}

RendererImpl *Renderer::GetImpl() {
    return m_impl;
}

} // namespace Vulkan
