#pragma once

namespace Performance {

struct FrameRateControllerSettings {
    f64 DesiredFrameTime;
};

class FrameRateController_Base {
public:

    FrameRateController_Base();
    FrameRateController_Base(FrameRateController_Base const &) = delete;
    FrameRateController_Base &operator=(FrameRateController_Base const &) = delete;
    virtual ~FrameRateController_Base() = 0;

    virtual void Initialize(FrameRateControllerSettings *settings) = 0;
    virtual void Finalize() = 0;
    virtual f64 GetElapsedTime() const = 0;
    virtual void Reset() = 0;
    virtual void Wait(f64 waitTimeSec) const = 0;

};

} // namespace Performance
