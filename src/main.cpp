#include "Player.hpp"
#include "Tilemap.hpp"

int main()
{
    InitWindow(800, 800, "Emerge");
    SetTargetFPS(60);

    Tilemap world;
    world.generateCave(43, 45, 5); // seed, fill%, smooth steps

    Player monster(world.pickSpawnFloorNearCenter());

    Camera2D cam{};
    cam.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    cam.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Update
        cam.target = monster.getPosition();
        monster.update(dt, world, cam);

        // Draw
        BeginDrawing();
        ClearBackground(Color{12, 30, 28, 255});

        BeginMode2D(cam);
        world.draw();
        monster.draw();

        EndMode2D();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
