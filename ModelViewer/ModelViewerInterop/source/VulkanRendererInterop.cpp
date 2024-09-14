#include "pch.h"
#include "VulkanRendererInterop.h"

#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
  : m_nativeRenderer(new Vulkan::Renderer){
}

VulkanRenderer::~VulkanRenderer() {
    if (m_nativeRenderer) {
        m_nativeRenderer->Finalize();
        delete m_nativeRenderer;
    }
}

void VulkanRenderer::Initialize(GraphicsApiInterface ^api, GraphicsDeviceInterface ^device, RendererRequirementsInterface ^requirements) {
    m_nativeRenderer->Initialize(api->GetNativeApi(), device->GetNativeDevice(), requirements->GetNativeRequirements());
}

void VulkanRenderer::SetSceneActive(GraphicsSceneInterface ^scene) {
    m_nativeRenderer->SetSceneActive(scene->GetNativeScene());
}

Graphics::Renderer_Base *VulkanRenderer::GetNativeRenderer() {
    return m_nativeRenderer;
}

void VulkanRenderer::Update(float dt) {
    m_nativeRenderer->Update(dt);
}
