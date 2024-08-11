#include <stdlib.h>
#include <stdio.h>

#define GLSL_VERSION 450

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define LAZY_INIT 0

struct {
    unsigned int SCREEN_WIDTH;
    unsigned int SCREEN_HEIGHT;
} GLOBAL = {
    .SCREEN_WIDTH = LAZY_INIT,
    .SCREEN_HEIGHT = LAZY_INIT,
};

void global_set_screen_size(unsigned int width, unsigned int height) {
    GLOBAL.SCREEN_WIDTH = width;
    GLOBAL.SCREEN_HEIGHT = height;
}

typedef struct {
    float head_radius;
    Vector2 base;
    Vector2 direction; // normalized
    float length;
} Arrow;

Vector2 arrow_get_head(Arrow arrow) {
    return Vector2Add(
        arrow.base,
        Vector2Scale(arrow.direction, arrow.length)
    );
}

void arrow_draw(Arrow arrow, float thick, Color color) {
    Vector2 b = Vector2Zero();
    Vector2 h = Vector2Scale(arrow.direction, arrow.length);
    Vector2 hu = Vector2Add(h, Vector2Scale(Vector2Rotate(arrow.direction, DEG2RAD * -150), arrow.head_radius));
    Vector2 hd = Vector2Add(h, Vector2Scale(Vector2Rotate(arrow.direction, DEG2RAD * -210), arrow.head_radius));

    b = Vector2Add(arrow.base, b);
    h = Vector2Add(arrow.base, h);
    hu = Vector2Add(arrow.base, hu);
    hd = Vector2Add(arrow.base, hd);

    DrawLineEx(b, h, thick, color);
    DrawLineEx(h, hu, thick, color);
    DrawLineEx(h, hd, thick, color);
}

typedef struct {
    Vector2 position;
    Vector2 direction;
    float speed;
    float accelaration;
    float __drag;
    //
    int rotation_direction;
    float rotation_speed; // DEG
    //
    Vector2 aim_direction;
} Player;

Player player_create_default() {
    return (Player) {
        .position = 0,
        .direction = {0, -1},
        .speed = 100,
        .accelaration = 0,
        .__drag = 0,
        .rotation_direction = 0,
        .rotation_speed = 100,
        .aim_direction = {1, 0},
    };
}

void player_process_input(Player* player) {
    if (IsKeyDown(KEY_UP) && IsKeyDown(KEY_DOWN)) {
        player->accelaration = 0;
    }
    else if (IsKeyDown(KEY_UP)) {
        player->accelaration = 1000;
    }
    else if (IsKeyDown(KEY_DOWN)) {
        player->accelaration = -100;
    }
    else {
        player->accelaration = 0;
    }

    if (IsKeyDown(KEY_RIGHT) && IsKeyDown(KEY_LEFT)) {
        player->rotation_direction = 0;
    }
    else if (IsKeyDown(KEY_RIGHT)) {
        player->rotation_direction = 1;
    }
    else if (IsKeyDown(KEY_LEFT)) {
        player->rotation_direction = -1;
    }
    else {
        player->rotation_direction = 0;
    }
}

void player_update(Player* player, Camera2D camera, float delta_time) {
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

    const float DRAG_COEFF = 1.0 / 800;
    const float CUT_SECTION = 1;
    float drag = DRAG_COEFF * CUT_SECTION * ((player->speed * player->speed)/2);
    player->__drag = drag;

    player->speed += (player->accelaration - drag) * delta_time;
    player->speed = Clamp(player->speed, 0.0, player->speed);

    player->direction = Vector2Rotate(
        player->direction,
        player->rotation_direction * DEG2RAD * player->rotation_speed * delta_time
    );

    player->position = Vector2Add(
        player->position,
        Vector2Scale(player->direction, player->speed * delta_time)
    );

    player->aim_direction = Vector2Normalize(Vector2Subtract(mouse, player->position));
}

void player_draw(Player* player) {
    float player_radius = 50;

    arrow_draw(
        (Arrow) {
            .head_radius = 30,
            .base = player->position,
            .direction = player->direction,
            .length = player_radius + 10*10,
        },
        5,
        RED
    );
    DrawCircleV(player->position, player_radius, WHITE);

    arrow_draw(
        (Arrow) {
            .head_radius = 30,
            .base = player->position,
            .direction = player->aim_direction,
            .length = player_radius + 10*10,
        },
        5,
        GREEN
    );
}

