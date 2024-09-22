#pragma once

static const std::pair<const char*, const char*> ContentNameMapping[] = {
    { "ID_POLYGON_MODE", "rasterizer.polygonMode" },
    { "ID_CULL_MODE", "rasterizer.cullMode" },
};

static const std::pair<const char*, const char*> ContentValueMapping[] = {
    { "PolygonModeFill", "VK_POLYGON_MODE_FILL" },
    { "PolygonModeLine", "VK_POLYGON_MODE_LINE" },
    { "PolygonModePoint", "VK_POLYGON_MODE_POINT" },
    { "CullModeNone", "VK_CULL_MODE_NONE" },
    { "CullModeFront", "VK_CULL_MODE_FRONT_BIT" },
    { "CullModeBack", "VK_CULL_MODE_BACK_BIT" },
    { "CullModeFrontAndBack", "VK_CULL_MODE_FRONT_AND_BACK" },
};