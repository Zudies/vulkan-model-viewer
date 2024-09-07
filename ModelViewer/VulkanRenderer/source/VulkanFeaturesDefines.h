#pragma once

namespace Vulkan {

// JSON pointers to the set of required and optional features to be used
// See https://miloyip.github.io/rapidjson/md_doc_pointer.html for formatting rules
static char const JSON_REQ_FEATURES_REQUIRED[] = {
    "/requiredFeatures"
};
static char const JSON_REQ_FEATURES_OPTIONAL[] = {
    "/optionalFeatures"
};
static char const JSON_REQ_USE_VALIDATION[] = {
    "/useValidation"
};

static char const JSON_REQ_SURFACES_INDEX[] = {
    "/surfaces/%d/index"
};
static char const JSON_REQ_SURFACES_FORMATS[] = {
    "/surfaces/%d/preferredFormats"
};
static char const JSON_REQ_SURFACES_COLORSPACES[] = {
    "/surfaces/%d/preferredColorSpaces"
};
static char const JSON_REQ_SURFACES_PRESENT_MODE[] = {
    "/surfaces/%d/preferredPresentModes"
};

// Feature names
static char const *FEATURE_IS_DISCRETE_GPU = "DISCRETE_GPU";
static char const *FEATURE_SUPPORTS_GRAPHICS_OPERATIONS = "GRAPHICS_OPERATIONS";
static char const *FEATURE_SURFACE_WINDOW_PRESENT = "SURFACE_WINDOW_PRESENT";

// List of validation layers that will be enabled if validation is enabled
static char const *VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"
};

// List of validation extensions used with validation layers
static char const *VALIDATION_EXTENSIONS[] = {
    "VK_EXT_debug_utils",
};

// List of surface formats and color spaces
static char const *SURFACE_FORMAT_R8G8B8_UINT = "R8G8B8_UINT";
static char const *SURFACE_FORMAT_R8G8B8_SINT = "R8G8B8_SINT";
static char const *SURFACE_FORMAT_B8G8R8_UNORM = "B8G8R8_UNORM";
static char const *SURFACE_FORMAT_B8G8R8_SNORM = "B8G8R8_SNORM";
static char const *SURFACE_FORMAT_B8G8R8_SRGB = "B8G8R8_SRGB";

static char const *SURFACE_FORMAT_R8G8B8A8_UINT = "R8G8B8A8_UINT";
static char const *SURFACE_FORMAT_R8G8B8A8_SINT = "R8G8B8A8_SINT";
static char const *SURFACE_FORMAT_B8G8R8A8_UNORM = "B8G8R8A8_UNORM";
static char const *SURFACE_FORMAT_B8G8R8A8_SNORM = "B8G8R8A8_SNORM";
static char const *SURFACE_FORMAT_B8G8R8A8_SRGB = "B8G8R8A8_SRGB";

static char const *SURFACE_COLOR_SPACE_SRGB_NONLINEAR = "SRGB_NONLINEAR";

// List of swapchain present modes
static char const *PRESENT_MODE_IMMEDIATE = "IMMEDIATE";
static char const *PRESENT_MODE_FIFO = "FIFO";
static char const *PRESENT_MODE_FIFO_RELAXED = "FIFO_RELAXED";
static char const *PRESENT_MODE_MAILBOX = "MAILBOX";

} // namespace Vulkan
