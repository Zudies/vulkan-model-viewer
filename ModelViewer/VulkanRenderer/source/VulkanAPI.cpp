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

Graphics::PhysicalDevice const *API::GetDevice(size_t index) const {
    ASSERT(m_impl);

    return m_impl->GetDevice(index);
}

Graphics::PhysicalDevice const *API::FindSuitableDevice(Graphics::API_Base::FeatureList const &requiredFeatures, Graphics::API_Base::FeatureList const &optionalFeatures) const {
    ASSERT(m_impl);

    return m_impl->FindSuitableDevice(requiredFeatures, optionalFeatures);
}

} // namespace Vulkan
