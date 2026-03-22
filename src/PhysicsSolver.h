#pragma once

#include "PhysicsBody.h"
#include "AABB.h"
#include <glm/glm.hpp>
#include <vector>

class PhysicsSolver{
public:
    struct Config{
        float gravity = -18.0f;             // m/s^2 downward
        float maxFallSpeed = -50.0f;        // terminal velocity
        float skinWidth = 0.005f;           // small gap to prevent z-fighting with surface.
    };

    PhysicsSolver();
    explicit PhysicsSolver(const Config& config);

    // advances the body one timestep against the world geometry
    void step(  PhysicsBody& body,
                const std::vector<AABB>& world,
                float deltaSeconds);

    const Config& config() const { return cfg; }
    Config& config() { return cfg; }
private:
    void applyGravity(PhysicsBody& body, float dt);
    void resolveAxis(PhysicsBody& body,
                        const std::vector<AABB>& world,
                        int axis);
    Config cfg;
};
