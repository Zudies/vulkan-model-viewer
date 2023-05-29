#include "pch.h"
#include "WindowsFrameRateControllerImpl.h"
#include <thread>
#include <chrono>

namespace Performance {

WindowsFrameRateControllerImpl::WindowsFrameRateControllerImpl()
  : m_settings({}),
    m_inverseFrequency(0.0),
    m_startTime({}) {
}

WindowsFrameRateControllerImpl::~WindowsFrameRateControllerImpl() {
}

void WindowsFrameRateControllerImpl::Initialize(FrameRateControllerSettings *settings) {
    m_settings = *settings;

    LARGE_INTEGER frequency;
    auto result = QueryPerformanceFrequency(&frequency);
    ASSERT(result);

    m_inverseFrequency = 1.0 / frequency.QuadPart;
    Reset();
}

void WindowsFrameRateControllerImpl::Finalize() {
}

f64 WindowsFrameRateControllerImpl::GetElapsedTime() const {
    LARGE_INTEGER endTime, elapsedTicks;
    auto result = QueryPerformanceCounter(&endTime);
    ASSERT(result);
    elapsedTicks.QuadPart = endTime.QuadPart - m_startTime.QuadPart;
    m_startTime = endTime;

    return elapsedTicks.QuadPart * m_inverseFrequency;
}

void WindowsFrameRateControllerImpl::Reset() {
    auto result = QueryPerformanceCounter(&m_startTime);
    ASSERT(result);
}

void WindowsFrameRateControllerImpl::Wait(f64 waitTimeSec) const {
    if (waitTimeSec > 0.0) {
        std::this_thread::sleep_for(std::chrono::duration<double>(waitTimeSec));
    }
}

} // namespace Performance
