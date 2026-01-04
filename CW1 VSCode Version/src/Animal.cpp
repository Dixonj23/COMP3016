#include "Animal.hpp"
#include <cmath>

static inline float len2(Vector2 v) { return v.x * v.x + v.y * v.y; }
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

void Animal::randomise(const Tilemap &world)
{
    pos = world.randomFloorPosition();
    home = pos;

    // varying size and speed per creature
    radius = (float)GetRandomValue(6, 18);
    speed = clampf(140 - (radius * 4.0f), 40.0f, 120.0f); // bigger creatures are slower
    roam = (float)GetRandomValue(120, 240);

    // color palette
    Color cols[] = {
        Color{220, 180, 60, 255}, Color{120, 200, 160, 255},
        Color{200, 120, 160, 255}, Color{180, 200, 80, 255}};
    color = cols[GetRandomValue(0, 3)];

    // pick first target near home
    float a = GetRandomValue(0, 628) / 100.0f;
    float r = (float)GetRandomValue(30, (int)roam);
    target = {home.x + cosf(a) * r, home.y + sinf(a) * r};
    retargetTimer = (float)GetRandomValue(60, 180) / 60.0f;
}

void Animal::update(float dt, const Tilemap &world)
{
    retargetTimer -= dt;

    Vector2 toTarget = {target.x - pos.x, target.y - pos.y};
    float d2 = len2(toTarget);

    bool needNew = (retargetTimer <= 0.0f) || (d2 < (radius * radius * 1.5f));

    // corrects to roam near home
    float homeD2 = len2(Vector2{pos.x - home.x, pos.y - home.y});
    bool tooFar = (homeD2 > roam * roam);

    if (needNew || tooFar)
    {
        float a = GetRandomValue(0, 628) / 100.0f;
        float r = (float)GetRandomValue(40, (int)roam);

        Vector2 base = tooFar ? home : pos;
        target = {base.x + cosf(a) * r, base.y + sinf(a) * r};
        retargetTimer = (float)GetRandomValue(60, 180) / 60.0f;
    }

    // move toward target with simple seeking
    toTarget = {target.x - pos.x, target.y - pos.y};
    float len = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
    Vector2 dir = len > 0.001f ? Vector2{toTarget.x / len, toTarget.y / len} : Vector2{0, 0};

    Vector2 delta = {dir.x * speed * dt, dir.y * speed * dt};

    Vector2 next = {pos.x + delta.x, pos.y + delta.y};
    int tx = (int)(next.x / Tilemap::TILE_SIZE), ty = (int)(next.y / Tilemap::TILE_SIZE);
    if (!world.isWall(tx, ty))
        pos = next;
    else
    {
        // if hitting a wall, force new target
        retargetTimer = 0.0f;
    }
}

void Animal::draw() const
{
    // eye to tell movement direction
    DrawCircleV(pos, radius, color);

    Vector2 to = {target.x - pos.x, target.y - pos.y};
    float l = sqrtf(to.x * to.x + to.y * to.y);
    if (l > 0.0001f)
    {
        to.x /= l;
        to.y /= l;
        Vector2 eye = {pos.x + to.x * (radius * 0.6f), pos.y + to.y * (radius * 0.6f)};
        DrawCircleV(eye, clampf(radius * 0.2f, 1.5f, 3.0f), BLACK);
    }
}
