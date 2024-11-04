#include "serious/graphics/Camera.hpp"

#include <Tracy.hpp>

namespace serious {

void Camera::Update(float deltaTime) {
    ZoneScopedN("Camera Update");

    if (Moving()) {
        UpdateCameraPosition(deltaTime);
        UpdateViewMatrix();
    }
}

void Camera::UpdateCameraPosition(float deltaTime) {
    glm::vec3 camFront = CalculateFrontVector();
    float moveSpeed = deltaTime * m_MovementSpeed;

    // 使用方向键状态更新位置
    m_Position += (float)(keys.backward - keys.forward) * camFront * moveSpeed;
    m_Position += (float)(keys.left - keys.right) * glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
    m_Position.y += (float)(keys.up - keys.down) * moveSpeed;
}

glm::vec3 Camera::CalculateFrontVector() const {
    glm::vec3 camFront;
    camFront.x = -cos(glm::radians(m_Rotation.x)) * sin(glm::radians(m_Rotation.y));
    camFront.y = sin(glm::radians(m_Rotation.x));
    camFront.z = cos(glm::radians(m_Rotation.x)) * cos(glm::radians(m_Rotation.y));
    return glm::normalize(camFront);
}

void Camera::SetPerspective(float fov_, float aspect_, float znear_, float zfar_) {
    fov = fov_;
    aspectRatio = aspect_;
    zNear = znear_;
    zFar = zfar_;

    matrices.projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
    matrices.projection[1][1] *= -1.0f;
}

bool Camera::Moving() const {
    return keys.forward || keys.backward || keys.left || keys.right || keys.up || keys.down;
}

void Camera::Translate(const glm::vec3& delta) {
    m_Position += delta;
    UpdateViewMatrix();
}

void Camera::Rotate(const glm::vec3& delta) {
    m_Rotation += delta * m_RotationSpeed;
    UpdateViewMatrix();
}

void Camera::SetPosition(const glm::vec3& position) {
    m_Position = position;
    UpdateViewMatrix();
}

void Camera::SetRotation(const glm::vec3& rotation) {
    m_Rotation = rotation;
    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix() {
    glm::mat4 rotM(1.0f), transM(1.0f);

    rotM = glm::rotate(rotM, glm::radians(-m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    transM = glm::translate(glm::mat4(1.0f), -m_Position);
    matrices.view = rotM * transM;
}

}  // namespace serious
