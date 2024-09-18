#pragma once

namespace Graphics {

enum class GraphicsError {
    UNKNOWN = -1,
    OK,

    // Device lost
    DEVICE_LOST,

    // Required API version is not supported by hardware
    UNSUPPORTED_API_VERSION,

    // Out of host memory (cpu)
    OUT_OF_HOST_MEMORY,

    // Out of device memory (gpu)
    OUT_OF_DEVICE_MEMORY,

    // Error encountered when initializing
    INITIALIZATION_FAILED,

    // The requested layer(s) is not supported
    NO_SUCH_LAYER,

    // The requested extension(s) is not supported
    NO_SUCH_EXTENSION,

    // No suitable device that could run this application was found
    NO_SUPPORTED_DEVICE,

    // Failed to create swapchain
    SWAPCHAIN_CREATE_ERROR,

    // Swap chain is not valid
    SWAPCHAIN_INVALID,

    // Swap chain is out of date
    SWAPCHAIN_OUT_OF_DATE,

    // Failed to create command pools
    COMMAND_POOL_CREATE_ERROR,

    // Failed to allocate command buffers
    COMMAND_BUFFER_CREATE_ERROR,

    // Failed to create an image
    IMAGE_CREATE_ERROR,

    // Failed to create a render pass
    RENDERPASS_CREATE_ERROR,

    // Memory pool could not be found
    NO_SUPPORTED_MEMORY,

    // Error occurred when recording or submitting command buffers
    QUEUE_ERROR,

    // A device memory transfer failed
    TRANSFER_FAILED,

    // Failed to load a file
    FILE_LOAD_ERROR,

    // An internal format was not supported
    UNSUPPORTED_FORMAT,

};

} // namespace Graphics
