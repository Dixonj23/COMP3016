#include "Hunter.hpp"
#include "Player.hpp"
#include "Tilemap.hpp"
#include <cmath>

static float len(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
static Vector2 norm(Vector2 v)
{
    float L = len(v);
    return (L > 1e-4) ? Vector2{v.x / L, v.y / L} : Vector2{0, 0};
}
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
    // timers
    if (hitFlashTimer > 0.0f)
        hitFlashTimer -= dt;
    if (hitStunTimer > 0.0f)
        hitStunTimer -= dt;
    if (memory > 0.0f)
        memory -= dt;

    // sensing
    Vector2 pp = player.getPosition();
    Vector2 toP = {pp.x - pos.x, pp.y - pos.y};
    float distP = sqrtf(toP.x * toP.x + toP.y * toP.y);
    Vector2 dirToP = (distP > 1e-4f) ? Vector2{toP.x / distP, toP.y / distP} : Vector2{0, 0};

    Vector2 fwd = {cosf(facingRad), sinf(facingRad)};
    float cosHalf = cosf((fovDeg * 0.5f) * (PI / 180.0f));

    bool inCone = (distP <= sightRange) && (fwd.x * dirToP.x + fwd.y * dirToP.y >= cosHalf);
    bool seePlayer = inCone && world.hasLineOfSight(pos, pp);

    // share squad intel
    if (seePlayer)
    {
        lastSeen = pp;
        memory = loseSightTime;
        intel.spot = pp;
        intel.timeToLive = 2.5f;
    }

    // What do we currently "know"?
    bool hasSharedIntel = (intel.timeToLive > 0.0f);
    bool hasPersonalIntel = (!seePlayer && memory > 0.0f);
    bool knowsTarget = seePlayer || hasSharedIntel || hasPersonalIntel;

    Vector2 trackPos{};
    if (seePlayer)
        trackPos = pp;
    else if (hasSharedIntel)
        trackPos = intel.spot;
    else if (hasPersonalIntel)
        trackPos = lastSeen;

    // state based movement
    if (seePlayer)
    {
        if (state != State::Chase)
        {
            state = State::Chase;
            requestPathTo(world, lastSeen);
        }
    }
    else if (hasSharedIntel)
    {
        if (state == State::Patrol)
        {
            state = State::Search;
            requestPathTo(world, intel.spot);
        }
    }
    else if (!hasPersonalIntel)
    {
        if (state != State::Patrol)
        {
            state = State::Patrol;
            retargetTimer = 0.0f;
        }
    }

    // rotate towards player
    auto wrapPi = [](float a)
    { while(a<=-PI)a+=2*PI; while(a>PI)a-=2*PI; return a; };
    auto rotateTowards = [&](float cur, float tgt, float maxStep)
    {
        float d = wrapPi(tgt - cur);
        if (d > maxStep)
            d = maxStep;
        if (d < -maxStep)
            d = -maxStep;
        return wrapPi(cur + d);
    };

    if (knowsTarget)
    {
        Vector2 aim = {trackPos.x - pos.x, trackPos.y - pos.y};
        float L = sqrtf(aim.x * aim.x + aim.y * aim.y);
        if (L > 1e-4f)
        {
            float targetAng = atan2f(aim.y, aim.x);
            facingRad = rotateTowards(facingRad, targetAng, turnRate * dt);
        }
    }
    else if (pathIndex < (int)path.size())
    {
        // face next waypoint instead
        Vector2 wp = {path[pathIndex].x - pos.x, path[pathIndex].y - pos.y};
        float L = sqrtf(wp.x * wp.x + wp.y * wp.y);
        if (L > 1e-4f)
        {
            float targetAng = atan2f(wp.y, wp.x);
            facingRad = rotateTowards(facingRad, targetAng, turnRate * dt);
        }
    }

    // repath every now and then
    repathTimer -= dt;
    if (repathTimer <= 0.0f)
    {
        repathTimer = repathInterval;
        if (seePlayer)
        {
            requestPathTo(world, pp);
        }
        else if (hasSharedIntel)
        {
            requestPathTo(world, intel.spot);
        }
        else if (state == State::Search && hasPersonalIntel)
        {
            requestPathTo(world, lastSeen);
        }
    }

    // find movement target
    Vector2 desiredMove{0, 0};

    if (seePlayer)
    {
        // ranged kiting/strafe using TRUE position
        if (distP < minRange)
        {
            desiredMove = {-dirToP.x, -dirToP.y};
        }
        else if (distP > preferredRange * 1.2f)
        {
            desiredMove = dirToP;
        }
        else
        {
            Vector2 right = {-dirToP.y, dirToP.x};
            float side = (((int)GetTime()) % 4 < 2) ? 1.0f : -1.0f;
            desiredMove = {right.x * side, right.y * side};
            float scale = (strafeSpeed / fmaxf(speed, 1.0f));
            desiredMove.x *= scale;
            desiredMove.y *= scale;
        }
        state = State::Chase;
    }
    else
    {
        // No line of sight: use pathing; do not steer toward unknown true player pos
        if (state == State::Patrol)
        {
            retargetTimer -= dt;
            if (retargetTimer <= 0.0f)
                pickNewPatrolTarget(world);
        }
    }

    // knockback and stun
    desiredMove.x += knockVel.x / fmaxf(speed, 1.0f);
    desiredMove.y += knockVel.y / fmaxf(speed, 1.0f);
    knockVel.x -= knockVel.x * fminf(knockFriction * dt, 1.0f);
    knockVel.y -= knockVel.y * fminf(knockFriction * dt, 1.0f);

    if (hitStunTimer > 0.0f)
    {
        desiredMove.x *= 0.25f;
        desiredMove.y *= 0.25f;
    }

    // normalise movement
    float m = sqrtf(desiredMove.x * desiredMove.x + desiredMove.y * desiredMove.y);
    if (m > 1e-4f)
    {
        desiredMove.x /= m;
        desiredMove.y /= m;
    }

    // actually moving time
    if (seePlayer)
    {
        Vector2 delta = {desiredMove.x * speed * dt, desiredMove.y * speed * dt};
        world.resolveCollision(pos, radius, delta);
    }
    else
    {
        switch (state)
        {
        case State::Patrol:
            followPath(world, dt);
            break;
        case State::Chase:
            followPath(world, dt);
            break; // chasing last fix
        case State::Search:
            followPath(world, dt);
            break;
        }
    }

    // chasing distance
    if (state == State::Search)
    {
        // clear intel after a couple seconds without line of sight
        if (hasSharedIntel)
        {
            float dx = pos.x - intel.spot.x, dy = pos.y - intel.spot.y;
            if (dx * dx + dy * dy <= 24.0f * 24.0f)
            {
                intel.timeToLive = 0.0f;
            }
        }
    }
    if (state == State::Chase && distP > maxChaseRange && !hasSharedIntel && !hasPersonalIntel)
    {
        state = State::Patrol;
        retargetTimer = 0.0f;
    }
};

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

    // hit flash
    if (hitFlashTimer > 0.0f)
    {
        float a = fminf(hitFlashTimer / 0.12f, 1.0f);
        DrawCircleV(pos, radius + 2.0f, Fade(WHITE, 0.6f * a));
    }

    // healthbar
    float f = hp / fmaxf(1.0f, maxHp);
    int w = 36, h = 5;
    int ox = (int)(pos.x - w / 2), oy = (int)(pos.y - radius - 12);
    DrawRectangle(ox - 1, oy - 1, w + 2, h + 2, BLACK);
    DrawRectangle(ox, oy, w, h, Color{30, 30, 30, 220});
    Color hpCol = (f > 0.6f) ? Color{80, 220, 120, 255} : (f > 0.3f) ? Color{240, 200, 80, 255}
                                                                     : Color{230, 80, 60, 255};
    DrawRectangle(ox, oy, (int)(w * f), h, hpCol);
    DrawRectangleLines(ox - 1, oy - 1, w + 2, h + 2, BLACK);
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
    if (hitStunTimer > 0.0f)
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

void Hunter::applyHit(float dmg, Vector2 sourcePos, float impulse)
{
    hp -= dmg;
    if (hp < 0.0f)
        hp = 0.0f;
    // flash and small stagger
    hitFlashTimer = 0.12f;
    hitStunTimer = 0.15f;

    // knockback
    Vector2 away{pos.x - sourcePos.x, pos.y - sourcePos.y};
    float L = sqrtf(away.x * away.x + away.y * away.y);
    if (L > 1e-4)
    {
        away.x /= L;
        away.y /= L;
    }
    knockVel.x += away.x * impulse;
    knockVel.y += away.y * impulse;
}