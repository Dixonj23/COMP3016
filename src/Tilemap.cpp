#include "Tilemap.hpp"
#include <cmath>

void Tilemap::loadExampleMap()
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            bool border = (x == 0 || y == 0 || x == WIDTH - 1 || y == HEIGHT - 1);
            bool wall = border;

            // Add some random internal walls
            if (!border && GetRandomValue(0, 100) < 10)
            { // 10% chance
                wall = true;
            }

            map[y][x] = wall ? 1 : 0;
        }
    }
}

bool Tilemap::isWall(int tx, int ty) const
{
    if (tx < 0 || ty < 0 || tx >= WIDTH || ty >= HEIGHT)
        return true;
    return map[ty][tx] == 1;
}

void Tilemap::draw() const
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            Rectangle r{(float)x * TILE_SIZE, (float)y * TILE_SIZE,
                        (float)TILE_SIZE, (float)TILE_SIZE};
            if (map[y][x])
            {
                DrawRectangleRec(r, Color{30, 45, 55, 255});
            }
            else
            {
                DrawRectangleRec(r, Color{18, 120, 100, 255});
            }
        }
    }
}

// basic collision function
void Tilemap::resolveCollision(Vector2 &pos, float radius, Vector2 delta)
{
    pos.x += delta.x;
    pos.y += delta.y;

    int tx = (int)(pos.x / TILE_SIZE);
    int ty = (int)(pos.y / TILE_SIZE);
    if (isWall(tx, ty))
    {
        pos.x -= delta.x;
        pos.y -= delta.y;
    }
}
