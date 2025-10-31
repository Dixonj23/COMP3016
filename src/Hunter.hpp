#pragma once
#include <raylib.h>
#include <vector>
#include "Combat.hpp"
#include "Player.hpp"
#include "Tilemap.hpp"

struct SquadIntel
{
    Vector2 spot{0, 0};
    float timeToLive = 0.0f;
};

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
    float turnRate = 6.0f;

    // Health
    float maxHp = 120.0f;
    float hp = 120.0f;
    bool isAlive() const { return hp > 0.0f; }
    void takeDamage(float dmg)
    {
        hp -= dmg;
        if (hp < 0.0f)
            hp = 0.0f;
    }

    // rifle
    float shootRange = 560.0f;
    float shootCooldown = 0.6f;
    float burstCooldown = 1.2f;
    int burstSize = 3;
    int burstLeft = 0;
    float shootTimer = 0.0f;

    bool canSeePlayerCone(const Tilemap &world, Vector2 pp) const;
    bool hasFriendlyInLine(const std::vector<Hunter> &squad, int selfIndex,
                           Vector2 start, Vector2 end, float safety) const;
    bool tryShoot(float dt, const Tilemap &world, const Player &player,
                  const std::vector<Hunter> &squad, int selfIndex,
                  std::vector<Bullet> &outBullets);

    // sensing
    float fovDeg = 70.0f;
    float sightRange = 520.0f;
    float facingRad = 0.0f;
    float loseSightTime = 2.0f;
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
    void update(float dt, const Tilemap &world, const Player &player, SquadIntel &intel);
    void draw() const;
    void drawFOV() const;

private:
    void requestPathTo(const Tilemap &world, Vector2 goal);
    void followPath(const Tilemap &world, float dt);
    void pickNewPatrolTarget(const Tilemap &world);
};