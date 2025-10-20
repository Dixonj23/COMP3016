#include <raylib.h>

int main()
{
    int ballX = 400;
    int ballY = 400;
    int ballSpeed = 3;

    Color green = {20, 160, 133, 255};

    InitWindow(800, 800, "First Raylib Game");
    SetTargetFPS(60);

    // Game Loop
    while (WindowShouldClose() == false)
    {
        // 1. Event Handling

        // 2. Updating Positions
        if (IsKeyDown(KEY_RIGHT))
        {
            ballX += ballSpeed;
        }
        else if (IsKeyDown(KEY_LEFT))
        {
            ballX -= ballSpeed;
        }
        else if (IsKeyDown(KEY_UP))
        {
            ballY -= ballSpeed;
        }
        else if (IsKeyDown(KEY_DOWN))
        {
            ballY += ballSpeed;
        }
        // 3. Drawing
        BeginDrawing();
        ClearBackground(green);
        DrawCircle(ballX, ballY, 20, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
