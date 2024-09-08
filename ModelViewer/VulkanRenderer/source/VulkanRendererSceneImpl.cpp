#include "pch.h"
#include "VulkanRendererSceneImpl.h"

namespace Vulkan {

RendererSceneImpl::RendererSceneImpl() {
}

RendererSceneImpl::~RendererSceneImpl() {
}

Graphics::GraphicsError RendererSceneImpl::Initialize() {
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl::Finalize() {
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl::Update(f64 deltaTime) {
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
