#pragma once

#include "base/FrameRateController_Base.h"

namespace Performance {

class WindowsFrameRateControllerImpl {
public:
    WindowsFrameRateControllerImpl();
    ~WindowsFrameRateControllerImpl();

    void Initialize(FrameRateControllerSettings *settings);
    void Finalize();
    f64 GetElapsedTime() const;
    void Reset();
    void Wait(f64 waitTimeSec) const;

private:
    FrameRateControllerSettings m_settings;
    f64 m_inverseFrequency;
    mutable LARGE_INTEGER m_startTime;
};

} // namespace Performance
