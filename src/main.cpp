#include "Player.hpp"
#include "Tilemap.hpp"
#include "Animal.hpp"
#include "Boulder.hpp"
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
        monster.update(dt, world, cam, animals);

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
            monster.tryBite(animals);
        }

        Vector2 slamPos;
        if (monster.consumeSlamImpact(slamPos))
        {
            impacts.push_back({slamPos, 0.25f, 0.0f});
            shakeDuration = 0.14f;
            shakeTime = shakeDuration;
            shakeMagnitude = 8.0f;
        }

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
        EndMode2D();
        EndTextureMode();

        // Draw EVERYTHING
        BeginDrawing();
        ClearBackground(Color{12, 30, 28, 255});

        BeginMode2D(cam);
        world.draw();
        for (auto &a : animals)
            a.draw();
        for (auto &b : boulders)
            b.draw();
        monster.draw();

        /* old impact FX (might still use)
        for (auto &fx : impacts)
        {
            float t = fx.elapsed / fx.time;
            float alpha = 1.0f - t;
            float radius = 20.0f + 80.0f * t;
            DrawCircleV(fx.pos, radius, Fade(ORANGE, alpha * 0.5f));
            DrawCircleLines((int)fx.pos.x, (int)fx.pos.y, radius, Fade(RED, alpha));
        }
        */

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
