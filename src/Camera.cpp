#include "Camera.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

Camera::Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane)
    : fov(fovDegrees)
    , aspect(aspectRatio)
    , nearZ(nearPlane)
    , farZ(farPlane)
{
    updateVectors();
}

void Camera::processKeyboard(GLFWwindow* window, float deltaSeconds) {
    float velocity = moveSpeed * deltaSeconds;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        pos += front * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        pos -= front * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        pos -= right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        pos += right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        pos += up * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        pos -= up * velocity;
    }
}

void Camera::processMouse(double xOffset, double yOffset) {
    yawAngle += static_cast<float>(xOffset) * mouseSensitivity;
    pitchAngle -= static_cast<float>(yOffset) * mouseSensitivity;
    pitchAngle = std::clamp(pitchAngle, -89.0f, 89.0f);
    updateVectors();
}

void Camera::setAspectRatio(float aspectRatio) {
    aspect = aspectRatio;
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 Camera::projectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, nearZ, farZ);
    proj[1][1] *= -1.0f;
    return proj;
}

void Camera::updateVectors() {
    float yawRad = glm::radians(yawAngle);
    float pitchRad = glm::radians(pitchAngle);

    glm::vec3 direction;
    direction.x = std::cos(yawRad) * std::cos(pitchRad);
    direction.y = std::sin(pitchRad);
    direction.z = std::sin(yawRad) * std::cos(pitchRad);

    front = glm::normalize(direction);
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}
