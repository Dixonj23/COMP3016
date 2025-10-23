#include "Player.hpp"
#include "Tilemap.hpp"
#include "Animal.hpp"
#include <vector>

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

        // Update wildlife
        for (auto &a : animals)
            a.update(dt, world);

        // Draw EVERYTHING
        BeginDrawing();
        ClearBackground(Color{12, 30, 28, 255});

        BeginMode2D(cam);
        world.draw();
        for (auto &a : animals)
            a.draw();
        monster.draw();

        EndMode2D();

        // Debug Hud
        DrawText(TextFormat("Animals: %d", (int)animals.size()), 10, 10, 18, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
