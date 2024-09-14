#include "pch.h"
#include "VulkanBasicSceneInterop.h"
#include "GraphicsRendererInterface.h"

#include "VulkanRendererScene_Basic.h"

VulkanBasicScene::VulkanBasicScene()
  : m_nativeScene(new Vulkan::RendererScene_Basic) {

}

VulkanBasicScene::~VulkanBasicScene() {
    if (m_nativeScene) {
        m_nativeScene->Finalize();
        delete m_nativeScene;
    }
}

void VulkanBasicScene::Initialize(GraphicsRendererInterface ^renderer) {
    m_nativeScene->Initialize(renderer->GetNativeRenderer());
}

Graphics::RendererScene_Base *VulkanBasicScene::GetNativeScene() {
    return m_nativeScene;
}
