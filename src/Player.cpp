#include "Player.hpp"
#include <cmath>

Player::Player(Vector2 startPos) : pos(startPos) {}

void Player::update(float dt, Tilemap &world, const Camera2D &cam)
{
    // cooldowns
    if (biteTimer > 0.0f)
        biteTimer -= dt;
    if (biteFxTimer > 0.0f)
        biteFxTimer -= dt;

    // Getting mouse position in world space
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
    Vector2 forward = {mouseWorld.x - pos.x, mouseWorld.y - pos.y};
    float len = sqrt(forward.x * forward.x + forward.y * forward.y);
    if (len > 0.0001f)
    {
        forward.x /= len;
        forward.y /= len;
    }
    angle = atan2f(forward.y, forward.x); // making facing independent of collision

    // Movement
    Vector2 right = {-forward.y, forward.x};
    float f = (IsKeyDown(KEY_W) ? 1.0f : 0.0f) - (IsKeyDown(KEY_S) ? 1.0f : 0.0f);
    float r = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);

    Vector2 move = {forward.x * f + right.x * r, forward.y * f + right.y * r};
    float moveLen = sqrtf(move.x * move.x + move.y * move.y);
    if (moveLen > 0.0001f)
    {
        move.x /= moveLen;
        move.y /= moveLen;
    }

    Vector2 delta = {move.x * speed * dt, move.y * speed * dt};

    // collision
    world.resolveCollision(pos, 14.0f, delta);
}

void Player::draw() const
{
    // body
    DrawCircleV(pos, 14.0f, WHITE);
    // nose
    Vector2 nose = {pos.x + cosf(angle) * 20, pos.y + sinf(angle) * 20};
    DrawLineEx(pos, nose, 3.0f, BLACK);

    // bite FX
    if (biteFxTimer > 0.0f)
    {
        int segments = 16;
        float half = (biteArcDeg * 0.5f) * (PI / 180.0f);
        float a0 = angle - half;
        float a1 = angle + half;
        for (int i = 0; i < segments; ++i)
        {
            float t0 = (float)i / segments;
            float t1 = (float)(i + 1) / segments;
            float aa = a0 + (a1 - a0) * t0;
            float bb = a0 + (a1 - a0) * t1;
            Vector2 p0 = {pos.x + cosf(aa) * biteRange, pos.y + sinf(aa) * biteRange};
            Vector2 p1 = {pos.x + cosf(bb) * biteRange, pos.y + sinf(bb) * biteRange};
            DrawLineEx(p0, p1, 2.0f, RED);
        }
    }
}

static inline float dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }
static inline float len(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

int Player::tryBite(std::vector<Animal> &animals)
{
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return 0;

    if (biteTimer > 0.0f)
        return 0;

    biteTimer = biteCooldown;
    biteFxTimer = 0.12f;

    // bite/eat check
    Vector2 fwd = {cosf(angle), sinf(angle)};
    float cosHalfArc = cosf((biteArcDeg * 0.5f) * (PI / 180.0f));

    int eaten = 0;
    for (auto &a : animals)
    {
        if (!a.alive)
            continue;
        Vector2 to = {a.pos.x - pos.x, a.pos.y - pos.y};
        float d = len(to);
        if (d > (biteRange + a.radius))
            continue;

        // angle check
        Vector2 n = d > 0.0001f ? Vector2{to.x / d, to.y / d} : Vector2{0, 0};
        float facing = dot(fwd, n);
        if (facing >= cosHalfArc)
        {
            a.alive = false;
            eaten++;
        }
    }

    if (eaten > 0)
    {
        food += eaten;
        biteTimer = biteCooldown;
    }
    else
    {
        // small cooldown to prevent spamming
        biteTimer = biteCooldown;
    }
    return eaten;
}