typedef struct {
    Shader shader;
    float time;
    float speed;
    Texture noise_texture;
    Texture wave_texture;
} FireTrailMaterial;

void fire_trail_material_set_shader(FireTrailMaterial* ftm) {
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "time"),
        &ftm->time,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "speed"),
        &ftm->speed,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValueTexture(
        ftm->shader,
        GetShaderLocation(ftm->shader, "noiseTexture"),
        ftm->noise_texture
    );
}

void fire_trail_material_draw(FireTrailMaterial* ftm) {
    BeginShaderMode(ftm->shader);
        DrawTexturePro(
            ftm->wave_texture,
            (Rectangle) { 0, 0, ftm->wave_texture.width, ftm->wave_texture.height },
            (Rectangle) {0, 0, 512, 512},
            (Vector2) {0, 0},
            0,
            WHITE
        );
    EndShaderMode();
}

typedef struct {
    FireTrailMaterial material;
    RenderTexture2D fire_trail_target_texture;
    bool paused;
    Vector2 base;
    Vector2 direction;
    float length;
    float girth;
} FireTrail;

void fire_trail_process_input(FireTrail* fire_trail) {
    if (IsKeyPressed(KEY_SPACE)) {
        fire_trail->paused ^= 1;
    }
}

void fire_trail_update(FireTrail* fire_trail, float delta_time, Camera2D camera, Player* player) {
    fire_trail->base = Vector2Add(
        player->position,
        Vector2Scale(player->aim_direction, 50 + 10*10 + fire_trail->girth/8.0)
    );

    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    fire_trail->direction = Vector2Normalize(Vector2Subtract(mouse, fire_trail->base));

    if (!fire_trail->paused) {
        fire_trail->material.time += delta_time;
    }
}

void fire_trail_draw(FireTrail* fire_trail) {
    Texture tex = fire_trail->fire_trail_target_texture.texture;
    DrawTexturePro(
        tex,
        (Rectangle) { 0, 0, -tex.width, tex.height },
        (Rectangle) {
            .x = fire_trail->base.x,
            .y = fire_trail->base.y, //  - fire_trail->girth/2.0 + fire_trail->girth/2.0,
            .width = fire_trail->length,
            .height = fire_trail->girth,
        },
        // (Vector2) {0, 0},
        // 0,
        (Vector2) { 0, fire_trail->girth/2.0 },
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, fire_trail->direction),
        WHITE
    );
    printf("Angle: %d\n", (int)(RAD2DEG * Vector2Angle((Vector2) {1, 0}, fire_trail->direction)));
}

typedef struct {
    FireTrail fire_trail;
} PlayerHeart;

