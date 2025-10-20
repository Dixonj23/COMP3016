#pragma once
#include <raylib.h>

class Tilemap
{
public:
    static const int TILE_SIZE = 32;
    static const int WIDTH = 100;
    static const int HEIGHT = 100;

    void loadExampleMap();
    void draw() const;
    bool isWall(int tx, int ty) const;
    void resolveCollision(Vector2 &pos, float radius, Vector2 delta);

private:
    int map[HEIGHT][WIDTH];
};