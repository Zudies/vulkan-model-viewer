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

// Feature names
static char const *FEATURE_IS_DISCRETE_GPU = "DISCRETE_GPU";
static char const *FEATURE_SUPPORTS_GRAPHICS_OPERATIONS = "GRAPHICS_OPERATIONS";

// List of validation layers that will be enabled if validation is enabled
static char const *VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"
};

// List of validation extensions used with validation layers
static char const *VALIDATION_EXTENSIONS[] = {
    "VK_EXT_debug_utils",
};

} // namespace Graphics
