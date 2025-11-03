#include "Player.hpp"
#include "Tilemap.hpp"
#include "Animal.hpp"
#include "Boulder.hpp"
#include "Hunter.hpp"
#include "Combat.hpp"
#include <vector>
#include <algorithm>
#include <raymath.h>

// impcat visuals
struct ImpactFX
{
    Vector2 pos;
    float time = 0.2f;
    float elapsed = 0.0f;
};

// game phases
enum class GamePhase
{
    MainMenu,
    Pause,
    Grow,
    Hunt,
    Escape,
    Won,
    GameOver
};
GamePhase phase = GamePhase::MainMenu;
GamePhase phaseBeforePause = GamePhase::Hunt; // remember previous when pausing

// containers
std::vector<ImpactFX> impacts;
std::vector<Boulder> boulders;
std::vector<Hunter> hunters;
std::vector<Bullet> bullets;
SquadIntel squadIntel;

// helper functions
static Vector2 NearestBorderPoint(const Tilemap &world, Vector2 p)
{
    float worldW = Tilemap::WIDTH * Tilemap::TILE_SIZE;
    float worldH = Tilemap::HEIGHT * Tilemap::TILE_SIZE;

    float dL = p.x;
    float dR = worldW - p.x;
    float dT = p.y;
    float dB = worldH - p.y;

    float dmin = dL;
    int edge = 0;
    if (dR < dmin)
    {
        dmin = dR;
        edge = 1;
    }
    if (dT < dmin)
    {
        dmin = dT;
        edge = 2;
    }
    if (dB < dmin)
    {
        dmin = dB;
        edge = 3;
    }

    switch (edge)
    {
    case 0:
        return {0.0f, p.y};
    case 1:
        return {worldW, p.y};
    case 2:
        return {p.x, 0.0f};
    default:
        return {p.x, worldH};
    }
}

// Scan the outer ring for any air and return its world center
static bool FindBorderGap(const Tilemap &world, Vector2 &outPos)
{
    const int W = Tilemap::WIDTH;
    const int H = Tilemap::HEIGHT;
    const int TS = Tilemap::TILE_SIZE;

    auto centerOf = [&](int tx, int ty)
    {
        return Vector2{tx * (float)TS + TS * 0.5f, ty * (float)TS + TS * 0.5f};
    };

    // top & bottom rows
    for (int x = 0; x < W; ++x)
    {
        if (!world.isWall(x, 0))
        {
            outPos = centerOf(x, 0);
            return true;
        }
        if (!world.isWall(x, H - 1))
        {
            outPos = centerOf(x, H - 1);
            return true;
        }
    }
    // left & right columns
    for (int y = 0; y < H; ++y)
    {
        if (!world.isWall(0, y))
        {
            outPos = centerOf(0, y);
            return true;
        }
        if (!world.isWall(W - 1, y))
        {
            outPos = centerOf(W - 1, y);
            return true;
        }
    }
    return false;
}

// simple ui button that returns true on click
static bool DrawButton(Rectangle r, const char *label, int fontSize = 28)
{
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);
    bool down = hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool click = hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    Color fill = hover ? Color{60, 90, 110, 220} : Color{40, 60, 70, 200};
    if (down)
        fill = Color{30, 50, 60, 240};

    DrawRectangleRounded(r, 0.2f, 8, fill);
    DrawRectangleRoundedLines(r, 0.2f, 8, Color{220, 230, 240, 220});
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)(r.x + r.width / 2 - tw / 2), (int)(r.y + r.height / 2 - fontSize / 2), fontSize, WHITE);
    return click;
}

