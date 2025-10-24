#pragma once
#include <raylib.h>
#include "Tilemap.hpp"
#include "Animal.hpp"
#include <cmath>

class Player
{
public:
    Player(Vector2 startPos);
    void update(float dt, Tilemap &world, const Camera2D &cam);
    void draw() const;
    Vector2 getPosition() const { return pos; }

    // function to bite, returns number of things consumed
    int tryBite(std::vector<Animal> &animals);

    int getFood() const { return food; }

    float getBiteCooldownFraction() const
    {
        return (biteTimer > 0.0f)
                   ? fminf(biteTimer / biteCooldown, 1.0f)
                   : 0.0f;
    }

private:
    Vector2 pos;
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
};
