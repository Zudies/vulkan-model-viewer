#pragma once

public interface class WindowSurfaceInterface {
public:
    enum class SurfaceType {
        SURFACE_TYPE_WIN32,
    };

    SurfaceType GetType();
};
