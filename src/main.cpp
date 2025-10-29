#include "Player.hpp"
#include "Tilemap.hpp"
#include "Animal.hpp"
#include <vector>
#include <algorithm>

int main()
{
    InitWindow(1200, 800, "Emerge");
    SetTargetFPS(60);

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
    cam.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    cam.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Update player
        cam.target = monster.getPosition();
        monster.update(dt, world, cam, animals);

        if (!monster.isTransforming() && !monster.isDashing())
        {
            monster.tryBite(animals);
        }

        int eatenThisClick = monster.tryBite(animals);

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

        // Draw EVERYTHING
        BeginDrawing();
        ClearBackground(Color{12, 30, 28, 255});

        BeginMode2D(cam);
        world.draw();
        for (auto &a : animals)
            a.draw();
        monster.draw();

        EndMode2D();

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

        // dah hint prompt
        if (monster.showDashHint)
        {
            float alpha = fminf(monster.dashHintTimer / 4.0f, 1.0f);
            Color c = Fade(SKYBLUE, alpha);

            const char *msg = "New Ability Unlocked! Press [SHIFT] to Dash";
            int tw = MeasureText(msg, 28);
            DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() - 100, 28, c);
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

    CloseWindow();
    return 0;
}
