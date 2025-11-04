#pragma once
#include <raylib.h>
#include <vector>
#include "Tilemap.hpp"
#include "Animal.hpp"

class Hunter;
class Player;

struct Boulder
{
    Vector2 pos{};
    Vector2 vel{};
    float radius = 16.0f;
    float life = 2.0f;
    bool alive = true;

    bool update(float dt, const Tilemap &world, std::vector<Animal> &animals, std::vector<Hunter> &hunters, const Player &player);

    void draw() const
    {
        if (!alive)
            return;
        DrawCircleV(pos, radius, Color{120, 110, 100, 255});
        DrawCircleLines((int)pos.x, (int)pos.y, radius, BLACK);
        // motion
        Vector2 tail = {pos.x - vel.x * 0.02f, pos.y - vel.y * 0.02f};
        DrawLineEx(tail, pos, 4.0f, Color{90, 85, 80, 200});
    }
};