#pragma once

#include "WindowSurfaceTypes.h"

namespace Graphics {

class WindowSurface {
public:
    virtual ~WindowSurface() = 0;
    virtual bool Compare(WindowSurface const &other) const = 0;

    // Use enum type to avoid RTTI when doing comparisons
    virtual WindowSurfaceType GetType() const = 0;
};

} // namespace Graphics
#pragma once
