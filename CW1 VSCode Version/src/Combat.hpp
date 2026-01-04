#pragma once
#include <raylib.h>

enum class Team
{
    Player,
    Hunter
};

struct Bullet
{
    Vector2 pos{};
    Vector2 vel{};
    float radius = 4.0f;
    float damage = 12.0f;
    Team team = Team::Hunter;
    bool alive = true;

    void update(float dt, const class Tilemap &world);
    void draw() const;
};