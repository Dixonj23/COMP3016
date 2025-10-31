#include "Player.hpp"
#include "Hunter.hpp"
#include <cmath>

Player::Player(Vector2 startPos) : pos(startPos)
{
    applyStageVisuals();
}

void Player::update(float dt, Tilemap &world, const Camera2D &cam, std::vector<Animal> &animals)
{
    // cooldown timers
    if (biteTimer > 0.0f)
        biteTimer -= dt;
    if (biteFxTimer > 0.0f)
        biteFxTimer -= dt;
    if (dashCDTimer > 0.0f)
        dashCDTimer -= dt;
    if (showDashHint)
    {
        dashHintTimer -= dt;
        if (dashHintTimer <= 0.0f)
            showDashHint = false;
    }
    if (boulderCDTimer > 0.0f)
        boulderCDTimer -= dt;
    if (slamCDTimer > 0.0f)
        slamCDTimer -= dt;

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

    // Evolution input
    if (!transforming && isEvolveReady() && IsKeyPressed(KEY_E))
    {
        transforming = true;
        transformElapsed = 0.0f;
    }

    // Transforming state
    if (transforming)
    {
        transformElapsed += dt;
        if (transformElapsed >= transformTime)
        {
            // complete transformation
            transforming = false;

            // consume food for this stage
            if (stage < 4)
            {
                food -= evolveThresholds[stage - 1];
                if (food < 0)
                    food = 0;
            }

            stage = (stage < 4) ? stage + 1 : 4;

            applyStageVisuals();

            // when reahing stage 2 reveal new ability
            if (stage == 2)
            {
                showDashHint = true;
                dashHintTimer = 4.0f;
            }
        }

        return;
    }

    // dash input
    bool canDash = (stage >= 2) && (dashCDTimer <= 0.0f) && !dashing;
    if (canDash && IsKeyPressed(KEY_LEFT_SHIFT))
    {
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
        dashDir = {mouseWorld.x - pos.x, mouseWorld.y - pos.y};
        // find length of dashDir vector
        float L = sqrt(dashDir.x * dashDir.x + dashDir.y * dashDir.y);
        // if on player skip normalisation
        if (L < 1e-4f)
        {
            dashDir = {cosf(angle), sinf(angle)};
        }
        else
        {
            dashDir.x /= L;
            dashDir.y /= L;
        }

        // lock facing to dash direction
        angle = atan2f(dashDir.y, dashDir.x);

        dashing = true;
        dashElapsed = 0.0f;
        biteTimer = fmaxf(biteTimer, 0.0f);
    }

    // lock movement during boulder throw
    if (boulderWinding)
    {
        boulderWindElapsed += dt;
        if (boulderWindElapsed >= boulderWindTime)
        {
            boulderWinding = false;
        }
        return;
    }

    // slam input
    if (stage >= 4)
    {
        if (!slamWinding && slamCDTimer <= 0.0f && !dashing && !boulderWinding && !transforming)
        {
            if (IsKeyPressed(KEY_Q))
            {
                slamWinding = true;
                slamWindElapsed = 0.0f;
            }
        }
        if (slamWinding)
        {
            slamWindElapsed += dt;
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
            Vector2 fw = {mouseWorld.x - pos.x, mouseWorld.y - pos.y};
            float L = sqrtf(fw.x * fw.x + fw.y * fw.y);
            if (L > 1e-4f)
            {
                fw.x /= L;
                fw.y /= L;
            }
            angle = atan2f(fw.y, fw.x);

            if (slamWindElapsed >= slamWindTime)
            {
                // slam time
                slamWinding = false;
                slamCDTimer = slamCooldown;
                slamImpactPos = pos;
                slamJustFired = true;

                for (auto &a : animals)
                {
                    if (!a.alive)
                        continue;
                    float dx = a.pos.x - pos.x, dy = a.pos.y - pos.y;
                    float rr = (slamRadius + a.radius + slamKillPad);
                    if (dx * dx + dy * dy <= rr * rr)
                        a.alive = false;
                }

                // break walls (except outer border)
                world.carveCircle(pos, slamRadius, true);
            }
            return;
        }
    }

    // Movement
    Vector2 delta{0, 0};

    // dashing movement
    if (dashing)
    {
        delta = {dashDir.x * dashSpeed * dt, dashDir.y * dashSpeed * dt};
        world.resolveCollision(pos, radius, delta);

        for (auto &a : animals)
        {
            if (!a.alive)
                continue;
            float dx = a.pos.x - pos.x, dy = a.pos.y - pos.y;
            float rr = (radius + a.radius + dashKillPad);
            // is monster overlaps animals during dash
            if (dx * dx + dy * dy <= rr * rr)
            {
                a.alive = false;
                food += 1;
            }
        }

        dashElapsed += dt;
        // starts cooldown when dash ends
        if (dashElapsed >= dashDuration)
        {
            dashing = false;
            dashCDTimer = dashCooldown;
        }

        // keep facing direction locked to dashDir
        angle = atan2f(dashDir.y, dashDir.x);

        // skip normal movement while dashing
        return;
    }

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

    delta = {move.x * speed * dt, move.y * speed * dt};

    // collision
    world.resolveCollision(pos, radius, delta);
}

