#pragma once
#include <raylib.h>
#include "Tilemap.hpp"

struct Animal
{
    Vector2 pos{};
    Vector2 home{};
    Vector2 target{};
    float speed = 60.0f;
    float radius = 10.0f;
    float roam = 160.0f; // how far from home it roams
    Color color = {220, 180, 60, 255};

    float retargetTimer = 0.0f; // seconds until choosing new target

    void randomise(const Tilemap &world);
    void update(float dt, const Tilemap &world);
    void draw() const;
};