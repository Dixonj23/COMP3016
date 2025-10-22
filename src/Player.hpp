#pragma once
#include <raylib.h>
#include "Tilemap.hpp"

class Player
{
public:
    Player(Vector2 startPos);
    void update(float dt, Tilemap &world, const Camera2D &cam);
    void draw() const;
    Vector2 getPosition() const { return pos; }

private:
    Vector2 pos;
    float angle = 0.0f;
    const float speed = 180.0f;
};