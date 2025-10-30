#pragma once
#include <raylib.h>
#include <vector>
#include "Tilemap.hpp"
#include "Player.hpp" //for position

class Hunter
{
public:
    enum class State
    {
        Patrol,
        Chase,
        Search
    };

    Vector2 pos{};
    float radius = 12.0f;
    float speed = 120.0f;

    // sensing
    float fovDeg = 70.0f;
    float sightRange = 520.0f;
    float facingRad = 0.0f;
    float loseSightTime = 1.0f;
    float memory = 0.0f;
    Vector2 lastSeen{};

    // Pathing
    std::vector<Vector2> path;
    int pathIndex = 0;
    float repathTimer = 0.0f;
    float repathInterval = 0.25;

    // behaviour/states
    State state = State::Patrol;
    Vector2 patrolHome{};
    float patrolRadius = 220.0f;
    float retargetTimer = 0.0f;

    void spawnAt(const Tilemap &world, Vector2 p);
    void update(float dt, const Tilemap &world, const Player &player);
    void draw() const;
    void drawFOV() const;

private:
    void requestPathTo(const Tilemap &world, Vector2 goal);
    void followPath(const Tilemap &world, float dt);
    void pickNewPatrolTarget(const Tilemap &world);
};