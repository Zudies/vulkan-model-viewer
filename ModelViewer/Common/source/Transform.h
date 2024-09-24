#pragma once

namespace Graphics {

// A simple transform containing translation, rotation, and scaling
class Transform {
public:
    Transform();
    ~Transform();

    void SetTranslation(glm::vec3 newTranslate);
    void SetTranslation(f32 x, f32 y, f32 z);
    glm::vec3 GetTranslation() const;

    void SetRotation(glm::vec3 newRotate);
    void SetRotation(f32 x, f32 y, f32 z);
    void SetRotation(glm::quat newRotate);
    glm::vec3 GetRotation() const;

    void SetScale(glm::vec3 newScale);
    void SetScale(f32 x, f32 y, f32 z);
    glm::vec3 GetScale() const;

    glm::mat4x4 GetTransformMatrix() const;

private:
    glm::vec3 m_translation;
    glm::vec3 m_eulerRotation;
    glm::vec3 m_scale;
};

} // namespace Graphics
