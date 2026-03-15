#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct GLFWwindow;

class Camera{
public:
    Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);

    void processKeyboard(GLFWwindow* window, float deltaSeconds);
    void processMouse(double xOffset, double yOffset);
    void setAspectRatio(float aspectRatio);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
 
    glm::vec3 position() const { return pos; }
    float yaw() const { return yawAngle; }
    float pitch() const { return pitchAngle; }

private:
    void updateVectors();

    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

    float yawAngle = -90.0f;
    float pitchAngle = 0.0f;

    float fov;
    float aspect;
    float nearZ;
    float farZ;

    float moveSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
};