int main() {

    {
        global_set_screen_size(1920, 1080);
    }

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    {
        int monitor_id = GetMonitorCount() - 1;
        // Vector2 monitor_pos = GetMonitorPosition(monitor_id);
        // printf("Monitor Pos: %d %d\n", (int)monitor_pos.x, (int)monitor_pos.y);

        SetWindowMonitor(monitor_id);
        // SetWindowPosition(monitor_pos.x, monitor_pos.y);
        SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_BORDERLESS_WINDOWED_MODE);
    }

    Camera2D camera = {0};
    camera.zoom = 1;
    camera.offset = (Vector2) {
        .x = GLOBAL.SCREEN_WIDTH/2.0,
        .y = GLOBAL.SCREEN_HEIGHT/2.0,
    };

    Player player = player_create_default();

    Texture simple_noise_texture = LoadTexture("assets\\simple-noise.png");
    Texture fire_trail_wave_texture = LoadTexture("assets\\wave.png");

    RenderTexture2D fire_trail_target_texture = LoadRenderTexture(512, 512);
    Shader fire_trail_shader = LoadShader(0, "shaders\\fire_trail.fs"); // NOTE: use default vs

    int fts_time_loc =  GetShaderLocation(fire_trail_shader, "time");
    int fts_speed_loc =  GetShaderLocation(fire_trail_shader, "speed");
    int fts_noise_texture_loc =  GetShaderLocation(fire_trail_shader, "noiseTexture");

    FireTrail fire_trail = {
        .material = (FireTrailMaterial) {
            .shader = fire_trail_shader,
            .time = 0,
            .speed = 0.6,
            .noise_texture = simple_noise_texture,
            .wave_texture = fire_trail_wave_texture,
        },
        .fire_trail_target_texture = fire_trail_target_texture,
        .paused = false,
        .base = {0, 0},
        .direction = {1, 0},
        .length = 200,
        .girth = 50,
    };

    float speed = 0.6;

    while (!WindowShouldClose()) {
        float time = (float) GetTime();
        float delta_time = GetFrameTime();

        if (IsWindowResized()) {
            global_set_screen_size(GetScreenWidth(), GetScreenHeight());
            camera.offset = (Vector2) {
                .x = GLOBAL.SCREEN_WIDTH/2.0,
                .y = GLOBAL.SCREEN_HEIGHT/2.0,
            };
        }

        if (IsKeyPressed(KEY_P)) {
            Vector2 window_pos = GetWindowPosition();
            printf("Window Pos: %0.1f %0.1f\n", window_pos.x, window_pos.y);
            printf("Window Size: %d %d\n", GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT);
        }

        // INPUT
        player_process_input(&player);
        fire_trail_process_input(&fire_trail);

        if (IsKeyPressed(KEY_SPACE)) {
            speed = (speed == 0.0) ? 0.6 : 0.0;
        }

        // UPDATE
        //player_update(&player, camera, delta_time);
        fire_trail_update(&fire_trail, delta_time, camera, &player);
        camera.target = player.position;

        // DRAW
        BeginTextureMode(fire_trail_target_texture);
            ClearBackground(BLANK);
            BeginShaderMode(fire_trail_shader);

                SetShaderValue(fire_trail_shader, fts_time_loc, &time, SHADER_UNIFORM_FLOAT);
                SetShaderValue(fire_trail_shader, fts_speed_loc, &speed, SHADER_UNIFORM_FLOAT);
                SetShaderValueTexture(fire_trail_shader, fts_noise_texture_loc, simple_noise_texture);

                //DrawRectangle(0, 0, 512, 512, BLUE);
                DrawTexturePro(
                    fire_trail_wave_texture,
                    (Rectangle) { 0, 0, fire_trail_wave_texture.width, fire_trail_wave_texture.height },
                    (Rectangle) {0, 0, 512, 512},
                    (Vector2) {0, 0},
                    0,
                    WHITE
                );

            EndShaderMode();

            fire_trail_material_draw(&fire_trail.material);

        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(camera);

                DrawCircle(0, 0, 5, RED); // ORIGIN

                player_draw(&player);

                fire_trail_draw(&fire_trail);

                // DrawTexture(fire_trail_target_texture.texture, -256, -256, WHITE);
                // DrawTextureRec(
                //     fire_trail_target_texture.texture,
                //     (Rectangle){ 0, 0, (float)fire_trail_target_texture.texture.width, (float)-fire_trail_target_texture.texture.height },
                //     (Vector2){ -256, -256 },
                //     WHITE
                // );
                DrawTexturePro(
                    fire_trail_target_texture.texture,
                    (Rectangle) { 0, 0, fire_trail_target_texture.texture.width, fire_trail_target_texture.texture.height },
                    (Rectangle) {-500, -250, 1000, 500},
                    (Vector2) {fire_trail_target_texture.texture.width/2.0, fire_trail_target_texture.texture.height/2.0},
                    90,
                    WHITE
                );

                DrawEllipse(
                    fire_trail.base.x - fire_trail.girth/16.0, fire_trail.base.y,
                    fire_trail.girth/8.0, fire_trail.girth/2.0,
                    (Color) { 255, 36, 0, 255 }
                ); // FIREBALL

            EndMode2D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Speed: %d", (int)player.speed), 10, 10 + 1*50, 20, MAGENTA);
            DrawText(TextFormat("Acc  : %d", (int)player.accelaration), 10, 10 + 2*50, 20, MAGENTA);
            DrawText(TextFormat("Drag : %0.2f", player.__drag), 10, 10 + 3*50, 20, MAGENTA);

        EndDrawing();
    }

    return 0;
}