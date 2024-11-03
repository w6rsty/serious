#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

namespace serious
{

class Camera
{
public:
    Camera() = default;

    void Update(float deltaTime);
    void SetPerspective(float fov, float aspect, float znear, float zfar);
    bool Moving() const;
    void Translate(const glm::vec3& delta);
    void Rotate(const glm::vec3& delta);
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    inline void SetMovementSpeed(float speed) { m_MovementSpeed = speed; }
    inline void SetRotationSpeed(float speed) { m_RotationSpeed = speed; }
private:
    void UpdateViewMatrix();
private:
    float m_RotationSpeed = 1.0f;
    float m_MovementSpeed = 1.0f;
    glm::vec3 m_Rotation = glm::vec3();
    glm::vec3 m_Position = glm::vec3();
public:
    float fov = 45.0f;
    float zNear = 0.01f;
    float zFar = 1000.0f; 
    float aspectRatio = 800.0f / 600.0f;
    struct {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    } keys;

    struct {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    } matrices;
}; 

}