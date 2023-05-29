#pragma once

#include "base/FrameRateController_Base.h"

namespace Performance {

class WindowsFrameRateControllerImpl;

class WindowsFrameRateController : public FrameRateController_Base {
public:
    WindowsFrameRateController();
    virtual ~WindowsFrameRateController();

    virtual void Initialize(FrameRateControllerSettings *settings);
    virtual void Finalize();
    virtual f64 GetElapsedTime() const;
    virtual void Reset();
    virtual void Wait(f64 waitTimeSec) const;

private:
    WindowsFrameRateControllerImpl *m_impl;

};

} // namespace Performance
