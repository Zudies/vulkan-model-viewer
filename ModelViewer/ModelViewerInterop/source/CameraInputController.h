#pragma once

using namespace System::Threading;

public ref struct MousePos {
    float x;
    float y;

    MousePos() : x(0.0f), y(0.0f) {}
    MousePos(MousePos %rhs) : x(rhs.x), y(rhs.y) {}
};

// Direction is positive-negative: x = right-left, y = up-down, z = forward-back
public ref struct MovementValue {
    float x;
    float y;
    float z;

    MovementValue() : x(0.0f), y(0.0f), z(0.0f) {}
    MovementValue(MovementValue %rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {}
};

public ref class CameraInputController {
public:
    CameraInputController();
    ~CameraInputController();

    // Settings
    void SetFixedUpDir(bool isFixedUp);
    void SetSpeed(float speed);
    void SetSensitivityX(float sensitivity);
    void SetSensitivityY(float sensitivity);

    // Active control
    void BeginControl(int x, int y);
    void EndControl(int x, int y);

    void SetMousePos(int x, int y);
    void ResetMousePos();

    void SetMovementInputForward(bool isSet);
    void SetMovementInputBackward(bool isSet);
    void SetMovementInputRight(bool isSet);
    void SetMovementInputLeft(bool isSet);
    void SetMovementInputUp(bool isSet);
    void SetMovementInputDown(bool isSet);

    // Camera updates
    bool IsActive();
    bool IsFixedUpDir();
    MousePos GetMouseDelta();
    MovementValue GetMovement();

private:
    void _setFlag(uint32_t flag, bool set);
    void _clearFlag(uint32_t flag);
    bool _getFlag(uint32_t flag);

private:
    uint32_t m_inputState;
    float m_speed;
    float m_sensitivityX;
    float m_sensitivityY;

    MousePos m_startPos;
    MousePos m_curPos;

    ReaderWriterLockSlim ^m_lock;

};
