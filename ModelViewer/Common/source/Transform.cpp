#include "pch.h"
#include "Transform.h"

#include "glm/ext/quaternion_common.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"

namespace Graphics {

Transform::Transform()
  : m_translation(0.0f),
    m_eulerRotation(0.0f),
    m_scale(1.0f)
{
}

Transform::~Transform() {

}

void Transform::SetTranslation(glm::vec3 newTranslate) {
    m_translation = newTranslate;
}

void Transform::SetTranslation(f32 x, f32 y, f32 z) {
    m_translation.x = x;
    m_translation.y = y;
    m_translation.z = z;
}

glm::vec3 Transform::GetTranslation() const {
    return m_translation;
}

void Transform::SetRotation(glm::vec3 newRotate) {
    m_eulerRotation = newRotate;
}

void Transform::SetRotation(f32 x, f32 y, f32 z) {
    m_eulerRotation.x = x;
    m_eulerRotation.y = y;
    m_eulerRotation.z = z;
}

void Transform::SetRotation(glm::quat newRotate) {
    m_eulerRotation = glm::eulerAngles(newRotate);
}

glm::vec3 Transform::GetRotation() const {
    return m_eulerRotation;
}

void Transform::SetScale(glm::vec3 newScale) {
    m_scale = newScale;
}

void Transform::SetScale(f32 x, f32 y, f32 z) {
    m_scale.x = x;
    m_scale.y = y;
    m_scale.z = z;
}

glm::vec3 Transform::GetScale() const {
    return m_scale;
}

glm::mat4x4 Transform::GetTransformMatrix() const {
    return glm::translate(glm::mat4x4(1.0f), m_translation)
        * glm::yawPitchRoll(m_eulerRotation.y, m_eulerRotation.x, m_eulerRotation.z)
        * glm::scale(glm::mat4x4(1.0f), m_scale);

}

} // namespace Graphics
