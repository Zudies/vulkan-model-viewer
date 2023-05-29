#include "pch.h"
#include "WindowsFrameRateController.h"
#include "WindowsFrameRateControllerImpl.h"

namespace Performance {

WindowsFrameRateController::WindowsFrameRateController()
  : FrameRateController_Base(),
    m_impl(nullptr) {
}

WindowsFrameRateController::~WindowsFrameRateController() {
    delete m_impl;
}

void WindowsFrameRateController::Initialize(FrameRateControllerSettings *settings) {
    ASSERT(!m_impl);

    m_impl = new WindowsFrameRateControllerImpl;
    return m_impl->Initialize(settings);
}

void WindowsFrameRateController::Finalize() {
    ASSERT(m_impl);
    return m_impl->Finalize();
}

f64 WindowsFrameRateController::GetElapsedTime() const {
    ASSERT(m_impl);
    return m_impl->GetElapsedTime();
}

void WindowsFrameRateController::Reset() {
    ASSERT(m_impl);
    m_impl->Reset();
}

void WindowsFrameRateController::Wait(f64 waitTimeSec) const {
    ASSERT(m_impl);
    m_impl->Wait(waitTimeSec);
}

} // namespace Performance
