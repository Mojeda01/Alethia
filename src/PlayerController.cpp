#include "PlayerController.h"
#include "Log.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

PlayerController::PlayerController()
    : cfg{}
{}

PlayerController::PlayerController(const Config& config) : cfg(config)
{
    physicsBody.size = glm::vec3(cfg.bodyWidth, cfg.bodyHeight, cfg.bodyWidth); 
}

void PlayerController::setPosition(const glm::vec3& feetPosition) {
    physicsBody.position = feetPosition;
    physicsBody.velocity = glm::vec3(0.0f);
    physicsBody.onGround = false;
}

glm::vec3 PlayerController::eyePosition() const {
    return physicsBody.position + glm::vec3(0.0f, cfg.eyeHeight, 0.0f);
}

void PlayerController::update( const InputManager& input,
                                PhysicsSolver& solver,
                                const std::vector<AABB>& world,
                                float deltaSeconds)
{
    handleNoclipToggle(input);
    handleLook(input);
    handleMovement(input, deltaSeconds);
    handleJump(input);
    solver.step(physicsBody, world, deltaSeconds);
}

void PlayerController::handleNoclipToggle(const InputManager& input) {
    if (input.wasKeyJustPressed(GLFW_KEY_F) && !input.imguiWantsKeyboard()) { 
        physicsBody.noclip = !physicsBody.noclip;
        physicsBody.velocity = glm::vec3(0.0f);
        Log::info(physicsBody.noclip ? "Noclip ON" : "Noclip OFF");
    }
}

void PlayerController::handleLook(const InputManager& input) {
    yawAngle += static_cast<float>(input.mouseDeltaX()) * cfg.mouseSensitivity;
    pitchAngle -= static_cast<float>(input.mouseDeltaY()) * cfg.mouseSensitivity; 
    pitchAngle = std::clamp(pitchAngle, -89.0f, 89.0f); 
}

void PlayerController::handleMovement(const InputManager& input, float deltaSeconds) 
{
    // compute forward and right from yaw only (no pitch influence on movement)
    float yawRad = glm::radians(yawAngle);
    glm::vec3 forward(std::cos(yawRad), 0.0f, std::sin(yawRad));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    float speed = ( input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || 
                    input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT))
                    ? cfg.sprintSpeed : cfg.walkSpeed;

    if (physicsBody.noclip){
        // noclip - fly in 3D
        float pitchRad = glm::radians(pitchAngle); 
        glm::vec3 fullForward(
            std::cos(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            std::sin(yawRad) * std::cos(pitchRad)
        );

        glm::vec3 moveDir(0.0f);
        if (input.isKeyPressed(GLFW_KEY_W)) moveDir += fullForward;
        if (input.isKeyPressed(GLFW_KEY_S)) moveDir -= fullForward;
        if (input.isKeyPressed(GLFW_KEY_A)) moveDir -= right;
        if (input.isKeyPressed(GLFW_KEY_D)) moveDir += right;

        if (glm::length(moveDir) > 0.001f) {
            moveDir = glm::normalize(moveDir);
        }
        physicsBody.velocity = moveDir * cfg.noclipSpeed;
        return;
    }

    // ground movement - horizontal only.
    
    glm::vec3 moveDir(0.0f); 
    if (input.isKeyPressed(GLFW_KEY_W)) moveDir += forward;
    if (input.isKeyPressed(GLFW_KEY_S)) moveDir -= forward;
    if (input.isKeyPressed(GLFW_KEY_A)) moveDir -= right;
    if (input.isKeyPressed(GLFW_KEY_D)) moveDir += right;

    if (glm::length(moveDir) > 0.001f){
        moveDir = glm::normalize(moveDir);
    }

    physicsBody.velocity.x = moveDir.x * speed;
    physicsBody.velocity.z = moveDir.z * speed;
    // velocity.y is owned by gravity — do not overwrite it here
}

void PlayerController::handleJump(const InputManager& input) {
    if (physicsBody.noclip) return;
    if (physicsBody.onGround &&
            input.wasKeyJustPressed(GLFW_KEY_SPACE) &&
            !input.imguiWantsKeyboard()) {
        physicsBody.velocity.y = cfg.jumpImpulse;
        physicsBody.onGround = false;
        Log::info("Jump");
    }
}
