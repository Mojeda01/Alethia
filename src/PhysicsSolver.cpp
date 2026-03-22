#include "PhysicsSolver.h"
#include <algorithm>
#include <cmath>

PhysicsSolver::PhysicsSolver() : cfg{}
{}

PhysicsSolver::PhysicsSolver(const Config& config) : cfg(config)
{}


void PhysicsSolver::step(   PhysicsBody& body,
                            const std::vector<AABB>& world,
                            float deltaSeconds)
{
    if (body.noclip){
        // noclip - no gravity, no collision, free movement
        body.position += body.velocity * deltaSeconds;
        body.onGround = false;
        return;
    }

    applyGravity(body, deltaSeconds);

    // move and resolve one axis at a time 
    body.onGround = false;

    for (int axis = 0; axis < 3; ++axis){
        body.position[axis] += body.velocity[axis] * deltaSeconds;
        resolveAxis(body, world, axis);
    }
}

void PhysicsSolver::applyGravity(PhysicsBody& body, float dt) {
    body.velocity.y += cfg.gravity * dt;
    body.velocity.y = std::max(body.velocity.y, cfg.maxFallSpeed);
}

void PhysicsSolver::resolveAxis(PhysicsBody& body,
                                 const std::vector<AABB>& world,
                                 int axis)
{
    AABB bodyBox = body.getAABB();
    
    for (const AABB& obstacle : world) {
        // broad phase — check overlap on all three axes
        bool overlapX = bodyBox.max.x > obstacle.min.x &&
            bodyBox.min.x < obstacle.max.x;
        bool overlapY = bodyBox.max.y > obstacle.min.y &&
            bodyBox.min.y < obstacle.max.y;
        bool overlapZ = bodyBox.max.z > obstacle.min.z &&
            bodyBox.min.z < obstacle.max.z;

        if (!overlapX || !overlapY || !overlapZ) continue;

        // penetration resolution on the current axis only
        float penetration = 0.0f;

        if (axis == 0) { // X
            if (body.velocity.x > 0.0f) {
                penetration = obstacle.min.x - bodyBox.max.x - cfg.skinWidth;
            } else if (body.velocity.x < 0.0f) {
                penetration = obstacle.max.x - bodyBox.min.x + cfg.skinWidth;
            }
            body.position.x += penetration;
            body.velocity.x = 0.0f;
        } else if (axis == 1) { // Y
            if (body.velocity.y > 0.0f) {
                penetration = obstacle.min.y - bodyBox.max.y - cfg.skinWidth;
            } else if (body.velocity.y < 0.0f) {
                penetration = obstacle.max.y - bodyBox.min.y + cfg.skinWidth;
                body.onGround = true;
            }
            body.position.y += penetration;
            body.velocity.y = 0.0f;
        } else {
            if (body.velocity.z > 0.0f) {
                penetration = obstacle.min.z - bodyBox.max.z - cfg.skinWidth;
            } else if (body.velocity.z < 0.0f) {
                penetration = obstacle.max.z - bodyBox.min.z + cfg.skinWidth;
            }
            body.position.z += penetration;
            body.velocity.z = 0.0f;
        }
        // re-fetch AABB after correction so subsequent obstacles use updated position
        bodyBox = body.getAABB();
    }
}
