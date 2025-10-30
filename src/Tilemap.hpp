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
    bool isBorder(int tx, int ty) const;

    // collision
    void resolveCollision(Vector2 &pos, float radius, Vector2 delta);

    // Cave generation
    void generateCave(unsigned seed = 1337, int fillPercent = 45, int smoothSteps = 5);
    Vector2 pickSpawnFloorNearCenter() const; // finds a spawnpoint in the cave

    // Random points
    Vector2 randomFloorPosition() const;

    // carve floor in a circular aread in world space
    void carveCircle(Vector2 centerWorld, float radiusPx, bool preserveBorder = true);

    // tile helper functions
    inline Vector2 tileToWorldCenter(int tx, int ty) const
    {
        return {tx * (float)TILE_SIZE + TILE_SIZE * 0.5f, ty * (float)TILE_SIZE + TILE_SIZE * 0.5f};
    }
    inline void worldToTile(Vector2 p, int &tx, int &ty) const
    {
        tx = (int)(p.x / TILE_SIZE);
        ty = (int)(p.y / TILE_SIZE);
    }

    // line of sight/vision
    bool hasLineOfSight(Vector2 a, Vector2 b) const;

    // pathfinding
    bool findPath(Vector2 startWorld, Vector2 goalWorld, std::vector<Vector2> &outPath) const;

private:
    int map[HEIGHT][WIDTH];

    // Variables for cave generation
    int countWallNeighbours(int x, int y) const;
    void floodFillRegions(std::vector<int> &regionIdOut, int &regionCount) const;
    void keepLargestRegionAndFillOthers();
    void connectRegionsToMain();
};