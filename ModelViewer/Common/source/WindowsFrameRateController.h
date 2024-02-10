#pragma once

#include "base/FrameRateController_Base.h"

namespace Performance {

class WindowsFrameRateControllerImpl;

class WindowsFrameRateController : public FrameRateController_Base {
public:
    WindowsFrameRateController();
    virtual ~WindowsFrameRateController();

    virtual void Initialize(FrameRateControllerSettings *settings) override;
    virtual void Finalize() override;
    virtual f64 GetElapsedTime() const override;
    virtual void Reset() override;
    virtual void Wait(f64 waitTimeSec) const override;

private:
    WindowsFrameRateControllerImpl *m_impl;

};

} // namespace Performance
