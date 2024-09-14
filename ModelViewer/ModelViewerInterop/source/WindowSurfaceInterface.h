#pragma once

namespace Graphics {

class WindowSurface;

}

public interface class WindowSurfaceInterface {
public:
    enum class SurfaceType {
        SURFACE_TYPE_WIN32,
    };


    Graphics::WindowSurface *GetNativeSurface();
    SurfaceType GetType();
};
