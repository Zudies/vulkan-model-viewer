#pragma once

namespace Graphics {

enum class GraphicsError {
    UNKNOWN = -1,
    OK,

    // Required API version is not supported by hardware
    UNSUPPORTED_API_VERSION,

    // Out of host memory (cpu)
    OUT_OF_HOST_MEMORY,

    // Out of device memory (gpu)
    OUT_OF_DEVICE_MEMORY,

    INITIALIZATION_FAILED,

    // The requested layer(s) is not supported
    NO_SUCH_LAYER,

    // The requested extension(s) is not supported
    NO_SUCH_EXTENSION,

    // No suitable device that could run this application was found
    NO_SUPPORTED_DEVICE,

};

} // namespace Graphics
