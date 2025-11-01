#include "Boulder.hpp"
#include "Tilemap.hpp"
#include "Hunter.hpp"
#include "Player.hpp"
#include <cmath>

bool Boulder::update(float dt, const Tilemap &world, std::vector<Animal> &animals, std::vector<Hunter> &hunters, const Player &player)
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

        for (auto &h : hunters)
        {
            if (!h.isAlive())
                continue;
            float hx = h.pos.x - pos.x, hy = h.pos.y - pos.y;
            if (hx * hx + hy * hy <= radius * radius)
            {
                h.applyHit(player.boulderDirectDamage(), pos, 180.0f);
            }
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