int main()
{
    InitWindow(1200, 800, "Emerge");
    SetTargetFPS(60);

    // lighting
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    RenderTexture2D lightRT = LoadRenderTexture(screenW, screenH);
    SetTextureFilter(lightRT.texture, TEXTURE_FILTER_BILINEAR);

    // world and mobs
    Tilemap world;
    // world.generateCave(43, 45, 5); test world

    world.generateCave(GetRandomValue(1, 100), 45, 5);

    Player monster(world.pickSpawnFloorNearCenter());

    // wildlife
    std::vector<Animal> animals;
    const int NUM_ANIMALS = 30;

    // exit objective
    bool exitActive = false;
    Vector2 exitPos{0, 0};
    float bannerTimer = 0.0f;

    // camera
    Camera2D cam{};
    float shakeTime = 0.0f;
    float shakeDuration = 0.0f;
    float shakeMagnitude = 0.0f;
    Vector2 baseOffset = {(float)screenW * 0.5f, (float)screenH * 0.5f};
    cam.offset = baseOffset;
    cam.zoom = 1.0f;

    // way to reset the game
    auto resetGame = [&]()
    {
        // world
        world = Tilemap{};
        // world.generateCave(43, 45, 5);
        world.generateCave(GetRandomValue(1, 100), 45, 5);
        world.setAllowBorderBreak(false);

        // player
        monster.resetForNewRun(world.pickSpawnFloorNearCenter());

        // animals
        animals.clear();
        animals.resize(NUM_ANIMALS);
        for (auto &a : animals)
            a.randomise(world);

        // hunters
        hunters.clear();
        for (int i = 0; i < 4; i++)
        {
            Vector2 hpos = world.randomFloorPosition();
            Hunter h;
            h.spawnAt(world, hpos);
            hunters.push_back(h);
        }

        // squad intel / projectiles / vfx
        squadIntel = {};
        bullets.clear();
        boulders.clear();
        impacts.clear();

        // exit / objective
        exitActive = false;
        exitPos = {0, 0};
        bannerTimer = 3.0f;

        // camera
        cam.target = monster.getPosition();
        cam.offset = baseOffset;
        cam.zoom = 1.0f;

        // shake
        shakeTime = shakeDuration = 0.0f;
        shakeMagnitude = 0.0f;

        // phase
        phase = GamePhase::Grow; // start in Grow
    };

    // main loop
    bool showControlsOverlay = false;
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // main menu
        if (phase == GamePhase::MainMenu)
        {
            BeginDrawing();
            ClearBackground(Color{12, 30, 28, 255});

            const char *title = "E M E R G E";
            int tw = MeasureText(title, 72);
            DrawText(title, screenW / 2 - tw / 2, 140, 72, WHITE);

            // menu background
            DrawCircleLines(screenW / 2, screenH / 2, 200, Fade(WHITE, 0.06f));
            DrawCircleLines(screenW / 2, screenH / 2, 300, Fade(WHITE, 0.03f));

            float bw = 260, bh = 56, gap = 16;
            Rectangle rPlay = {(float)screenW / 2 - bw / 2, 300, bw, bh};
            Rectangle rCtrls = {(float)screenW / 2 - bw / 2, 300 + (bh + gap), bw, bh};
            Rectangle rQuit = {(float)screenW / 2 - bw / 2, 300 + 2 * (bh + gap), bw, bh};

            if (DrawButton(rPlay, "PLAY"))
            {
                resetGame();
            }
            if (DrawButton(rCtrls, "CONTROLS"))
            {
                showControlsOverlay = !showControlsOverlay;
            }
            if (DrawButton(rQuit, "QUIT"))
            {
                EndDrawing();
                break;
            }

            const char *hint = "WASD: Move   LMB: Bite   Shift: Dash   RMB: Boulder (Stage 3)   Q: Slam (Stage 4)";
            int hw = MeasureText(hint, 20);
            DrawText(hint, screenW / 2 - hw / 2, screenH - 60, 20, Fade(WHITE, 0.9f));

            if (showControlsOverlay)
            {
                DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 160});
                const char *c1 = "CONTROLS";
                int cw = MeasureText(c1, 40);
                DrawText(c1, screenW / 2 - cw / 2, 120, 40, GOLD);

                const int fs = 22;
                int y = 180;
                DrawText("Move:   W A S D", 160, y, fs, WHITE);
                y += 28;
                DrawText("Bite:   Left Mouse", 160, y, fs, WHITE);
                y += 28;
                DrawText("Dash:   Left Shift (Stage 2)", 160, y, fs, WHITE);
                y += 28;
                DrawText("Boulder: Right Mouse (Stage 3)", 160, y, fs, WHITE);
                y += 28;
                DrawText("Slam:   Q (Stage 4)", 160, y, fs, WHITE);
                y += 28;
                DrawText("Evolve: E (when prompted)", 160, y, fs, WHITE);
                y += 40;
                DrawText("Goal:   Hunt the Hunters, then BREAK THE BORDER and ESCAPE", 160, y, fs, GOLD);

                Rectangle rClose = {(float)screenW / 2 - 160, (float)screenH - 120, 320, 48};
                if (DrawButton(rClose, "CLOSE", 24))
                    showControlsOverlay = false;
            }

            EndDrawing();
            continue; // skip gameplay when in menu
        }

        // pause
        if (IsKeyPressed(KEY_P))
        {
            if (phase == GamePhase::Pause)
            {
                phase = phaseBeforePause;
            }
            else if (phase != GamePhase::Won)
            {
                phaseBeforePause = phase;
                phase = GamePhase::Pause;
            }
        }

        // game over
        if (phase != GamePhase::MainMenu && phase != GamePhase::Pause &&
            phase != GamePhase::Won && phase != GamePhase::GameOver)
        {
            if (!monster.isAlive())
            {
                phase = GamePhase::GameOver;
                bannerTimer = 0.0f;
            }
        }

        bool updateSim = (phase != GamePhase::Pause && phase != GamePhase::MainMenu && phase != GamePhase::Won && phase != GamePhase::GameOver);

        // update when not paused
        if (updateSim)
        {
            // camera target follows player
            cam.target = monster.getPosition();

            // player update
            monster.update(dt, world, cam, animals, hunters);

            // camera shake
            if (shakeTime > 0.0f)
            {
                shakeTime -= dt;
                float t = (shakeDuration > 0.0f) ? (shakeTime / shakeDuration) : 0.0f;
                float falloff = t * t;
                float phaseN = GetTime();
                float sx = sinf(phaseN * 35.0f);
                float sy = cosf(phaseN * 29.0f);
                float amp = shakeMagnitude * falloff;
                cam.target.x += sx * amp;
                cam.target.y += sy * amp;
            }

            // fire boulder
            if (monster.tryFireBoulder(boulders, cam))
            {
                // (maybe sound/vfx?)
            }

            // bite
            if (!monster.isTransforming() && !monster.isDashing())
            {
                monster.tryBite(animals, hunters);
            }

            // slam impact
            Vector2 slamPos;
            if (monster.consumeSlamImpact(slamPos))
            {
                impacts.push_back({slamPos, 0.25f, 0.0f});
                shakeDuration = 0.14f;
                shakeTime = shakeDuration;
                shakeMagnitude = 8.0f;

                if (phase == GamePhase::Escape)
                {
                    world.carveCircle(slamPos, 48.0f, false, nullptr);
                }
            }

            // decay squad intel
            if (squadIntel.timeToLive > 0.0f)
            {
                squadIntel.timeToLive -= dt;
                if (squadIntel.timeToLive < 0.0f)
                    squadIntel.timeToLive = 0.0f;
            }

            // hunters
            for (int i = 0; i < (int)hunters.size(); ++i)
            {
                auto &h = hunters[i];
                if (!h.isAlive())
                    continue;
                h.update(dt, world, monster, squadIntel);
                h.tryShoot(dt, world, monster, hunters, i, bullets);
            }

            // bullets
            for (auto &b : bullets)
                b.update(dt, world);

            // bullet hits
            for (auto &b : bullets)
            {
                if (!b.alive)
                    continue;

                if (b.team == Team::Hunter)
                {
                    if (monster.isAlive())
                    {
                        float dx = monster.getPosition().x - b.pos.x;
                        float dy = monster.getPosition().y - b.pos.y;
                        float rr = (monster.getRadius() + b.radius);
                        if (dx * dx + dy * dy <= rr * rr)
                        {
                            if (monster.applyHit(b.damage))
                            {
                                b.alive = false;
                            }
                            else
                            {
                                b.alive = false;
                            }
                            continue;
                        }
                    }
                }
                else
                {
                    for (auto &h : hunters)
                    {
                        if (!h.isAlive())
                            continue;
                        float dx = h.pos.x - b.pos.x, dy = h.pos.y - b.pos.y;
                        float rr = (h.radius + b.radius);
                        if (dx * dx + dy * dy <= rr * rr)
                        {
                            h.takeDamage(b.damage);
                            b.alive = false;
                            break;
                        }
                    }
                }
            }

            // cleanup bullets
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                         [](const Bullet &b)
                                         { return !b.alive; }),
                          bullets.end());

            // cleanup hunters
            hunters.erase(std::remove_if(hunters.begin(), hunters.end(),
                                         [](const Hunter &h)
                                         { return !h.isAlive(); }),
                          hunters.end());

            // animals
            for (auto &a : animals)
                a.update(dt, world);
            animals.erase(std::remove_if(animals.begin(), animals.end(),
                                         [](const Animal &a)
                                         { return !a.alive; }),
                          animals.end());

            // boulders
            for (auto &b : boulders)
            {
                bool exploded = b.update(dt, world, animals, hunters, monster);
                if (exploded)
                {
                    // indent map
                    world.carveCircle(b.pos, 50.0f, true, nullptr);

                    if (phase == GamePhase::Escape)
                    {
                        world.carveCircle(b.pos, 48.0f, false, nullptr);
                    }

                    // animal AoE
                    float aoe = 64.0f;
                    for (auto &a : animals)
                    {
                        if (!a.alive)
                            continue;
                        float dx = a.pos.x - b.pos.x, dy = a.pos.y - b.pos.y;
                        if (dx * dx + dy * dy <= aoe * aoe)
                            a.alive = false;
                    }

                    // hunter AoE
                    for (auto &h : hunters)
                    {
                        float dx = h.pos.x - b.pos.x, dy = h.pos.y - b.pos.y;
                        float rr = (b.radius + h.radius);
                        if (dx * dx + dy * dy <= rr * rr)
                        {
                            h.applyHit(monster.boulderAoeDamage(), b.pos, 180.0f);
                        }
                    }

                    impacts.push_back({b.pos, 0.2f, 0.0f});
                    shakeDuration = 0.15f;
                    shakeTime = shakeDuration;
                    shakeMagnitude = 6.0f;
                }
            }
            boulders.erase(std::remove_if(boulders.begin(), boulders.end(),
                                          [](const Boulder &b)
                                          { return !b.alive; }),
                           boulders.end());

            // impacts
            for (auto &fx : impacts)
                fx.elapsed += dt;

            // growth -> hunt
            if (phase == GamePhase::Grow && monster.getStage() == 4)
            {
                phase = GamePhase::Hunt;
                bannerTimer = 3.0f;
            }

            // objective/ending
            if (phase == GamePhase::Hunt && hunters.empty())
            {
                phase = GamePhase::Escape;
                world.setAllowBorderBreak(true);
                bannerTimer = 3.0f;
            }
            if (phase == GamePhase::Escape && !exitActive)
            {
                Vector2 breach;
                bool flagged = world.consumeBorderBreach(breach);
                bool foundHole = false;

                // If the flag didn't trigger, double-check the actual border state
                if (!flagged)
                {
                    foundHole = FindBorderGap(world, breach);
                }

                if (flagged || foundHole)
                {
                    exitActive = true;
                    exitPos = breach;
                    bannerTimer = 3.0f;
                }
            }
            if (exitActive && phase == GamePhase::Escape)
            {
                float dx = monster.getPosition().x - exitPos.x;
                float dy = monster.getPosition().y - exitPos.y;
                if (dx * dx + dy * dy <= (monster.getRadius() + 28.0f) * (monster.getRadius() + 28.0f))
                {
                    phase = GamePhase::Won;
                    bannerTimer = 4.0f;
                }
            }
        }

        // light mask
        BeginTextureMode(lightRT);
        ClearBackground(BLACK);

        BeginMode2D(cam);
        {
            // Player light
            Vector2 p = monster.getPosition();
            float outerR = monster.getVisionRadius();
            float innerR = monster.getInnerLightRadius();
            DrawCircleV(p, innerR, WHITE);
            DrawCircleGradient((int)p.x, (int)p.y, outerR, WHITE, BLACK);

            // Impacts emit light
            for (auto &fx : impacts)
            {
                float t = fx.elapsed / fx.time;
                float a = 1.0f - t;
                float r = 60.0f + 120.0f * t;
                DrawCircleV(fx.pos, r, Fade(WHITE, a));
            }

            // Hunters aura and flashlight cone
            for (auto &h : hunters)
            {
                // Aura
                float auraOuter = 70.0f;
                float auraInner = 45.0f;
                DrawCircleV(h.pos, auraInner, WHITE);
                DrawCircleGradient((int)h.pos.x, (int)h.pos.y, auraOuter, WHITE, BLACK);

                // cone beam
                float beamRange = 360.0f;
                float halfFov = h.fovDeg * 0.5f;
                float startDeg = h.facingRad * RAD2DEG - halfFov;
                float endDeg = h.facingRad * RAD2DEG + halfFov;

                const int LAYERS = 4;
                for (int i = 0; i <= LAYERS; ++i)
                {
                    float t = (float)i / (float)(LAYERS + 1);
                    float r = beamRange * (1.0f + 0.12f * t);
                    Color c = Fade(WHITE, 0.35f * (1.0f - t));
                    DrawCircleSector(h.pos, r, startDeg, endDeg, 40, c);
                }
            }

            // Exit marker
            if (exitActive)
            {
                float t = (float)GetTime();
                float r = 28.0f + 4.0f * sinf(t * 4.0f);
                DrawCircleLines((int)exitPos.x, (int)exitPos.y, r, GOLD);
                DrawCircleV(exitPos, 6.0f, Fade(GOLD, 0.8f));
            }
        }
        EndMode2D();
        EndTextureMode();

        // draw EVERYTHING
        BeginDrawing();
        ClearBackground(Color{12, 30, 28, 255});

        BeginMode2D(cam);
        world.draw();
        for (auto &a : animals)
            a.draw();
        for (auto &b : boulders)
            b.draw();
        for (auto &h : hunters)
            h.draw();
        for (auto &bul : bullets)
            bul.draw();
        monster.draw();
        EndMode2D();

        // light
        BeginBlendMode(BLEND_MULTIPLIED);
        Rectangle src = {0, 0, (float)lightRT.texture.width, -(float)lightRT.texture.height};
        Rectangle dst = {0, 0, (float)screenW, (float)screenH};
        DrawTexturePro(lightRT.texture, src, dst, {0, 0}, 0.0f, WHITE);
        EndBlendMode();

        // hud and overlays
        //  objective banner
        if (bannerTimer > 0.0f)
            bannerTimer -= dt;

        const char *objective = nullptr;
        if (phase == GamePhase::Grow)
            objective = "Objective: FEED, GROW, SURVIVE";
        else if (phase == GamePhase::Hunt)
            objective = TextFormat("Objective: ELIMINATE HUNTERS (%d left)", (int)hunters.size());
        else if (phase == GamePhase::Escape)
            objective = exitActive ? "Objective: ESCAPE THROUGH THE BREACH" : "Objective: BREAK THE BORDER WALL TO ESCAPE";
        else if (phase == GamePhase::Won)
            objective = "YOU ESCAPED! Thanks for playing.";

        if (objective)
        {
            float alpha = (phase == GamePhase::Won) ? 1.0f : (bannerTimer > 0.0f ? fminf(bannerTimer / 3.0f, 1.0f) : 0.9f);
            Color c = Fade(WHITE, alpha);
            int tw = MeasureText(objective, 26);
            DrawText(objective, GetScreenWidth() / 2 - tw / 2, 16, 26, c);
        }

        // Compass arrow in Escape
        if (phase == GamePhase::Escape)
        {
            Vector2 playerPos = monster.getPosition();
            Vector2 targetWorld = exitActive ? exitPos : NearestBorderPoint(world, playerPos);
            Vector2 to = {targetWorld.x - playerPos.x, targetWorld.y - playerPos.y};
            float L = sqrtf(to.x * to.x + to.y * to.y);
            if (L > 1e-4f)
            {
                to.x /= L;
                to.y /= L;
            }
            else
            {
                to = {1, 0};
            }
            float ang = atan2f(to.y, to.x);

            Vector2 screenCenter = {(float)screenW * 0.5f, (float)screenH * 0.5f};
            float margin = 36.0f;
            float maxX = screenCenter.x - margin;
            float maxY = screenCenter.y - margin;
            float sx = (to.x == 0.0f) ? 1e9f : maxX / fabsf(to.x);
            float sy = (to.y == 0.0f) ? 1e9f : maxY / fabsf(to.y);
            float k = fminf(sx, sy);
            Vector2 arrowPos = {screenCenter.x + to.x * k, screenCenter.y + to.y * k};

            float size = 18.0f;
            Vector2 a = {+size, 0};
            Vector2 b = {-size * 0.6f, +size * 0.6f};
            Vector2 c3 = {-size * 0.6f, -size * 0.6f};
            float cs = cosf(ang), sn = sinf(ang);
            auto rot = [&](Vector2 v)
            { return Vector2{v.x * cs - v.y * sn, v.x * sn + v.y * cs}; };
            a = rot(a);
            b = rot(b);
            c3 = rot(c3);
            a.x += arrowPos.x;
            a.y += arrowPos.y;
            b.x += arrowPos.x;
            b.y += arrowPos.y;
            c3.x += arrowPos.x;
            c3.y += arrowPos.y;

            DrawCircleLines((int)screenCenter.x, (int)screenCenter.y, 22, Fade(GOLD, 0.25f));
            DrawTriangle(c3, b, a, Color{0, 0, 0, 120});
            DrawTriangle({c3.x - 1, c3.y - 1}, {b.x - 1, b.y - 1}, {a.x - 1, a.y - 1}, Color{255, 255, 255, 120});
            DrawTriangle(b, c3, a, GOLD);
            DrawTriangleLines(b, c3, a, GOLD);
        }

        // HUD pos
        int hudX = 20, hudY = 20;

        // Player HP
        int hpX = 20, hpY = GetScreenHeight() - 40;
        int barW = 240, barH = 16;
        DrawRectangleLines(hpX - 2, hpY - 2, barW + 4, barH + 4, WHITE);
        float hpFrac = monster.getHP() / monster.getMaxHP();
        DrawRectangle(hpX, hpY, (int)(barW * fmaxf(0.0f, hpFrac)), barH, (hpFrac > 0.3f) ? GREEN : RED);
        DrawText(TextFormat("HP: %d / %d", (int)monster.getHP(), (int)monster.getMaxHP()), hpX, hpY - 22, 18, WHITE);

        // Stage/food
        DrawText(TextFormat("Stage: %d", monster.getStage()), hudX, hudY, 22, WHITE);
        DrawText(TextFormat("Food: %d / %d", monster.getFood(), monster.getStageFoodCost()), hudX, hudY + 24, 20, WHITE);

        // Bite cooldown
        float biteFrac = monster.getBiteCooldownFraction();
        DrawRectangleLines(hudX - 2, hudY + 50 - 2, 104, 24, WHITE);
        Color biteCol = (biteFrac > 0.0f) ? RED : GREEN;
        int biteFill = (int)(100 * (1.0f - biteFrac));
        DrawRectangle(hudX, hudY + 50, biteFill, 20, biteCol);
        DrawText("BITE", hudX + 110, hudY + 50, 20, WHITE);

        // Dash cooldown (stage 2+)
        if (monster.getStage() >= 2)
        {
            float dashFrac = monster.getDashCooldownFraction();
            int dy = hudY + 80;
            DrawRectangleLines(hudX - 2, dy - 2, 154, 24, WHITE);
            Color dashCol = (dashFrac > 0.0f) ? SKYBLUE : BLUE;
            int dashFill = (int)(150 * (1.0f - dashFrac));
            DrawRectangle(hudX, dy, dashFill, 20, dashCol);
            DrawText("DASH", hudX + 160, dy + 2, 20, WHITE);
        }

        // Dash hint
        if (monster.showDashHint)
        {
            float alpha = fminf(monster.dashHintTimer / 4.0f, 1.0f);
            Color c = Fade(SKYBLUE, alpha);
            const char *msg = "New Ability Unlocked! Press [SHIFT] to Dash";
            int tw = MeasureText(msg, 28);
            DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() - 100, 28, c);
        }

        // Boulder cooldown (stage 3+)
        if (monster.getStage() >= 3)
        {
            float bFrac = monster.getBoulderCooldownFraction();
            int dy = hudY + 110;
            DrawRectangleLines(hudX - 2, dy - 2, 154, 24, WHITE);
            DrawRectangle(hudX, dy, (int)(150 * (1.0f - bFrac)), 20,
                          (bFrac > 0.0f) ? BROWN : Color{150, 90, 40, 255});
            DrawText("BOULDER", hudX + 160, dy + 2, 20, WHITE);
        }

        // Slam cooldown (stage 4+)
        if (monster.getStage() >= 4)
        {
            float sFrac = monster.getSlamCooldownFraction();
            int sy = hudY + 140;
            DrawRectangleLines(hudX - 2, sy - 2, 154, 24, WHITE);
            Color sCol = (sFrac > 0.0f) ? RED : MAROON;
            DrawRectangle(hudX, sy, (int)(150 * (1.0f - sFrac)), 20, sCol);
            DrawText("SLAM", hudX + 160, sy + 2, 20, WHITE);
        }

        // Evolve prompt
        if (!monster.isTransforming() && monster.isEvolveReady())
        {
            const char *msg = "Press E to EVOLVE";
            int tw = MeasureText(msg, 28);
            DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 60, 28, GOLD);
        }

        // Transform progress
        if (monster.isTransforming())
        {
            float p = monster.transformProgress();
            int bw = 300, bh = 16;
            int bx = GetScreenWidth() / 2 - bw / 2;
            int by = GetScreenHeight() - 80;
            DrawRectangleLines(bx - 2, by - 2, bw + 4, bh + 4, WHITE);
            DrawRectangle(bx, by, (int)(bw * p), bh, YELLOW);
            int tw = MeasureText("Transforming... Vulnerable!", 20);
            DrawText("Transforming... VULNERABLE!", GetScreenWidth() / 2 - tw / 2, by - 26, 20, ORANGE);
        }

        // pause overlay
        if (phase == GamePhase::Pause)
        {
            DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 140});
            const char *paused = "PAUSED";
            int pW = MeasureText(paused, 48);
            DrawText(paused, screenW / 2 - pW / 2, 140, 48, WHITE);

            float bw = 260, bh = 56, gap = 16;
            Rectangle rResume = {(float)screenW / 2 - bw / 2, 240, bw, bh};
            Rectangle rRestart = {(float)screenW / 2 - bw / 2, 240 + (bh + gap), bw, bh};
            Rectangle rMenu = {(float)screenW / 2 - bw / 2, 240 + 2 * (bh + gap), bw, bh};

            if (DrawButton(rResume, "RESUME"))
                phase = phaseBeforePause;
            if (DrawButton(rRestart, "RESTART"))
            {
                resetGame();
            }
            if (DrawButton(rMenu, "MAIN MENU"))
                phase = GamePhase::MainMenu;
        }

        if (phase == GamePhase::GameOver)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 180});

            const char *dead = "YOU DIED";
            int dw = MeasureText(dead, 64);
            DrawText(dead, GetScreenWidth() / 2 - dw / 2, 150, 64, RED);

            // small stats line (optional)
            const char *stat = TextFormat("Stage %d  |  Hunters remaining: %d",
                                          monster.getStage(), (int)hunters.size());
            int sw = MeasureText(stat, 22);
            DrawText(stat, GetScreenWidth() / 2 - sw / 2, 230, 22, WHITE);

            float bw = 260, bh = 56, gap = 16;
            Rectangle rRetry = {(float)GetScreenWidth() / 2 - bw / 2, 300, bw, bh};
            Rectangle rMenu = {(float)GetScreenWidth() / 2 - bw / 2, 300 + (bh + gap), bw, bh};

            if (DrawButton(rRetry, "RETRY"))
            {
                // call your reset function (you already have it)
                // e.g., resetGame();
                // If resetGame() is a lambda, make sure it's in scope here.
                // (It is in your previous file.)
                resetGame();
            }
            if (DrawButton(rMenu, "MAIN MENU"))
            {
                phase = GamePhase::MainMenu;
            }
        }

        // victory overlay
        if (phase == GamePhase::Won)
        {
            DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 160});
            const char *gg = "YOU ESCAPED!";
            int gw = MeasureText(gg, 48);
            DrawText(gg, screenW / 2 - gw / 2, 160, 48, GOLD);

            float bw = 260, bh = 56, gap = 16;
            Rectangle rRestart = {(float)screenW / 2 - bw / 2, 260, bw, bh};
            Rectangle rMenu = {(float)screenW / 2 - bw / 2, 260 + (bh + gap), bw, bh};
            if (DrawButton(rRestart, "PLAY AGAIN"))
            {
                resetGame();
            }
            if (DrawButton(rMenu, "MAIN MENU"))
                phase = GamePhase::MainMenu;
        }

        EndDrawing();
    }

    UnloadRenderTexture(lightRT);
    CloseWindow();
    return 0;
}
