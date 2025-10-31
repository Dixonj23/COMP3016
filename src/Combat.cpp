#include "Combat.hpp"
#include "Tilemap.hpp"

void Bullet::update(float dt, const Tilemap &world)
{
    if (!alive)
        return;

    const int STEPS = 2;
    Vector2 step = {vel.x * dt / STEPS, vel.y * dt / STEPS};

    for (int i = 0; i < STEPS && alive; ++i)
    {
        pos.x += step.x;
        pos.y += step.y;

        // wall collision kills bullet
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
                    alive = false;
                    return;
                }
            }
        }
    }
}

void Bullet::draw() const
{
    if (!alive)
        return;
    DrawCircleV(pos, radius, (team == Team::Hunter) ? YELLOW : GREEN);
}