#pragma once
#include <raylib.h>
#include <vector>

class Tilemap
{
public:
    static const int TILE_SIZE = 32;
    static const int WIDTH = 100;
    static const int HEIGHT = 100;

    void loadExampleMap();
    void draw() const;
    bool isWall(int tx, int ty) const;

    // collision
    void resolveCollision(Vector2 &pos, float radius, Vector2 delta);

    // Cave generation
    void generateCave(unsigned seed = 1337, int fillPercent = 45, int smoothSteps = 5);
    Vector2 pickSpawnFloorNearCenter() const; // finds a spawnpoint in the cave

    // Random points
    Vector2 randomFloorPosition() const;

private:
    int map[HEIGHT][WIDTH];

    // Variables for cave generation
    int countWallNeighbours(int x, int y) const;
    void floodFillRegions(std::vector<int> &regionIdOut, int &regionCount) const;
    void keepLargestRegionAndFillOthers();
    void connectRegionsToMain();
};