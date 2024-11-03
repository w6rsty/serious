#include "serious/graphics/Camera.hpp"

#include <Tracy.hpp>

namespace serious
{

void Camera::Update(float deltaTime)
{
    ZoneScopedN("Camera Update");

    if (this->Moving()) {
        glm::vec3 camFront;
        camFront.x = -cos(glm::radians(m_Rotation.x)) * sin(glm::radians(m_Rotation.y));
        camFront.y = sin(glm::radians(m_Rotation.x));
        camFront.z = cos(glm::radians(m_Rotation.x)) * cos(glm::radians(m_Rotation.y));
        camFront = glm::normalize(camFront);

        float moveSpeed = deltaTime * m_MovementSpeed;

        if (keys.forward)
            m_Position += camFront * moveSpeed;
        if (keys.backward)
            m_Position -= camFront * moveSpeed;
        if (keys.left)
            m_Position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
        if (keys.right)
            m_Position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
        if (keys.up)
            m_Position.y += moveSpeed;
        if (keys.down)
            m_Position.y -= moveSpeed;
    }
    UpdateViewMatrix();
}

void Camera::SetPerspective(float fov_, float aspect_, float znear_, float zfar_)
{
    this->fov = fov_;
    this->aspectRatio = aspect_;
    this->zNear = znear_;
    this->zFar = zfar_;

    matrices.projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
    matrices.projection[1][1] *= -1.0f;
}

bool Camera::Moving() const
{
    return keys.forward || keys.backward || keys.left || keys.right || keys.up || keys.down;
}

void Camera::Translate(const glm::vec3& delta)
{
    this->m_Position += delta;
    UpdateViewMatrix();
}

void Camera::Rotate(const glm::vec3& delta)
{
    this->m_Rotation += delta;
    UpdateViewMatrix();
}

void Camera::SetPosition(const glm::vec3& position)
{
    this->m_Position = position;
    UpdateViewMatrix();
}

void Camera::SetRotation(const glm::vec3& rotation)
{
    this->m_Rotation = rotation;
    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM = glm::mat4(1.0f);

    rotM = glm::rotate(rotM, glm::radians(m_Rotation.x * -1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = m_Position;
    translation.y *= -1.0f;
    transM = glm::translate(transM, translation);
    matrices.view = rotM * transM;
}

}