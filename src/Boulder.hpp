#pragma once
#include <raylib.h>
#include <vector>
#include "Tilemap.hpp"
#include "Animal.hpp"

struct Boulder
{
    Vector2 pos{};
    Vector2 vel{};
    float radius = 16.0f;
    float life = 2.0f;
    bool alive = true;

    bool update(float dt, const Tilemap &world, std::vector<Animal> &animals)
    {
        if (!alive)
            return false;

        // move in small steps
        const int steps = 2;
        Vector2 step = {vel.x * dt / steps, vel.y * dt / steps};
        bool exploded = false;

        for (int i = 0; i < steps && alive; ++i)
        {
            pos.x += step.x;
            pos.y += step.y;

            // kill animals on contact
            for (auto &a : animals)
            {
                if (!a.alive)
                    continue;
                float dx = a.pos.x - pos.x, dy = a.pos.y - pos.y;
                float rr = (radius + a.radius);
                if (dx * dx + dy * dy <= rr * rr)
                    a.alive = false;
            }

            // check collision with walls, explode on impact
            int minTx = (int)((pos.x - radius) / Tilemap::TILE_SIZE);
            int minTy = (int)((pos.y - radius) / Tilemap::TILE_SIZE);
            int maxTx = (int)((pos.x + radius) / Tilemap::TILE_SIZE);
            int maxTy = (int)((pos.y + radius) / Tilemap::TILE_SIZE);
            for (int ty = minTy; ty <= maxTy; ++ty)
            {
                for (int tx = minTx; tx <= maxTx; ++tx)
                {
                    if (!world.isWall(tx, ty))
                        continue;
                    Rectangle t{(float)tx * Tilemap::TILE_SIZE, (float)ty * Tilemap::TILE_SIZE,
                                (float)Tilemap::TILE_SIZE, (float)Tilemap::TILE_SIZE};

                    if (CheckCollisionCircleRec(pos, radius, t))
                    {
                        exploded = true;
                        alive = false;
                        break;
                    }
                }

                if (!alive)
                    break;
            }
        };

        life -= dt;
        if (life <= 0.0f)
            alive = false;
        return exploded;
    };

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