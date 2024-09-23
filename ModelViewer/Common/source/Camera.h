#pragma once

namespace Graphics {

class Camera {
    //TODO: Replace vectors with separate math library

public:
    Camera();
    ~Camera();

    // Gets/sets position in world coordinates
    void SetPosition(glm::vec3 newPos);
    void SetPosition(f32 x, f32 y, f32 z);
    glm::vec3 GetPosition() const;

    void MoveGlobal(glm::vec3 deltaPos);
    void MoveGlobal(f32 x, f32 y, f32 z);
    void MoveLocal(glm::vec3 deltaPos);
    void MoveLocal(f32 x, f32 y, f32 z);

    // Sets forward direction in world coordinates
    void SetForward(glm::vec3 forwardDir);
    void SetForward(f32 x, f32 y, f32 z);
    void LookAt(glm::vec3 lookPos);
    void LookAt(f32 x, f32 y, f32 z);

    // Updates look direction
    // Yaw/pitch/roll updates direction in local space
    //TODO: Needs work
    void UpdateYawPitchRollRad(f32 yaw, f32 pitch, f32 roll);
    void UpdateYawPitchRollDeg(f32 yaw, f32 pitch, f32 roll);

    // Horizontal/vertical updates direction in world space
    //   Up axis is fixed on (0, 1, 0)
    //   Right axis is fixed on the horizontal plane but rotates with the camera's horizontal angle
    void UpdateHorizontalAngleRad(f32 deltaAngle);
    void UpdateHorizontalAngleDeg(f32 deltaAngle);
    void UpdateVerticalAngleRad(f32 deltaAngle);
    void UpdateVerticalAngleDeg(f32 deltaAngle);

    // Sets projection matrix parameters
    void SetVerticalFOVRad(f32 fov);
    void SetVerticalFOVDeg(f32 fov);
    void SetAspectRatio(f32 width, f32 height);
    void SetNearFarPlanes(f32 nearPlane, f32 farPlane);

    glm::mat4x4 ViewMatrix() const;
    glm::mat4x4 ProjectionMatrix() const;

private:
    // Camera settings
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_right;

    // Projection matrix settings
    f32 m_verticalFov; // Field of view on the y-axis, stored in radians
    f32 m_aspectRatio; // Desired aspect ratio
    f32 m_near;
    f32 m_far;
};

} // namespace Graphics
