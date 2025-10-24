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
        monster.update(dt, world, cam);

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

        float cdFrac = monster.getBiteCooldownFraction();

        // cd bar outline
        DrawRectangleLines(hudX - 2, hudY - 2, 104, 24, WHITE);

        // fill cd bar
        Color fill = (cdFrac > 0.0f) ? RED : GREEN;
        int filledWidth = (int)(100 * (1.0f - cdFrac));
        DrawRectangle(hudX, hudY, filledWidth, 20, fill);

        DrawText("BITE", hudX + 110, hudY + 2, 20, WHITE);
        DrawText(TextFormat("Food: %d", monster.getFood()), hudX, hudY + 32, 20, WHITE);
        DrawText("Left Click: Bite", hudX, hudY + 56, 20, LIGHTGRAY);
        // Debug Hud
        DrawText(TextFormat("Animals: %d", (int)animals.size()), hudX, hudY + 80, 20, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
