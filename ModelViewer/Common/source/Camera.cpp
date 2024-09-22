#include "pch.h"
#include "Camera.h"

#include "glm/gtc/matrix_transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/vector_angle.hpp"

static const f32 EPSILON = 0.001f;

namespace Graphics {

Camera::Camera()
  : m_position(0.0f, 0.0f, 0.0f),
    m_forward(0.0f, 0.0f, 1.0f),
    m_right(1.0f, 0.0f, 0.0f),
    m_verticalFov(glm::radians(90.0f)),
    m_aspectRatio(1.0f),
    m_near(0.01f),
    m_far(std::numeric_limits<f32>::max())
{
}

Camera::~Camera() {
}

void Camera::SetPosition(glm::vec3 newPos) {
    m_position = newPos;
}

void Camera::SetPosition(f32 x, f32 y, f32 z) {
    SetPosition(glm::vec3(x, y, z));
}

glm::vec3 Camera::GetPosition() const {
    return m_position;
}

void Camera::MoveGlobal(glm::vec3 deltaPos) {
    m_position += deltaPos;
}

void Camera::MoveGlobal(f32 x, f32 y, f32 z) {
    MoveGlobal(glm::vec3(x, y, z));
}

void Camera::MoveLocal(glm::vec3 deltaPos) {
    glm::vec3 up = glm::cross(m_right, m_forward);
    m_position += (m_right * deltaPos.x) + (up * deltaPos.y) + (m_forward * deltaPos.z);
}

void Camera::MoveLocal(f32 x, f32 y, f32 z) {
    MoveLocal(glm::vec3(x, y, z));
}

void Camera::SetForward(glm::vec3 forwardDir) {
    m_forward = glm::normalize(forwardDir);
    if (glm::length(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f))) <= EPSILON) {
        m_right = glm::normalize(glm::cross(glm::cross(m_forward, m_right), m_forward));
    }
    else {
        m_right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_forward));
    }
}

void Camera::SetForward(f32 x, f32 y, f32 z) {
    SetForward(glm::vec3(x, y, z));
}

void Camera::LookAt(glm::vec3 lookPos) {
    SetForward(lookPos - m_position);
}

void Camera::LookAt(f32 x, f32 y, f32 z) {
    LookAt(glm::vec3(x, y, z));
}

void Camera::UpdateYawPitchRollRad(f32 yaw, f32 pitch, f32 roll) {
    glm::mat4x4 rotation = glm::yawPitchRoll(yaw, pitch, roll);
    m_forward = glm::normalize(rotation * glm::vec4(m_forward, 0.0f));
    m_right = glm::normalize(rotation * glm::vec4(m_right, 0.0f));
}

void Camera::UpdateYawPitchRollDeg(f32 yaw, f32 pitch, f32 roll) {
    UpdateYawPitchRollRad(glm::radians(yaw), glm::radians(pitch), glm::radians(roll));
}

void Camera::UpdateHorizontalAngleRad(f32 deltaAngle) {
    glm::mat4x4 rotation = glm::rotate(glm::mat4x4(1.0f), deltaAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    m_forward = glm::normalize(rotation * glm::vec4(m_forward, 0.0f));
    m_right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_forward));
}

void Camera::UpdateHorizontalAngleDeg(f32 deltaAngle) {
    UpdateHorizontalAngleRad(glm::radians(deltaAngle));
}

void Camera::UpdateVerticalAngleRad(f32 deltaAngle) {
    // Cap upward and downward rotation to before 180
    if (deltaAngle > 0.0f) {
        f32 curAngle = glm::angle(m_forward, glm::vec3(0.0f, -1.0f, 0.0f));
        deltaAngle = std::min(deltaAngle, curAngle - glm::radians(1.0f));
    }
    else {
        f32 curAngle = glm::angle(m_forward, glm::vec3(0.0f, 1.0f, 0.0f));
        deltaAngle = std::max(deltaAngle, -(curAngle - glm::radians(1.0f)));
    }

    m_forward = glm::normalize(glm::rotate(glm::mat4x4(1.0f), deltaAngle, m_right) * glm::vec4(m_forward, 0.0f));
}

void Camera::UpdateVerticalAngleDeg(f32 deltaAngle) {
    UpdateVerticalAngleRad(glm::radians(deltaAngle));
}

void Camera::SetVerticalFOVRad(f32 fov) {
    m_verticalFov = fov;
}

void Camera::SetVerticalFOVDeg(f32 fov) {
    m_verticalFov = glm::radians(fov);
}

void Camera::SetAspectRatio(f32 width, f32 height) {
    m_aspectRatio = width / height;
}

void Camera::SetNearFarPlanes(f32 nearPlane, f32 farPlane) {
    m_near = nearPlane;
    m_far = farPlane;
}

glm::mat4x4 Camera::ViewMatrix() const {
    glm::mat4x4 view(1.0f);

    glm::vec3 up = glm::cross(m_forward, m_right);
    view[0][0] = m_right.x;
    view[1][0] = m_right.y;
    view[2][0] = m_right.z;
    view[0][1] = up.x;
    view[1][1] = up.y;
    view[2][1] = up.z;
    view[0][2] = m_forward.x;
    view[1][2] = m_forward.y;
    view[2][2] = m_forward.z;
    view[3][0] = -glm::dot(m_right, m_position);
    view[3][1] = -glm::dot(up, m_position);
    view[3][2] = -glm::dot(m_forward, m_position);

    return view;
}

glm::mat4x4 Camera::ProjectionMatrix() const {
    return glm::perspectiveLH(m_verticalFov, m_aspectRatio, m_near, m_far);
}

} // namespace Graphics
