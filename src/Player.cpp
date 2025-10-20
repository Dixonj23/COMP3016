#include "Player.hpp"
#include <cmath>

Player::Player(Vector2 startPos) : pos(startPos) {}

void Player::update(float dt, Tilemap &world)
{
    Vector2 mouse = GetMousePosition();

    Vector2 forward = {mouse.x - pos.x, mouse.y - pos.y};

    float len = sqrt(forward.x * forward.x + forward.y * forward.y);
    if (len > 0.001f)
    {
        forward.x /= len;
        forward.y /= len;
    }
    angle = atan2f(forward.y, forward.x);

    Vector2 right = {-forward.y, forward.x};
    float f = (IsKeyDown(KEY_W) ? 1.0f : 0.0f) - (IsKeyDown(KEY_S) ? 1.0f : 0.0f);
    float r = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);
    Vector2 move = {forward.x * f + right.x * r, forward.y * f + right.y * r};

    float moveLen = sqrtf(move.x * move.x + move.y * move.y);
    if (moveLen > 0.001f)
    {
        move.x /= moveLen;
        move.y /= moveLen;
    }

    Vector2 delta = {move.x * speed * dt, move.y * speed * dt};
    world.resolveCollision(pos, 14.0f, delta);
}

void Player::draw() const
{
    DrawCircleV(pos, 14.0f, WHITE);
    Vector2 nose = {pos.x + cosf(angle) * 20, pos.y + sinf(angle) * 20};
    DrawLineEx(pos, nose, 3.0f, BLACK);
}