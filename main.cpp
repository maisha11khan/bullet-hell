#include "raylib.h"
#include <cmath>

const int GAME_W = 800;
const int GAME_H = 600;

const int MAX_BULLETS = 10000;
const int MAX_PARTICLES = 2000;
const int ENEMY_MAX_HEALTH = 200;
const float ATTACK_TIME = 8.0f;
const float REST_TIME = 3.0f;
const int START_BOMBS = 3;
const int START_LIVES = 5;

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    bool active;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
    bool active;
};

enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, WON };

Bullet playerBullets[MAX_BULLETS];
Bullet enemyBullets[MAX_BULLETS];
Particle particles[MAX_PARTICLES];

// ---------- Text helpers (custom font) ----------
Font gameFont;

void Txt(const char* text, int x, int y, int size, Color color) {
    DrawTextEx(gameFont, text, { (float)x, (float)y }, (float)size, 1, color);
}

void TxtCenter(const char* text, int y, int size, Color color) {
    Vector2 m = MeasureTextEx(gameFont, text, (float)size, 1);
    DrawTextEx(gameFont, text, { GAME_W / 2 - m.x / 2, (float)y }, (float)size, 1, color);
}

// ---------- Bullet helpers ----------
void SpawnBullet(Bullet* pool, float x, float y, float vx, float vy) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!pool[i].active) {
            pool[i].pos = { x, y };
            pool[i].vel = { vx, vy };
            pool[i].active = true;
            return;
        }
    }
}

void UpdateBullets(Bullet* pool, float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (pool[i].active) {
            pool[i].pos.x += pool[i].vel.x * dt;
            pool[i].pos.y += pool[i].vel.y * dt;
            if (pool[i].pos.x < -20 || pool[i].pos.x > GAME_W + 20 ||
                pool[i].pos.y < -20 || pool[i].pos.y > GAME_H + 20) {
                pool[i].active = false;
            }
        }
    }
}

void ClearBullets(Bullet* pool) {
    for (int i = 0; i < MAX_BULLETS; i++) pool[i].active = false;
}

void FireRing(Vector2 from, int n, float startAngle, float speed) {
    for (int i = 0; i < n; i++) {
        float a = startAngle + i * (2 * PI / n);
        SpawnBullet(enemyBullets, from.x, from.y, cosf(a) * speed, sinf(a) * speed);
    }
}

void FireAimed(Vector2 from, Vector2 target, int count, float spread, float speed) {
    float baseAngle = atan2f(target.y - from.y, target.x - from.x);
    for (int i = 0; i < count; i++) {
        float a = baseAngle + (i - (count - 1) / 2.0f) * spread;
        SpawnBullet(enemyBullets, from.x, from.y, cosf(a) * speed, sinf(a) * speed);
    }
}

// ---------- Particle helpers ----------
void SpawnParticles(Vector2 pos, int n, float speed, float life, float size, Color color) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                float a = GetRandomValue(0, 628) / 100.0f;
                float s = speed * (0.3f + GetRandomValue(0, 100) / 100.0f);
                particles[i].pos = pos;
                particles[i].vel = { cosf(a) * s, sinf(a) * s };
                particles[i].life = life;
                particles[i].maxLife = life;
                particles[i].size = size;
                particles[i].color = color;
                particles[i].active = true;
                break;
            }
        }
    }
}

void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].pos.x += particles[i].vel.x * dt;
            particles[i].pos.y += particles[i].vel.y * dt;
            particles[i].life -= dt;
            if (particles[i].life <= 0) particles[i].active = false;
        }
    }
}

void ClearParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GAME_W, GAME_H, "Bullet Hell");
    SetWindowMinSize(400, 300);
    SetTargetFPS(60);

    // The game is always drawn on this 800 x 600 canvas,
    // then the canvas is scaled to fit the window.
    RenderTexture2D canvas = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    bool customFont = true;
    gameFont = LoadFontEx("C:/Windows/Fonts/consola.ttf", 48, 0, 0);
    if (gameFont.texture.id == 0) {
        gameFont = GetFontDefault();
        customFont = false;
    } else {
        SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    Vector2 ship;
    float shootTimer;
    Vector2 enemy;
    float enemyDir;
    int enemyHealth;
    float ringTimer, aimTimer, spiralTimer, ringAngle, spiralAngle;
    bool resting;
    float modeTimer;
    int lives, bombs;
    float bombFlash, invincible;
    float shake = 0;
    float enemyFlash = 0;
    bool deathBoom = false;

    auto ResetGame = [&]() {
        ClearBullets(playerBullets);
        ClearBullets(enemyBullets);
        ClearParticles();
        ship = { 400, 500 };
        shootTimer = 0;
        enemy = { 400, 100 };
        enemyDir = 1;
        enemyHealth = ENEMY_MAX_HEALTH;
        ringTimer = 2.0f;
        aimTimer = 2.0f;
        spiralTimer = 0;
        ringAngle = 0;
        spiralAngle = 0;
        resting = false;
        modeTimer = ATTACK_TIME;
        lives = START_LIVES;
        bombs = START_BOMBS;
        bombFlash = 0;
        invincible = 0;
        shake = 0;
        enemyFlash = 0;
        deathBoom = false;
    };

    ResetGame();
    GameState state = MENU;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;   // avoids big jumps while resizing the window

        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

        if (state == MENU) {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
                state = PLAYING;
            }
        }
        else if (state == PAUSED) {
            if (IsKeyPressed(KEY_P)) state = PLAYING;
        }
        else if (state == PLAYING) {
            if (IsKeyPressed(KEY_P)) state = PAUSED;

            // Movement (Shift = slow mode)
            float speed = 300;
            if (IsKeyDown(KEY_LEFT_SHIFT)) speed = 120;

            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  ship.x -= speed * dt;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) ship.x += speed * dt;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    ship.y -= speed * dt;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  ship.y += speed * dt;

            if (ship.x < 15) ship.x = 15;
            if (ship.x > GAME_W - 15) ship.x = GAME_W - 15;
            if (ship.y < 15) ship.y = 15;
            if (ship.y > GAME_H - 15) ship.y = GAME_H - 15;

            // Player shooting
            shootTimer -= dt;
            if (IsKeyDown(KEY_SPACE) && shootTimer <= 0) {
                SpawnBullet(playerBullets, ship.x, ship.y - 20, 0, -700);
                shootTimer = 0.1f;
            }

            // Bomb
            if (IsKeyPressed(KEY_X) && bombs > 0) {
                bombs--;
                ClearBullets(enemyBullets);
                bombFlash = 0.3f;
                shake = 10;
                SpawnParticles(ship, 40, 350, 0.6f, 4, SKYBLUE);
                if (invincible < 1.0f) invincible = 1.0f;
            }
            if (bombFlash > 0) bombFlash -= dt;

            // Enemy moves
            enemy.x += enemyDir * 80 * dt;
            if (enemy.x > 650) enemyDir = -1;
            if (enemy.x < 150) enemyDir = 1;

            // Attack / rest cycle
            modeTimer -= dt;
            if (modeTimer <= 0) {
                resting = !resting;
                if (resting) {
                    modeTimer = REST_TIME;
                } else {
                    modeTimer = ATTACK_TIME;
                    ringTimer = 0.5f;
                    aimTimer = 1.0f;
                    spiralTimer = 0;
                }
            }

            int phase = 1;
            if (enemyHealth <= ENEMY_MAX_HEALTH * 2 / 3) phase = 2;
            if (enemyHealth <= ENEMY_MAX_HEALTH / 3)     phase = 3;

            ringTimer -= dt;
            aimTimer -= dt;
            spiralTimer -= dt;

            if (!resting) {
                if (phase == 1) {
                    if (ringTimer <= 0) {
                        FireRing(enemy, 12, ringAngle, 150);
                        ringAngle += 0.2f;
                        ringTimer = 1.5f;
                    }
                }
                else if (phase == 2) {
                    if (aimTimer <= 0) {
                        FireAimed(enemy, ship, 3, 0.25f, 170);
                        aimTimer = 1.5f;
                    }
                    if (ringTimer <= 0) {
                        FireRing(enemy, 10, ringAngle, 140);
                        ringAngle += 0.3f;
                        ringTimer = 2.5f;
                    }
                }
                else {
                    if (spiralTimer <= 0) {
                        SpawnBullet(enemyBullets, enemy.x, enemy.y,
                                    cosf(spiralAngle) * 110, sinf(spiralAngle) * 110);
                        SpawnBullet(enemyBullets, enemy.x, enemy.y,
                                    cosf(spiralAngle + PI) * 110, sinf(spiralAngle + PI) * 110);
                        spiralAngle += 0.25f;
                        spiralTimer = 0.18f;
                    }
                    if (aimTimer <= 0) {
                        FireAimed(enemy, ship, 1, 0, 240);
                        aimTimer = 2.5f;
                    }
                }
            }

            UpdateBullets(playerBullets, dt);
            UpdateBullets(enemyBullets, dt);

            // Player bullets hit the boss
            Rectangle enemyBox = { enemy.x - 60, enemy.y - 25, 120, 50 };
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (playerBullets[i].active &&
                    CheckCollisionPointRec(playerBullets[i].pos, enemyBox)) {
                    SpawnParticles(playerBullets[i].pos, 3, 200, 0.3f, 3, ORANGE);
                    playerBullets[i].active = false;
                    enemyHealth -= resting ? 2 : 1;
                    enemyFlash = 0.06f;
                }
            }
            if (enemyFlash > 0) enemyFlash -= dt;

            // Enemy bullets hit the player
            if (invincible > 0) invincible -= dt;
            if (invincible <= 0) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (enemyBullets[i].active &&
                        CheckCollisionCircles(ship, 3, enemyBullets[i].pos, 4)) {
                        lives--;
                        invincible = 2.0f;
                        shake = 14;
                        SpawnParticles(ship, 35, 300, 0.7f, 4, RED);
                        ClearBullets(enemyBullets);
                        break;
                    }
                }
            }

            if (enemyHealth <= 0) {
                state = WON;
                if (!deathBoom) {
                    deathBoom = true;
                    shake = 18;
                    ClearBullets(enemyBullets);
                    SpawnParticles(enemy, 150, 400, 1.0f, 5, GOLD);
                    SpawnParticles(enemy, 100, 250, 1.2f, 4, PURPLE);
                }
            }
            if (lives <= 0) state = GAME_OVER;
        }
        else {
            // GAME_OVER or WON
            if (IsKeyPressed(KEY_R)) {
                ResetGame();
                state = PLAYING;
            }
            if (IsKeyPressed(KEY_M)) state = MENU;
        }

        // Effects keep running in every state except pause
        if (state != PAUSED) {
            UpdateParticles(dt);
            if (shake > 0) {
                shake -= 40 * dt;
                if (shake < 0) shake = 0;
            }
        }

        // ---------- Drawing (onto the 800 x 600 canvas) ----------
        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        if (state == MENU) {
            TxtCenter("BULLET HELL", 90, 72, SKYBLUE);
            TxtCenter("Dodge the bullets. Defeat the boss.", 185, 24, LIGHTGRAY);

            TxtCenter("Controls", 255, 30, ORANGE);
            Txt("WASD / Arrows : Move",      240, 305, 22, WHITE);
            Txt("Hold SPACE    : Shoot",     240, 335, 22, WHITE);
            Txt("Hold SHIFT    : Slow move", 240, 365, 22, WHITE);
            Txt("X             : Bomb",      240, 395, 22, WHITE);
            Txt("P             : Pause",     240, 425, 22, WHITE);
            Txt("F11           : Fullscreen",240, 455, 22, WHITE);

            if ((int)(GetTime() * 2) % 2 == 0) {
                TxtCenter("Press ENTER to start", 520, 30, GREEN);
            }
        }
        else {
            Camera2D cam = { 0 };
            cam.zoom = 1.0f;
            if (shake > 0) {
                cam.offset = { (float)GetRandomValue(-100, 100) / 100.0f * shake,
                               (float)GetRandomValue(-100, 100) / 100.0f * shake };
            }
            BeginMode2D(cam);

            int count = 0;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (playerBullets[i].active) {
                    DrawCircleV(playerBullets[i].pos, 4, YELLOW);
                    count++;
                }
                if (enemyBullets[i].active) {
                    DrawCircleV(enemyBullets[i].pos, 4, RED);
                    count++;
                }
            }

            if (state != WON) {
                Rectangle box = { enemy.x - 60, enemy.y - 25, 120, 50 };
                Color bossColor = resting ? GOLD : PURPLE;
                if (enemyFlash > 0) bossColor = WHITE;
                DrawRectangleRec(box, bossColor);
            }

            bool showShip = true;
            if (invincible > 0 && ((int)(invincible * 10) % 2 == 0)) showShip = false;
            if (state == GAME_OVER) showShip = false;
            if (showShip) {
                DrawCircleV(ship, 15, SKYBLUE);
                DrawCircleV(ship, 3, WHITE);
            }

            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (particles[i].active) {
                    float t = particles[i].life / particles[i].maxLife;
                    DrawCircleV(particles[i].pos, particles[i].size * t,
                                Fade(particles[i].color, t));
                }
            }

            EndMode2D();

            // HUD (does not shake)
            if (enemyHealth > 0) {
                DrawRectangle(20, 570, (int)(enemyHealth * 3.5f), 12, GREEN);
            }

            int shownPhase = 1;
            if (enemyHealth <= ENEMY_MAX_HEALTH * 2 / 3) shownPhase = 2;
            if (enemyHealth <= ENEMY_MAX_HEALTH / 3)     shownPhase = 3;
            TxtCenter(TextFormat("Phase: %d / 3", shownPhase), 10, 24, ORANGE);

            if (resting && state == PLAYING) {
                TxtCenter("BREAK! Hit it now! (double damage)", 45, 22, GOLD);
            }

            if (bombFlash > 0) {
                DrawRectangle(0, 0, GAME_W, GAME_H, Fade(WHITE, bombFlash / 0.3f * 0.6f));
            }

            DrawFPS(10, 10);
            Txt(TextFormat("Bullets: %d", count), 10, 35, 20, WHITE);
            Txt(TextFormat("Lives: %d", lives), 660, 10, 24, WHITE);
            Txt(TextFormat("Bombs: %d (X)", bombs), 620, 40, 20, ORANGE);

            if (state == PAUSED) {
                DrawRectangle(0, 0, GAME_W, GAME_H, Fade(BLACK, 0.6f));
                TxtCenter("PAUSED", 250, 50, WHITE);
                TxtCenter("Press P to continue", 320, 24, LIGHTGRAY);
            }
            if (state == GAME_OVER) {
                TxtCenter("GAME OVER", 245, 54, RED);
                TxtCenter("R: restart    M: menu", 320, 24, WHITE);
            }
            if (state == WON) {
                TxtCenter("YOU WIN!", 245, 54, GREEN);
                TxtCenter("R: play again    M: menu", 320, 24, WHITE);
            }
        }

        EndTextureMode();

        // ---------- Show the canvas scaled to the window ----------
        BeginDrawing();
        ClearBackground(BLACK);

        float scale = fminf((float)GetScreenWidth() / GAME_W,
                            (float)GetScreenHeight() / GAME_H);
        float w = GAME_W * scale;
        float h = GAME_H * scale;
        Rectangle src = { 0, 0, (float)GAME_W, -(float)GAME_H };
        Rectangle dst = { (GetScreenWidth() - w) / 2, (GetScreenHeight() - h) / 2, w, h };
        DrawTexturePro(canvas.texture, src, dst, { 0, 0 }, 0, WHITE);

        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    if (customFont) UnloadFont(gameFont);
    CloseWindow();
    return 0;
}