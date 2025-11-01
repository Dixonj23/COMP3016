#include "Player.hpp"
#include "Tilemap.hpp"
#include "Animal.hpp"
#include "Boulder.hpp"
#include "Hunter.hpp"
#include "Combat.hpp"
#include <vector>
#include <algorithm>
#include <raymath.h>

struct ImpactFX
{
    Vector2 pos;
    float time = 0.2f;
    float elapsed = 0.0f;
};

std::vector<ImpactFX> impacts;

std::vector<Boulder> boulders;

std::vector<Hunter> hunters;

std::vector<Bullet> bullets;

SquadIntel squadIntel;

int main()
{
    InitWindow(1200, 800, "Emerge");
    SetTargetFPS(60);

    // lighting
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    RenderTexture2D lightRT = LoadRenderTexture(screenW, screenH);
    SetTextureFilter(lightRT.texture, TEXTURE_FILTER_BILINEAR);

    Tilemap world;
    world.generateCave(43, 45, 5); // seed, fill%, smooth steps

    Player monster(world.pickSpawnFloorNearCenter());

    // spawn a herd of animals
    std::vector<Animal> animals;
    const int NUM_NIMALS = 30;
    animals.resize(NUM_NIMALS);
    for (auto &a : animals)
        a.randomise(world);

    // spawn 4 hunters
    for (int i = 0; i < 4; i++)
    {
        Vector2 hpos = world.randomFloorPosition();
        Hunter h;
        h.spawnAt(world, hpos);
        hunters.push_back(h);
    }

    Camera2D cam{};
    float shakeTime = 0.0f;
    float shakeDuration = 0.0f;
    float shakeMagnitude = 0.0f;
    Vector2 baseOffset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    cam.offset = baseOffset;
    cam.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Update player
        cam.target = monster.getPosition();
        monster.update(dt, world, cam, animals, hunters);

        // Update camera shake
        if (shakeTime > 0.0f)
        {
            shakeTime -= dt;
            float t = (shakeDuration > 0.0f) ? (shakeTime / shakeDuration) : 0.0f;
            float falloff = t * t;

            float phase = GetTime();
            float sx = sinf(phase * 35.0f);
            float sy = cosf(phase * 29.0f);

            float amp = shakeMagnitude * falloff;
            cam.target.x += sx * amp;
            cam.target.y += sy * amp;
        }

        if (monster.tryFireBoulder(boulders, cam))
        {
            //
        }

        if (!monster.isTransforming() && !monster.isDashing())
        {
            monster.tryBite(animals, hunters);
        }

        Vector2 slamPos;
        if (monster.consumeSlamImpact(slamPos))
        {
            impacts.push_back({slamPos, 0.25f, 0.0f});
            shakeDuration = 0.14f;
            shakeTime = shakeDuration;
            shakeMagnitude = 8.0f;
        }

        // decay the shared intel
        if (squadIntel.timeToLive > 0.0f)
        {
            squadIntel.timeToLive -= dt;
            if (squadIntel.timeToLive < 0.0f)
                squadIntel.timeToLive = 0.0f;
        }

        // update hunters
        for (int i = 0; i < (int)hunters.size(); ++i)
        {
            auto &h = hunters[i];
            if (!h.isAlive())
                continue;
            h.update(dt, world, monster, squadIntel);
            h.tryShoot(dt, world, monster, hunters, i, bullets);
        }

        // update bullets
        for (auto &b : bullets)
            b.update(dt, world);

        // hit checks
        for (auto &b : bullets)
        {
            if (!b.alive)
                continue;

            if (b.team == Team::Hunter)
            {
                // hit player?
                if (monster.isAlive())
                {
                    float dx = monster.getPosition().x - b.pos.x;
                    float dy = monster.getPosition().y - b.pos.y;
                    float rr = (monster.getRadius() + b.radius);
                    if (dx * dx + dy * dy <= rr * rr)
                    {
                        if (monster.applyHit(b.damage))
                        {
                            b.alive = false;
                        }
                        else
                        {
                            b.alive = false;
                        }
                        continue;
                    }
                }
            }
            else
            {
                for (auto &h : hunters)
                {
                    if (!h.isAlive())
                        continue;
                    float dx = h.pos.x - b.pos.x, dy = h.pos.y - b.pos.y;
                    float rr = (h.radius + b.radius);
                    if (dx * dx + dy * dy <= rr * rr)
                    {
                        h.takeDamage(b.damage);
                        b.alive = false;
                        break;
                    }
                }
            }
        }

        // Remove dead bullets
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                     [](const Bullet &b)
                                     { return !b.alive; }),
                      bullets.end());

        // Remove dead hunters (maybe draw corpses later?)
        hunters.erase(std::remove_if(hunters.begin(), hunters.end(),
                                     [](const Hunter &h)
                                     { return !h.isAlive(); }),
                      hunters.end());

        // Update wildlife
        for (auto &a : animals)
        {
            a.update(dt, world);
        }

        animals.erase(
            std::remove_if(animals.begin(), animals.end(),
                           [](const Animal &a)
                           { return !a.alive; }),
            animals.end());

        // Update boulders
        for (auto &b : boulders)
        {
            bool exploded = b.update(dt, world, animals);
            if (exploded)
            {
                // kill aoe
                float aoe = 64.0f;
                for (auto &a : animals)
                {
                    if (!a.alive)
                        continue;
                    float dx = a.pos.x - b.pos.x, dy = a.pos.y - b.pos.y;
                    if (dx * dx + dy * dy <= aoe * aoe)
                        a.alive = false;
                }

                for (auto &h : hunters)
                {
                    float dx = h.pos.x - b.pos.x, dy = h.pos.y - b.pos.y;
                    float rr = (b.radius + h.radius);
                    if (dx * dx + dy * dy <= rr * rr)
                    {
                        h.applyHit(monster.boulderAoeDamage(), b.pos, 180.0f);
                    }
                }

                // boulder impact
                impacts.push_back({b.pos, 0.2f, 0.0f});
                shakeDuration = 0.15f;
                shakeTime = shakeDuration;
                shakeMagnitude = 6.0f;
            }
        }
        boulders.erase(std::remove_if(boulders.begin(), boulders.end(),
                                      [](const Boulder &b)
                                      { return !b.alive; }),
                       boulders.end());

        // update impacts
        for (auto &fx : impacts)
        {
            fx.elapsed += dt;
        }

        // light mask
        BeginTextureMode(lightRT);
        ClearBackground(BLACK);

        BeginMode2D(cam);
        // player light circle
        Vector2 p = monster.getPosition();
        float outerR = monster.getVisionRadius();
        float innerR = monster.getInnerLightRadius();
        DrawCircleV(p, innerR, WHITE);
        DrawCircleGradient((int)p.x, (int)p.y, outerR, WHITE, BLACK);
        for (auto &fx : impacts)
        {
            float t = fx.elapsed / fx.time;
            float a = 1.0f - t;
            float r = 60.0f + 120.0f * t;
            DrawCircleV(fx.pos, r, Fade(WHITE, a));
        }

        // hunter lights
        BeginMode2D(cam);
        for (auto &h : hunters)
        {
            h.drawFOV();
            // small circle light for clarity
            float auraOuter = 70.0f;
            float auraInner = 45.0f;

            DrawCircleV(h.pos, auraInner, WHITE);
            DrawCircleGradient((int)h.pos.x, (int)h.pos.y, auraOuter, WHITE, BLACK);

            // cone light for hunter vision
            float beamRange = 360.0f;
            float halfFov = h.fovDeg * 0.5f;

            float startDeg = h.facingRad * RAD2DEG - halfFov;
            float endDeg = h.facingRad * RAD2DEG + halfFov;

            const int LAYERS = 4;
            for (int i = 0; i <= LAYERS; ++i)
            {
                float t = (float)i / (float)(LAYERS + 1);
                float r = beamRange * (1.0f + 0.12f * t);
                Color c = Fade(WHITE, 0.35f * (1.0f - t));
                DrawCircleSector(h.pos, r, startDeg, endDeg, 40, c);
            }
        }

        EndMode2D();
        EndMode2D();
        EndTextureMode();

        // Draw world
        BeginDrawing();
        ClearBackground(Color{12, 30, 28, 255});

        BeginMode2D(cam);
        world.draw();
        for (auto &a : animals)
            a.draw();
        for (auto &b : boulders)
            b.draw();
        for (auto &h : hunters)
            h.draw();
        for (auto &bul : bullets)
            bul.draw();
        monster.draw();
        EndMode2D();

        // apply light mask after drawing world
        BeginBlendMode(BLEND_MULTIPLIED);
        Rectangle src = {0, 0, (float)lightRT.texture.width, -(float)lightRT.texture.height};
        Rectangle dst = {0, 0, (float)screenW, (float)screenH};
        DrawTexturePro(lightRT.texture, src, dst, {0, 0}, 0.0f, WHITE);
        EndBlendMode();

        // HUD
        int hudX = 20;
        int hudY = 20;

        // Player HP bar
        int hpX = 20, hpY = GetScreenHeight() - 40;
        int bw = 240, bh = 16;
        DrawRectangleLines(hpX - 2, hpY - 2, bw + 4, bh + 4, WHITE);
        float hpFrac = monster.getHP() / monster.getMaxHP();
        DrawRectangle(hpX, hpY, (int)(bw * fmaxf(0.0f, hpFrac)), bh, (hpFrac > 0.3f) ? GREEN : RED);
        DrawText(TextFormat("HP: %d / %d", (int)monster.getHP(), (int)monster.getMaxHP()),
                 hpX, hpY - 22, 18, WHITE);

        // stage/food
        DrawText(TextFormat("Stage: %d", monster.getStage()), hudX, hudY, 22, WHITE);
        DrawText(TextFormat("Food: %d", monster.getFood()), hudX, hudY + 24, 20, WHITE);

        // bite cooldown bar
        float biteFrac = monster.getBiteCooldownFraction();
        DrawRectangleLines(hudX - 2, hudY + 50 - 2, 104, 24, WHITE); // bar outline
        Color biteCol = (biteFrac > 0.0f) ? RED : GREEN;
        int filledWidth = (int)(100 * (1.0f - biteFrac));
        DrawRectangle(hudX, hudY + 50, filledWidth, 20, biteCol);
        DrawText("BITE", hudX + 110, hudY + 50, 20, WHITE);

        // dash cooldown bar
        if (monster.getStage() >= 2)
        {
            float dashFrac = monster.getDashCooldownFraction();
            int dy = hudY + 50 + 30;
            DrawRectangleLines(hudX - 2, dy - 2, 154, 24, WHITE);
            Color dashCol = (dashFrac > 0.0f) ? SKYBLUE : BLUE;
            int dashFill = (int(150 * (1.0f - dashFrac)));
            DrawRectangle(hudX, hudY + 50 + 30 - 2, dashFill, 20, dashCol);
            DrawText("DASH", hudX + 160, dy + 2, 20, WHITE);
        }

        // dash hint prompt
        if (monster.showDashHint)
        {
            float alpha = fminf(monster.dashHintTimer / 4.0f, 1.0f);
            Color c = Fade(SKYBLUE, alpha);

            const char *msg = "New Ability Unlocked! Press [SHIFT] to Dash";
            int tw = MeasureText(msg, 28);
            DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() - 100, 28, c);
        }

        // boulder cooldown bar
        if (monster.getStage() >= 3)
        {
            float bFrac = monster.getBoulderCooldownFraction();
            int dy = hudY + 110;
            DrawRectangleLines(hudX - 2, dy - 2, 154, 24, WHITE);
            DrawRectangle(hudX, dy, (int)(150 * (1.0f - bFrac)), 20, (bFrac > 0.0f) ? BROWN : Color{150, 90, 40, 255});
            DrawText("BOULDER", hudX + 160, dy + 2, 20, WHITE);
        }

        // slam cooldown bar
        if (monster.getStage() >= 4)
        {
            float sFrac = monster.getSlamCooldownFraction();
            int sy = hudY + 140;
            DrawRectangleLines(hudX - 2, sy - 2, 154, 24, WHITE);
            Color sCol = (sFrac > 0.0f) ? RED : MAROON;
            DrawRectangle(hudX, sy, (int)(150 * (1.0f - sFrac)), 20, sCol);
            DrawText("SLAM", hudX + 160, sy + 2, 20, WHITE);
        }

        // Evolution prompt
        if (!monster.isTransforming() && monster.isEvolveReady())
        {
            const char *msg = "Press E to EVOLVE";
            int tw = MeasureText(msg, 28);
            DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 60, 28, GOLD);
        }

        // transform progress bar
        if (monster.isTransforming())
        {
            float p = monster.transformProgress();
            int bw = 300, bh = 16;
            int bx = GetScreenWidth() / 2 - bw / 2;
            int by = GetScreenHeight() - 80;
            DrawRectangleLines(bx - 2, by - 2, bw + 4, bh + 4, WHITE);
            DrawRectangle(bx, by, (int)(bw * p), bh, YELLOW);
            int tw = MeasureText("Transforming... Vulnerable!", 20);
            DrawText("Transforming... VULNERABLE!", GetScreenWidth() / 2 - tw / 2, by - 26, 20, ORANGE);
        }

        EndDrawing();
    }
    UnloadRenderTexture(lightRT);
    CloseWindow();
    return 0;
}
