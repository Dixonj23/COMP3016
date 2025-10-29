#pragma once
#include <raylib.h>
#include "Tilemap.hpp"
#include "Animal.hpp"
#include <cmath>
#include "Boulder.hpp"

class Player
{
public:
    Player(Vector2 startPos);
    void update(float dt, Tilemap &world, const Camera2D &cam, std::vector<Animal> &animals);
    void draw() const;
    Vector2 getPosition() const { return pos; }

    //  ability tutorial parameters
    bool showDashHint = false;
    float dashHintTimer = 0.0f;

    // stage 1 bite function, returns number of things consumed
    int tryBite(std::vector<Animal> &animals);

    float getBiteCooldownFraction() const
    {
        return (biteTimer > 0.0f)
                   ? fminf(biteTimer / biteCooldown, 1.0f)
                   : 0.0f;
    }

    // evolution functions
    bool isEvolveReady() const
    {
        if (stage >= 4)
            return false;
        int need = evolveThresholds[stage - 1];
        return food >= need;
    }

    bool isTransforming() const { return transforming; }
    float transformProgress() const { return transforming ? (transformElapsed / transformTime) : 0.0f; }
    int getStage() const { return stage; }
    int getFood() const { return food; }

    // Evolution VFX functions
    void applyStageVisuals();
    float getRadius() const { return radius; }
    Color getBodyColor() const { return bodyColor; }

    // stage 2 dash abuility
    bool isDashing() const { return dashing; }
    float getDashCooldownFraction() const
    {
        return (dashCDTimer > 0.0f) ? fminf(dashCDTimer / dashCooldown, 1.0f) : 0.0f;
    }

    // stage 3 boulder function, returnstrue if shot
    bool tryFireBoulder(std::vector<Boulder> &pool, const Camera2D &cam);
    float getBoulderCooldownFraction() const
    {
        return (boulderCDTimer > 0.0f) ? fminf(boulderCDTimer / boulderCooldown, 1.0f) : 0.0f;
    }

    // stage 4 slam function
    bool isSlamming() const { return slamWinding; }
    float getSlamCooldownFraction() const
    {
        return (slamCDTimer > 0.0f) ? fminf(slamCDTimer / slamCooldown, 1.0f) : 0.0f;
    }
    bool consumeSlamImpact(Vector2 &out)
    {
        if (slamJustFired)
        {
            out = slamImpactPos;
            slamJustFired = false;
            return true;
        }
        return false;
    }

private:
    Vector2 pos;
    float radius = 14.0f;
    Color bodyColor = WHITE;
    float angle = 0.0f;
    const float speed = 180.0f;

    // Attack parameters
    float biteRange = 28.0f;
    float biteArcDeg = 90.0f;
    float biteCooldown = 0.5f;
    float biteTimer = 0.0f;

    int food = 0; // number of things eaten

    // bite FX
    mutable float biteFxTimer = 0.0f;

    // Evolution parameters
    int stage = 1;
    int evolveThresholds[3] = {1, 2, 3};

    float stageRadii[4] = {14.0f, 18.0f, 22.0f, 26.0f};
    Color stageColours[4] = {
        Color{230, 245, 255, 255}, // pale
        Color{140, 220, 200, 255}, // teal
        Color{255, 170, 110, 255}, // orange
        Color{220, 120, 120, 255}};

    bool transforming = false;
    float transformTime = 1.5f;
    float transformElapsed = 0.0f;

    // stage 2 dash parameters
    bool dashing = false;
    Vector2 dashDir{1.0f, 0.0f};
    float dashDuration = 0.5f;
    float dashElapsed = 0.0f;
    float dashCooldown = 5.0f;
    float dashCDTimer = 0.0f;
    float dashSpeed = 700.0f;
    float dashKillPad = 4.0f;

    // stage 3 boulder parameters
    float boulderCooldown = 8.0f;
    float boulderCDTimer = 0.0f;
    float boulderSpeed = 420.0f;
    float boulderRadius = 16.0f;

    // wind up
    bool boulderWinding = false;
    float boulderWindTime = 0.4f;
    float boulderWindElapsed = 0.0f;
    Vector2 boulderDir{};

    // stage 4 slam wall break
    bool slamWinding = false;
    float slamWindTime = 0.25f;
    float slamWindElapsed = 0.0f;

    float slamCooldown = 7.0f;
    float slamCDTimer = 0.0f;

    float slamRadius = 96.0f;
    float slamKillPad = 6.0f;

    // slam impact FX parameters
    bool slamJustFired = false;
    Vector2 slamImpactPos{};
};
