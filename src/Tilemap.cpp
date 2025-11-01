#include "Tilemap.hpp"
#include <random>
#include <queue>
#include <algorithm>
#include <cmath>
#include <raymath.h>

void Tilemap::loadExampleMap()
{
    generateCave(1337, 45, 5);

    /* old world gen code
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            bool border = (x == 0 || y == 0 || x == WIDTH - 1 || y == HEIGHT - 1);
            bool wall = border;

            // Add some random internal walls
            if (!border && GetRandomValue(0, 100) < 10)
            { // 10% chance
                wall = true;
            }

            map[y][x] = wall ? 1 : 0;
        }
    }
    */
}

bool Tilemap::isWall(int tx, int ty) const
{
    if (tx < 0 || ty < 0 || tx >= WIDTH || ty >= HEIGHT)
        return true;
    return map[ty][tx] == 1;
}

void Tilemap::draw() const
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            Color c = map[y][x] ? Color{30, 45, 55, 255} : Color{18, 120, 100, 255};
            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);

            /* old world gen code
            Rectangle r{(float)x * TILE_SIZE, (float)y * TILE_SIZE,
                        (float)TILE_SIZE, (float)TILE_SIZE};
            if (map[y][x])
            {
                DrawRectangleRec(r, Color{30, 45, 55, 255});
            }
            else
            {
                DrawRectangleRec(r, Color{18, 120, 100, 255});
            }
            */
        }
    }
}

// basic collision function
void Tilemap::resolveCollision(Vector2 &pos, float radius, Vector2 delta) const
{
    Vector2 next = {pos.x + delta.x, pos.y + delta.y};
    int tx = (int)(next.x / TILE_SIZE), ty = (int)(next.y / TILE_SIZE);
    if (!isWall(tx, ty))
        pos = next;
}

// Cave generation
int Tilemap::countWallNeighbours(int x, int y) const
{
    int cnt = 0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue;
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || ny < 0 || nx >= WIDTH || ny >= HEIGHT)
            {
                cnt++;
                continue;
            }
            if (map[ny][nx] == 1)
                cnt++;
        }
    }
    return cnt;
}

void Tilemap::generateCave(unsigned seed, int fillPercent, int smoothSteps)
{
    fillPercent = std::clamp(fillPercent, 1, 99);
    smoothSteps = std::clamp(smoothSteps, 1, 8);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d100(0, 99);

    // randomly fill map
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            bool border = (x == 0 || y == 0 || x == WIDTH - 1 || y == HEIGHT - 1);
            map[y][x] = border ? 1 : (d100(rng) < fillPercent ? 1 : 0);
        }
    }

    /*Smooth using following rule:
        If 5 or more neighbouring cells are walls then cell is wall, else keep the same
    */
    for (int step = 0; step < smoothSteps; ++step)
    {
        int next[HEIGHT][WIDTH];
        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                if (x == 0 || y == 0 || x == WIDTH - 1 || y == HEIGHT - 1)
                {
                    next[y][x] = 1;
                    continue;
                }
                int n = countWallNeighbours(x, y);
                if (n > 4)
                    next[y][x] = 1;
                else if (n < 4)
                    next[y][x] = 0;
                else
                    next[y][x] = map[y][x];
            }
        }
        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                map[y][x] = next[y][x];
            }
        }
    }

    // Keep larget floor region , fill empty spaces
    keepLargestRegionAndFillOthers();

    // Connect remains empty spaces (corridor generation)
    connectRegionsToMain();
}

void Tilemap::floodFillRegions(std::vector<int> &regionIdOut, int &regionCount) const
{
    regionIdOut.assign(WIDTH * HEIGHT, -1);
    regionCount = 0;
    auto idx = [&](int x, int y)
    { return y * WIDTH + x; };

    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            if (map[y][x] == 1)
                continue;
            if (regionIdOut[idx(x, y) != -1])
                continue;

            int id = regionCount++;
            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            regionIdOut[idx(x, y)] = id;

            while (!q.empty())
            {
                auto [cx, cy] = q.front();
                q.pop();
                static const int DIRS[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (auto &d : DIRS)
                {
                    int nx = cx + d[0], ny = cy + d[1];
                    if (nx <= 0 || ny <= 0 || nx >= WIDTH - 1 || ny >= HEIGHT - 1)
                        continue;
                    if (map[ny][nx] == 1)
                        continue;
                    int &tag = regionIdOut[idx(nx, ny)];
                    if (tag == -1)
                    {
                        tag = id;
                        q.push({nx, ny});
                    }
                }
            }
        }
    }
}

void Tilemap::keepLargestRegionAndFillOthers()
{
    std::vector<int> regionId;
    int regionCount = 0;
    floodFillRegions(regionId, regionCount);
    if (regionCount <= 1)
        return;

    // count sizes
    std::vector<int> size(regionCount, 0);
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            if (map[y][x] == 0)
                size[regionId[y * WIDTH + x]]++;
        }
    }
    int mainId = (int)(std::max_element(size.begin(), size.end()) - size.begin());

    // fill other regions
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            int id = regionId[y * WIDTH + x];
            if (map[y][x] == 0 && id != mainId)
                map[y][x] = 1;
        }
    }
}

