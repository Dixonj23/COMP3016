#include "Hunter.hpp"
#include <cmath>

static float len(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
static Vector2 norm(Vector2 v)
{
    float L = len(v);
    return (L > 1e-4) ? Vector2{v.x / L, v.y / L} : Vector2{0, 0};
}
static float dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }
static float wrapPi(float a)
{
    while (a <= -PI)
        a += 2 * PI;
    while (a > PI)
        a -= 2 * PI;
    return a;
}
static float rotateTowards(float current, float target, float maxStep)
{
    float d = wrapPi(target - current);
    if (d > maxStep)
        d = maxStep;
    if (d < -maxStep)
        d = -maxStep;
    return wrapPi(current + d);
}

void Hunter::spawnAt(const Tilemap &world, Vector2 p)
{
    pos = p;
    patrolHome = p;
    pickNewPatrolTarget(world);
}

void Hunter::pickNewPatrolTarget(const Tilemap &world)
{
    // pick a random point near home
    float a = GetRandomValue(0, 628) / 100.0f;
    float r = (float)GetRandomValue(80, (int)patrolRadius);
    Vector2 goal = {patrolHome.x + cosf(a) * r, patrolHome.y + sinf(a) * r};
    requestPathTo(world, goal);
    retargetTimer = GetRandomValue(120, 240) / 60.0f; // 2-4 seconds
}

void Hunter::requestPathTo(const Tilemap &world, Vector2 goal)
{
    world.findPath(pos, goal, path);
    pathIndex = 0;
}

void Hunter::followPath(const Tilemap &world, float dt)
{
    if (pathIndex >= (int)path.size())
    {
        // arrived
        return;
    }
    Vector2 target = path[pathIndex];
    Vector2 to = {target.x - pos.x, target.y - pos.y};
    float d = len(to);
    if (d < 6.0f)
    {
        pathIndex++;
        return;
    }
    Vector2 dir = {to.x / d, to.y / d};

    // smooth rotation towards path
    float targetAng = atan2f(dir.y, dir.x);
    float maxStep = turnRate * dt;
    facingRad = rotateTowards(facingRad, targetAng, maxStep);

    Vector2 delta = {dir.x * speed * dt, dir.y * speed * dt};
    world.resolveCollision(pos, radius, delta);
}

void Hunter::update(float dt, const Tilemap &world, const Player &player)
{
    // sensing
    Vector2 pp = player.getPosition();
    Vector2 toP = {pp.x - pos.x, pp.y - pos.y};
    float dist = len(toP);

    bool seePlayer = false;
    if (dist <= sightRange)
    {
        Vector2 fwd = {cosf(facingRad), sinf(facingRad)};
        Vector2 dirToP = (dist > 1e-4f) ? Vector2{toP.x / dist, toP.y / dist} : Vector2{0, 0};
        float cosHalf = cosf((fovDeg * 0.5f) * (PI / 180.0f));
        float facing = dot(fwd, dirToP);
        if (facing >= cosHalf)
        {
            if (world.hasLineOfSight(pos, pp))
                seePlayer = true;
        }
    }

    if (seePlayer)
    {
        lastSeen = pp;
        memory = loseSightTime;
        if (state != State::Chase)
        {
            state = State::Chase;
            requestPathTo(world, lastSeen);
        }
    }
    else
    {
        if (memory > 0.0f)
        {
            memory -= dt;
        }
        else
        {
            if (state == State::Chase)
            {
                state = State::Search;
                requestPathTo(world, lastSeen);
            }
        }
    }

    // repath occsionjally during chase/search
    repathTimer -= dt;
    if (repathTimer <= 0.0f)
    {
        repathTimer = repathInterval;
        if (state == State::Chase && seePlayer)
        {
            requestPathTo(world, pp);
        }
        else if (state == State::Search)
        {
            // keep heading to last seen
            requestPathTo(world, lastSeen);
        }
    }

    // patrol state retarget
    if (state == State::Patrol)
    {
        retargetTimer -= dt;
        if (retargetTimer <= 0.0f)
            pickNewPatrolTarget(world);
    }

    // state movement
    switch (state)
    {
    case State::Patrol:
        followPath(world, dt);
        break;
    case State::Chase:
        followPath(world, dt);
        break;
    case State::Search:
        followPath(world, dt);
        // if reached lastSeen, fall back to patrolling after a short wait
        if (pathIndex >= (int)path.size())
        {
            retargetTimer = 0.0f;
            state = State::Patrol;
        }
        break;
    }
}

void Hunter::draw() const
{
    Color body = (state == State::Chase) ? RED : (state == State::Search ? ORANGE : BLUE);
    DrawCircleV(pos, radius, body);
    // tiny eye (like animals), points towards current path target
    if (pathIndex < (int)path.size())
    {
        Vector2 to = {path[pathIndex].x - pos.x, path[pathIndex].y - pos.y};
        Vector2 d = norm(to);
        Vector2 eye = {pos.x + d.x * (radius * 0.6f), pos.y + d.y * (radius * 0.6f)};
        DrawCircleV(eye, 3.0f, BLACK);
    }
}

void Hunter::drawFOV() const
{
    // draw a translucent sector centered at pos, oriented by facingRad
    float half = (fovDeg * 0.5f);
    float startDeg = (facingRad * RAD2DEG) - half;
    float endDeg = (facingRad * RAD2DEG) + half;

    // pick color by state so player can “read” hunter
    Color c = (state == State::Chase)    ? Color{255, 60, 60, 90}
              : (state == State::Search) ? Color{255, 160, 60, 70}
                                         : Color{60, 160, 255, 60};

    // Soft fill + outline so it’s visible
    DrawCircleSector(pos, sightRange, startDeg, endDeg, 36, c);
}