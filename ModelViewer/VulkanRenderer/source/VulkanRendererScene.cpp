#include "pch.h"
#include "VulkanRendererScene.h"
#include "VulkanRendererSceneImpl.h"

namespace Vulkan {

RendererScene::RendererScene() {
}

RendererScene::~RendererScene() {
}

Graphics::GraphicsError RendererScene::Initialize() {
    ASSERT(m_impl);
    return m_impl->Initialize();
}

Graphics::GraphicsError RendererScene::Finalize() {
    ASSERT(m_impl);
    return m_impl->Finalize();
}

Graphics::GraphicsError RendererScene::Update(f64 deltaTime) {
    ASSERT(m_impl);
    return m_impl->Update(deltaTime);
}

} // namespace Vulkan
