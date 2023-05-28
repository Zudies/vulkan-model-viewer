#include "pch.h"
#include "VulkanAPIImpl.h"

namespace Vulkan {

APIImpl::APIImpl() {
}

APIImpl::~APIImpl() {
}

Graphics::GraphicsError APIImpl::Initialize() {
    // This is where the API would normally be loaded, but we're using the static lib
    //  this time instead of the dll so no extra work needs to be done to load function
    //  pointers

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError APIImpl::Finalize() {
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