void Tilemap::connectRegionsToMain()
{
    /*
    After keepLargestRegionAndFillOthers(), any floor left is already main, but the smoothing might still leave narrow breaks.
    This code will carve winding connections to empty pockets
    */

    // Find approximate biggest open area for starting position
    Vector2 center = {WIDTH * 0.5f, HEIGHT * 0.5f};
    int cx = (int)center.x, cy = (int)center.y;
    // Adjust to nearest floor
    for (int r = 0; r < 20 && map[cy][cx] == 1; ++r)
    {
        if (cx + 1 < WIDTH && map[cy][cx + 1] == 0)
        {
            cx++;
            break;
        }
        if (cx - 1 >= 0 && map[cy][cx - 1] == 0)
        {
            cx--;
            break;
        }
        if (cy + 1 < HEIGHT && map[cy + 1][cx] == 0)
        {
            cy++;
            break;
        }
        if (cy - 1 >= 0 && map[cy - 1][cx] == 0)
        {
            cy--;
            break;
        }
    }

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> turn(0, 100);

    // random walks to create winding passages
    for (int w = 0; w < 12; ++w)
    {
        int x = cx, y = cy;
        int dx = (w % 2 == 0) ? 1 : -1;
        int dy = (w % 3 == 0) ? 1 : -1;
        int steps = 120;

        (void)dx;
        (void)dy;

        while (steps--)
        {
            if (x <= 1 || y <= 1 || x >= WIDTH - 2 || y >= HEIGHT - 2)
                break;

            map[y][x] = 0; // carve passages

            // widen passages and add irregularity
            if (turn(rng) < 40 && x + 1 < WIDTH - 1)
                map[y][x + 1] = 0;
            if (turn(rng) < 40 && x - 1 > 0)
                map[y][x - 1] = 0;

            // add random turns
            if (turn(rng) < 25)
            {
                dx = (turn(rng) < 50) ? 1 : -1;
                dy = 0;
            }
            if (turn(rng) < 25)
            {
                dy = (turn(rng) < 50) ? 1 : -1;
                dx = 0;
            }
        }
    }
}

// spawnpoint allocation
Vector2 Tilemap::pickSpawnFloorNearCenter() const
{
    int cx = WIDTH / 2, cy = HEIGHT / 2;
    const int R = std::max(WIDTH, HEIGHT);
    for (int r = 0; r < R; ++r)
    {
        for (int dy = -r; dy <= r; ++dy)
        {
            for (int dx = -r; dx <= r; ++dx)
            {
                int x = cx + dx, y = cy + dy;
                if (x <= 1 || y <= 1 || x >= WIDTH - 1 || y >= HEIGHT - 1)
                    continue;
                if (!isWall(x, y))
                {
                    return Vector2{(x + 0.5f) * TILE_SIZE, (y + 0.5f) * TILE_SIZE};
                }
            }
        }
    }
    return Vector2{(float)cx * TILE_SIZE, (float)cy * TILE_SIZE};
}

Vector2 Tilemap::randomFloorPosition() const
{
    for (int tries = 0; tries < 1024; ++tries)
    {
        int x = GetRandomValue(1, WIDTH - 2);
        int y = GetRandomValue(1, HEIGHT - 2);
        if (!isWall(x, y))
        {
            return Vector2{(x + 0.5f) * TILE_SIZE, (y + 0.5f) * TILE_SIZE};
        }
    }

    // Fallbak to center
    return Vector2{(WIDTH * 0.5f) * TILE_SIZE, (HEIGHT * 0.5f) * TILE_SIZE};
}

// wall destruction
bool Tilemap::carveCircle(Vector2 centerWorld, float radiusPx, bool preserveBorder, Vector2 *outBorderBreakPos)
{
    int minTx = (int)floorf((centerWorld.x - radiusPx) / TILE_SIZE);
    int minTy = (int)floorf((centerWorld.y - radiusPx) / TILE_SIZE);
    int maxTx = (int)floorf((centerWorld.x + radiusPx) / TILE_SIZE);
    int maxTy = (int)floorf((centerWorld.y + radiusPx) / TILE_SIZE);

    minTx = Clamp(minTx, 0, WIDTH - 1);
    minTy = Clamp(minTy, 0, HEIGHT - 1);
    maxTx = Clamp(maxTx, 0, WIDTH - 1);
    maxTy = Clamp(maxTy, 0, HEIGHT - 1);

    bool brokeBorder = false;
    Vector2 breakPos{};

    for (int ty = minTy; ty <= maxTy; ++ty)
    {
        for (int tx = minTx; tx <= maxTx; ++tx)
        {
            if (preserveBorder && isBorder(tx, ty))
                continue;

            Rectangle t = {
                tx * (float)TILE_SIZE,
                ty * (float)TILE_SIZE,
                (float)TILE_SIZE,
                (float)TILE_SIZE};

            if (CheckCollisionCircleRec(centerWorld, radiusPx, t))
            {
                if (map[ty][tx] == 1)
                {
                    map[ty][tx] = 0; // remove wall by making it a floor
                    if (isBorder(tx, ty))
                    {
                        brokeBorder = true;
                        breakPos = {t.x + t.width * 0.5f, t.y + t.height * 0.5f};
                    }
                }
            }
        }

        if (brokeBorder)
        {
            breachFlag = true;
            lastBreachPos = breakPos;
            if (outBorderBreakPos)
                *outBorderBreakPos = breakPos;
        }
    }
    return brokeBorder;
};

