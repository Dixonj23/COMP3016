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

void Hunter::update(float dt, const Tilemap &world, const Player &player, SquadIntel &intel)
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
        if (facing >= cosHalf && world.hasLineOfSight(pos, pp))
        {
            seePlayer = true;
        }
    }

    if (seePlayer)
    {
        lastSeen = pp;
        memory = loseSightTime;
        // update squad intel
        intel.spot = pp;
        intel.timeToLive = 2.5f; // squad will converge here for 2.5s

        if (state != State::Chase)
        {
            state = State::Chase;
            requestPathTo(world, lastSeen);
        }
    }
    else
    {
        // move to intel if player notin sight
        if (intel.timeToLive > 0.0f)
        {
            if (state == State::Patrol)
            {
                state = State::Search;
                requestPathTo(world, intel.spot);
            }
        }
        else
        {
            // if no intel fall back to own memory or patrol
            if (memory > 0.0f)
            {
                memory -= dt;
            }
            else if (state == State::Chase || state == State::Search)
            {
                state = State::Patrol;
                retargetTimer = 0.0f;
            }
        }
    }

    // repath occsionjally during chase/search
    repathTimer -= dt;
    if (repathTimer <= 0.0f)
    {
        repathTimer = repathInterval;
        if (seePlayer)
        {
            requestPathTo(world, pp);
        }
        else if (intel.timeToLive > 0.0f)
        {
            requestPathTo(world, intel.spot);
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
            state = State::Patrol;
            retargetTimer = 0.0f;
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

bool Hunter::hasFriendlyInLine(const std::vector<Hunter> &squad, int selfIndex,
                               Vector2 start, Vector2 end, float safety) const
{
    // distance from a point to segment helper
    auto segDist2 = [](Vector2 a, Vector2 b, Vector2 p)
    {
        Vector2 ab{b.x - a.x, b.y - a.y}, ap{p.x - a.x, p.y - a.y};
        float ab2 = ab.x * ab.x + ab.y * ab.y;
        if (ab2 < 1e-4f)
            return (ap.x * ap.x + ap.y * ap.y);
        float t = (ap.x * ab.x + ap.y * ab.y) / ab2;
        t = fminf(1.0f, fmaxf(0.0f, t));
        Vector2 c{a.x + ab.x * t, a.y + ab.y * t};
        float dx = p.x - c.x, dy = p.y - c.y;
        return dx * dx + dy * dy;
    };

    float safety2 = safety * safety;
    for (int i = 0; i < (int)squad.size(); ++i)
    {
        if (i == selfIndex)
            continue;
        const Hunter &h = squad[i];
        if (!h.isAlive())
            continue;
        // ignore friendlies very close behind the muzzle
        if (segDist2(start, end, h.pos) <= safety2)
            return true;
    }
    return false;
}

bool Hunter::tryShoot(float dt, const Tilemap &world, const Player &player,
                      const std::vector<Hunter> &squad, int selfIndex,
                      std::vector<Bullet> &out)
{
    shootTimer -= dt;
    if (shootTimer > 0.0f)
        return false;

    // must see player and be in range
    Vector2 pp = player.getPosition();
    Vector2 toP{pp.x - pos.x, pp.y - pos.y};
    float dist = len(toP);
    if (dist > shootRange)
        return false;

    Vector2 fwd{cosf(facingRad), sinf(facingRad)};
    Vector2 dir = (dist > 1e-4f) ? Vector2{toP.x / dist, toP.y / dist} : Vector2{0, 0};
    float cosHalf = cosf((fovDeg * 0.5f) * (PI / 180.0f));
    if (fwd.x * dir.x + fwd.y * dir.y < cosHalf)
        return false;
    if (!world.hasLineOfSight(pos, pp))
        return false;

    // friendly fire avoidance
    Vector2 end{pos.x + dir.x * (dist + 60.0f), pos.y + dir.y * (dist + 60.0f)};
    if (hasFriendlyInLine(squad, selfIndex, pos, end, 20.0f))
    {
        // hold fire and try again later
        shootTimer = 0.1f;
        return false;
    }

    // fire one bullet
    Bullet b;
    b.team = Team::Hunter;
    b.pos = {pos.x + fwd.x * (radius + 6.0f), pos.y + fwd.y * (radius + 6.0f)};
    b.vel = {dir.x * 700.0f, dir.y * 700.0f};
    b.damage = 12.0f;
    out.push_back(b);

    // handle burst
    if (burstLeft <= 0)
    {
        burstLeft = burstSize - 1;
        shootTimer = shootCooldown;
    }
    else
    {
        burstLeft--;
        shootTimer = (burstLeft == 0) ? burstCooldown : shootCooldown;
    }
    return true;
}