void Player::draw() const
{
    // body
    DrawCircleV(pos, radius, WHITE);
    // nose
    Vector2 nose = {pos.x + cosf(angle) * (radius + 6.0f), pos.y + sinf(angle) * (radius + 6.0f)};
    DrawLineEx(pos, nose, 3.0f, BLACK);

    // transform FX
    if (transforming)
    {
        float t = transformProgress();

        // puilsing ring
        DrawCircleLines((int)pos.x, (int)pos.y, radius + 2.0f + 10.0f * t, Color{255, 220, 100, 255});
        DrawCircleV(pos, radius + 6.0f + 6.0f * t, Fade(YELLOW, 0.25f + 0.35f * t));
    }

    // dash FX
    if (dashing)
    {
        Vector2 back = {pos.x - dashDir.x * (radius + 18.0f), pos.y - dashDir.y * (radius + 18.0f)};
        DrawCircleV(back, radius * 0.7f, Fade(SKYBLUE, 0.5f));
    }

    // bite FX
    if (biteFxTimer > 0.0f)
    {
        Color arcCol = (biteTimer > 0.0f) ? GRAY : RED;
        int segments = 18;
        float half = (biteArcDeg * 0.5f) * (PI / 180.0f);
        float a0 = angle - half;
        float a1 = angle + half;

        float lineThickness = 6.0f;

        DrawCircleSector(pos, biteRange, a0 * RAD2DEG, a1 * RAD2DEG, 32, Fade(arcCol, 0.15f));

        for (int i = 0; i < segments; ++i)
        {
            float t0 = (float)i / segments;
            float t1 = (float)(i + 1) / segments;
            float aa = a0 + (a1 - a0) * t0;
            float bb = a0 + (a1 - a0) * t1;
            Vector2 p0 = {pos.x + cosf(aa) * biteRange, pos.y + sinf(aa) * biteRange};
            Vector2 p1 = {pos.x + cosf(bb) * biteRange, pos.y + sinf(bb) * biteRange};
            DrawLineEx(p0, p1, lineThickness, RED);
        }
    }

    // boulder throw FX
    if (boulderWinding)
    {
        float t = boulderWindElapsed / boulderWindTime;
        float pulse = 0.5f + 0.5f + sinf(GetTime() * 20.0f);
        Color glow = Fade(BROWN, 0.03f + 0.04f * pulse);
        float r = radius + 6.0f + 6.0f * t;
        DrawCircleV(pos, r, glow);
        DrawCircleLines((int)pos.x, (int)pos.y, r, BROWN);
    }

    // slam charge FX
    if (slamWinding)
    {
        float t = slamWindElapsed / slamWindTime;
        float r = radius + t * (slamRadius - radius) * 0.25f;
        DrawCircleV(pos, r, Fade(RED, 0.25f + 0.25f * sinf(GetTime() * 20.0f)));
        DrawCircleLines((int)pos.x, (int)pos.y, r, RED);
    }
}

static inline float dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }
static inline float len(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

int Player::tryBite(std::vector<Animal> &animals)
{
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return 0;

    if (dashing || transforming || boulderWinding)
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
            // heal on eating
            hp += 10;
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

bool Player::tryFireBoulder(std::vector<Boulder> &pool, const Camera2D &cam)
{
    if (stage < 3)
        return false;
    if (transforming || dashing)
        return false;

    if (!boulderWinding && boulderCDTimer <= 0.0f && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    {
        // direction
        Vector2 mouseworld = GetScreenToWorld2D(GetMousePosition(), cam);
        boulderDir = {mouseworld.x - pos.x, mouseworld.y - pos.y};
        float L = sqrtf(boulderDir.x * boulderDir.x + boulderDir.y * boulderDir.y);
        if (L < 1e-4f)
            boulderDir = {cosf(angle), sinf(angle)};
        else
        {
            boulderDir.x /= L;
            boulderDir.y /= L;
        }

        boulderWinding = true;
        boulderWindElapsed = 0.0f;
        angle = atan2f(boulderDir.y, boulderDir.x);
        return false; // nothing thrown yet
    };

    if (boulderWinding)
    {
        boulderWindElapsed += GetFrameTime();
        if (boulderWindElapsed > -boulderWindTime)
        {
            // fire boulder
            Boulder b;
            // offset slightly to avoid player
            b.pos = {pos.x + boulderDir.x * (radius + boulderRadius + 4.0f),
                     pos.y + boulderDir.y * (radius + boulderRadius + 4.0f)};
            b.vel = {boulderDir.x * boulderSpeed, boulderDir.y * boulderSpeed};
            b.radius = boulderRadius;
            pool.push_back(b);

            boulderWinding = false;
            boulderCDTimer = boulderCooldown;
            return true; // boulder thrown
        };
    }

    return false;
};

void Player::applyStageVisuals()
{
    int idx = (stage <= 1) ? 0 : (stage == 2 ? 1 : (stage == 3 ? 2 : 3));
    radius = stageRadii[idx];
    bodyColor = stageColours[idx];
    biteRange = 24.0f + (radius - 14) * 1.2f;

    // small hp bump per stage
    float stageHp[4] = {100.0f, 115.0f, 130.0f, 150.0f};
    maxHp = stageHp[idx];
    if (hp > maxHp)
        hp = maxHp;
}