// line of sight
bool Tilemap::hasLineOfSight(Vector2 a, Vector2 b) const
{
    // fancy line drawing code, named after someone but cant remember who
    int x0, y0, x1, y1;
    worldToTile(a, x0, y0);
    worldToTile(b, x1, y1);
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true)
    {
        if (isWall(x0, y0))
            return false;
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
    return true;
}

// pathfinding
bool Tilemap::findPath(Vector2 startWorld, Vector2 goalWorld, std::vector<Vector2> &outPath) const
{
    // simple grid pathfinding using map tiles

    struct Node
    {
        int x, y;   // tile position
        int g, f;   // cost so far and total cost estimate
        int px, py; // parent coordinates
    };

    // convert world coordinates to tile grid coordinates
    int sx, sy, gx, gy;
    worldToTile(startWorld, sx, sy);
    worldToTile(goalWorld, gx, gy);
    // dont path to a wall (obviously)
    if (isWall(gx, gy))
        return false;

    const int W = WIDTH, H = HEIGHT;

    // static caches to avoid large arrays each frame
    static int openFlag[HEIGHT][WIDTH];
    static int closedFlag[HEIGHT][WIDTH];
    static Node parent[HEIGHT][WIDTH];
    static int stamp = 1;
    stamp++;

    // gotta love heuristic costs (sarcasm)
    auto Hcost = [&](int x, int y)
    { return 10 * (abs(x - gx) + abs(y - gy)); };

    // priority queue using a vector
    struct Q
    {
        int x, y, f;
    };
    std::vector<Q> heap;

    // push helper to maintain order
    auto push = [&](int x, int y, int f)
    {heap.push_back({x,y,f}); std::push_heap(heap.begin(), heap.end(), [](const Q&a, const Q&b){return a.f>b.f;}); };

    // pop helper to return node with the smallest f (total cost so far)
    auto pop = [&]()
    { std::pop_heap(heap.begin(), heap.end(), [](const Q&a, const Q&b){return a.f>b.f;}); Q q=heap.back(); heap.pop_back(); return q; };

    openFlag[sy][sx] = stamp;
    parent[sy][sx] = {sx, sy, 0, Hcost(sx, sy), -1, -1};
    push(sx, sy, parent[sy][sx].f);

    const int DIR8[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

    // pathfinding loop
    while (!heap.empty())
    {
        Q cur = pop(); // node with lowest f
        int x = cur.x, y = cur.y;
        if (closedFlag[y][x] == stamp)
            continue;
        closedFlag[y][x] = stamp; // mark as visited

        // goal check
        if (x == gx && y == gy)
        {
            // rebuild path by backtracking
            outPath.clear();
            while (!(x == sx && y == sy))
            {
                outPath.push_back(tileToWorldCenter(x, y));
                Node p = parent[y][x];
                x = p.px;
                y = p.py;
            }
            std::reverse(outPath.begin(), outPath.end());
            return true; // success
        }

        // explore neighbours in four directions
        for (int i = 0; i < 8; ++i)
        {
            int nx = x + DIR8[i][0], ny = y + DIR8[i][1];

            // skip out of bounds
            if (nx < 0 || ny < 0 || nx >= W || ny >= H)
                continue;
            // skip walls
            if (isWall(nx, ny))
                continue;

            bool diagonal = (DIR8[i][0] != 0 && DIR8[i][1] != 0);
            if (diagonal)
            {
                int bx = x + DIR8[i][0];
                int by = y;
                int cx = x;
                int cy = y + DIR8[i][1];
                if (isWall(bx, by) || isWall(cx, cy))
                    continue;
            }

            // skip closed tiles
            if (closedFlag[ny][nx] == stamp)
                continue;

            // cost to move to neighbour
            int stepCost = diagonal ? 14 : 10;
            int g = parent[y][x].g + stepCost;

            // update if neighbours not open or cheaper path found
            if (openFlag[ny][nx] != stamp || g < parent[ny][nx].g)
            {
                parent[ny][nx] = {nx, ny, g, g + Hcost(nx, ny), x, y};
                openFlag[ny][nx] = stamp;
                push(nx, ny, parent[ny][nx].f);
            }
        }
    }
    // edge case, no path is found
    return false;
};
