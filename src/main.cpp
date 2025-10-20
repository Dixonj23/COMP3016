#include "Player.hpp"
#include "Tilemap.hpp"

int main()
{
    InitWindow(800, 800, "Emerge");
    SetTargetFPS(60);

    Tilemap world;
    world.loadExampleMap();

    Player monster({400, 400});

    Camera2D cam{};
    cam.target = monster.getPosition();
    cam.offset = {400, 400};
    cam.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Update
        monster.update(dt, world);
        cam.target = monster.getPosition();

        // Draw
        BeginDrawing();
        ClearBackground(DARKGREEN);

        BeginMode2D(cam);
        world.draw();
        monster.draw();

                EndMode2D();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
