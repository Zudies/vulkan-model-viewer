#include "pch.h"
#include "VulkanRendererImpl.h"
#include "VulkanAPI.h"
#include "VulkanAPIImpl.h"

namespace Vulkan {

RendererImpl::RendererImpl()
  : m_api(nullptr) {
}

RendererImpl::~RendererImpl() {
}

Graphics::GraphicsError RendererImpl::Initialize(API *api) {
    m_api = api->GetImpl();
    ASSERT(m_api->m_vkInstance);

    

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererImpl::Finalize() {
    ASSERT(m_api->m_vkInstance);

    return Graphics::GraphicsError::OK;

}

Graphics::GraphicsError RendererImpl::Update(f64 deltaTime) {
    ASSERT(m_api->m_vkInstance);
    UNUSED_PARAM(deltaTime);

    return Graphics::GraphicsError::OK;
}




} // namespace Vulkan
