#pragma once

#include "PhysicsBody.h"
#include "PhysicsSolver.h"
#include "InputManager.h"
#include "SceneObject.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <vector>

class PlayerController{
public:
    struct Config{
        float walkSpeed     = 4.0f;
        float sprintSpeed   = 8.0f;
        float jumpImpulse   = 7.0f;
        float eyeHeight     = 1.65f;
        float bodyWidth     = 0.6f;
        float bodyHeight    = 1.8f;
        float mouseSensitivity = 0.1f;
        float noclipSpeed   = 10.0f;
    };
    
    PlayerController();
    explicit PlayerController(const Config& config);

    // call once per frame in play mode.
    void update(    const InputManager& input,
                    PhysicsSolver& solver,
                    const std::vector<SceneObject>& world,
                    float deltaSeconds);
    // feeds the play camera
    glm::vec3 eyePosition() const;
    float yaw() const { return yawAngle; }
    float pitch() const { return pitchAngle; }

    // spawn point — call when entering play mode
    void setPosition(const glm::vec3& feetPosition);
    const PhysicsBody& body() const { return physicsBody; }

    // Config is public so it can be tweaked
    Config cfg;
private:
    void handleLook(const InputManager& input);
    void handleMovement(const InputManager& input, float deltaSeconds);
    void handleJump(const InputManager& input);
    void handleNoclipToggle(const InputManager& input);

    PhysicsBody physicsBody;
    float yawAngle = -90.0f;
    float pitchAngle = 0.0f;
};
