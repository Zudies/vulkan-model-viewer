#include "pch.h"
#include "CameraInputController.h"

static const uint32_t FLAG_INPUT_FORWARD = 0x0001;
static const uint32_t FLAG_INPUT_BACKWARD = 0x0002;
static const uint32_t FLAG_INPUT_RIGHT = 0x0004;
static const uint32_t FLAG_INPUT_LEFT = 0x0008;
static const uint32_t FLAG_INPUT_UP = 0x000F;
static const uint32_t FLAG_INPUT_DOWN = 0x0010;
static const uint32_t FLAG_FIXED_UP_DIR = 0x0020;
static const uint32_t FLAG_ACTIVE = 0x0040;

CameraInputController::CameraInputController()
  : m_inputState(0) {
    m_lock = gcnew ReaderWriterLockSlim(LockRecursionPolicy::SupportsRecursion);
}

CameraInputController::~CameraInputController() {
}

void CameraInputController::SetFixedUpDir(bool isFixedUp) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_FIXED_UP_DIR, isFixedUp);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetSpeed(float speed) {
    m_lock->EnterWriteLock();
    m_speed = speed;
    m_lock->ExitWriteLock();
}

void CameraInputController::SetSensitivityX(float sensitivity) {
    m_lock->EnterWriteLock();
    m_sensitivityX = sensitivity;
    m_lock->ExitWriteLock();
}

void CameraInputController::SetSensitivityY(float sensitivity) {
    m_lock->EnterWriteLock();
    m_sensitivityY = sensitivity;
    m_lock->ExitWriteLock();
}

void CameraInputController::BeginControl(int x, int y) {
    m_lock->EnterWriteLock();
    _clearFlag(
        FLAG_INPUT_FORWARD |
        FLAG_INPUT_BACKWARD |
        FLAG_INPUT_RIGHT |
        FLAG_INPUT_LEFT |
        FLAG_INPUT_UP |
        FLAG_INPUT_DOWN);

    _setFlag(FLAG_ACTIVE, true);
    m_startPos.x = static_cast<float>(x);
    m_startPos.y = static_cast<float>(y);
    m_curPos.x = static_cast<float>(x);
    m_curPos.y = static_cast<float>(y);
    m_lock->ExitWriteLock();
}

void CameraInputController::EndControl(int x, int y) {
    m_lock->EnterWriteLock();
    _clearFlag(FLAG_ACTIVE |
        FLAG_INPUT_FORWARD |
        FLAG_INPUT_BACKWARD |
        FLAG_INPUT_RIGHT |
        FLAG_INPUT_LEFT |
        FLAG_INPUT_UP |
        FLAG_INPUT_DOWN);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMousePos(int x, int y) {
    m_lock->EnterWriteLock();
    m_curPos.x = static_cast<float>(x);
    m_curPos.y = static_cast<float>(y);
    m_lock->ExitWriteLock();
}

void CameraInputController::ResetMousePos() {
    m_lock->EnterWriteLock();
    m_curPos.x = m_startPos.x;
    m_curPos.y = m_startPos.y;

    SetCursorPos(static_cast<int>(m_startPos.x), static_cast<int>(m_startPos.y));
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMovementInputForward(bool isSet) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_INPUT_FORWARD, isSet);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMovementInputBackward(bool isSet) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_INPUT_BACKWARD, isSet);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMovementInputRight(bool isSet) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_INPUT_RIGHT, isSet);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMovementInputLeft(bool isSet) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_INPUT_LEFT, isSet);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMovementInputUp(bool isSet) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_INPUT_UP, isSet);
    m_lock->ExitWriteLock();
}

void CameraInputController::SetMovementInputDown(bool isSet) {
    m_lock->EnterWriteLock();
    _setFlag(FLAG_INPUT_DOWN, isSet);
    m_lock->ExitWriteLock();
}

bool CameraInputController::IsActive() {
    m_lock->EnterReadLock();
    bool ret = _getFlag(FLAG_ACTIVE);
    m_lock->ExitReadLock();
    return ret;
}

bool CameraInputController::IsFixedUpDir() {
    m_lock->EnterReadLock();
    bool ret = _getFlag(FLAG_FIXED_UP_DIR);
    m_lock->ExitReadLock();
    return ret;
}

MousePos CameraInputController::GetMouseDelta() {
    MousePos ret;

    m_lock->EnterReadLock();
    ret.x = (m_curPos.x - m_startPos.x) * m_sensitivityX;
    ret.y = (m_curPos.y - m_startPos.y) * m_sensitivityY;
    m_lock->ExitReadLock();

    return ret;
}

MovementValue CameraInputController::GetMovement() {
    MovementValue ret;

    m_lock->EnterReadLock();
    if (_getFlag(FLAG_INPUT_FORWARD)) {
        ret.z += m_speed;
    }
    if (_getFlag(FLAG_INPUT_BACKWARD)) {
        ret.z -= m_speed;
    }
    if (_getFlag(FLAG_INPUT_RIGHT)) {
        ret.x += m_speed;
    }
    if (_getFlag(FLAG_INPUT_LEFT)) {
        ret.x -= m_speed;
    }
    if (_getFlag(FLAG_INPUT_UP)) {
        ret.y += m_speed;
    }
    if (_getFlag(FLAG_INPUT_DOWN)) {
        ret.y -= m_speed;
    }
    m_lock->ExitReadLock();

    return ret;
}

void CameraInputController::_setFlag(uint32_t flag, bool set) {
    if (set) {
        m_inputState |= flag;
    }
    else {
        _clearFlag(flag);
    }
}

void CameraInputController::_clearFlag(uint32_t flag) {
    m_inputState &= ~flag;
}

bool CameraInputController::_getFlag(uint32_t flag) {
    return (m_inputState & flag) == flag;
}
