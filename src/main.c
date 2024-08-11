#include <stdlib.h>
#include <stdio.h>

#define GLSL_VERSION 450

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "asset.h"
#include "animation.h"

#define LAZY_INIT 0

struct {
    unsigned int SCREEN_WIDTH;
    unsigned int SCREEN_HEIGHT;
    TextureAssets TEXTURE_ASSETS;
} GLOBAL;
// = {
//     LAZY_INIT
//     // .SCREEN_WIDTH = LAZY_INIT,
//     // .SCREEN_HEIGHT = LAZY_INIT,
//     // .TEXTURE_ASSETS = {LAZY_INIT},
// };

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

    // arrow_draw(
    //     (Arrow) {
    //         .head_radius = 30,
    //         .base = player->position,
    //         .direction = player->aim_direction,
    //         .length = player_radius + 10*10,
    //     },
    //     5,
    //     GREEN
    // );
}

typedef struct {
    Shader shader;
    float time;
    float noise_speed;
    float wave_speed;
    Texture noise_texture;
    Texture wave_texture;
} FireTrailMaterial;

void fire_trail_material_set_shader_values(FireTrailMaterial* ftm) {
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "time"),
        &ftm->time,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "noiseSpeed"),
        &ftm->noise_speed,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "waveSpeed"),
        &ftm->wave_speed,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValueTexture(
        ftm->shader,
        GetShaderLocation(ftm->shader, "noiseTexture"),
        ftm->noise_texture
    );
}

void fire_trail_material_draw(FireTrailMaterial* ftm, RenderTexture2D target) {
    BeginShaderMode(ftm->shader);
        fire_trail_material_set_shader_values(ftm);
        DrawTexturePro(
            ftm->wave_texture,
            (Rectangle) { 0, 0, ftm->wave_texture.width, ftm->wave_texture.height },
            (Rectangle) {0, 0, target.texture.width, target.texture.height},
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

void fire_trail_update(FireTrail* fire_trail, float delta_time) {
    if (!fire_trail->paused) {
        fire_trail->material.time += delta_time;
    }
}

void fire_trail_update_player_relative(FireTrail* fire_trail, Camera2D camera, Player* player) {
    fire_trail->base = Vector2Add(
        player->position,
        Vector2Scale(player->aim_direction, 128.0 - 10.0 /* hearth texture length */)
    );

    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    fire_trail->direction = Vector2Normalize(Vector2Subtract(mouse, fire_trail->base));
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
    // printf("Angle: %d\n", (int)(RAD2DEG * Vector2Angle((Vector2) {1, 0}, fire_trail->direction)));
}

typedef struct {
    FireTrail fire_trail;
} PlayerHeart;

int main() {

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");

    {
        global_set_screen_size(1920, 1080);
        GLOBAL.TEXTURE_ASSETS = new_texture_assets();
    }

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

    TextureHandle circle_vfx_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    Image circle_vfx_ss = LoadImage("assets\\circle-vfx.png");
    texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, circle_vfx_ss_texture_handle, circle_vfx_ss);

    int anim_frame_count = 8;
    float* checkpoints = malloc(anim_frame_count * sizeof(float));
    for (int i = 0; i < anim_frame_count; i++) {
        checkpoints[i] = 0.15 * (i+1);
    }
    SequenceTimer stimer = new_sequence_timer(checkpoints, anim_frame_count, Timer_Repeating);
    SpriteSheetAnimation circle_vfx_anim = new_sprite_sheet_animation(
        stimer,
        circle_vfx_ss_texture_handle,
        (Vector2) {64, 64},
        1, anim_frame_count, anim_frame_count
    );

    Texture circle_texture = LoadTexture("assets\\circle.png");
    Texture circle_effect_texture = LoadTexture("assets\\circle-effect.png");
    Texture circle_just_effect_texture = LoadTexture("assets\\circle-just-effect.png");
    SetTextureFilter(circle_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(circle_effect_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(circle_just_effect_texture, TEXTURE_FILTER_POINT);

    Texture heart_texture = LoadTexture("assets\\heart.png");

    Texture simple_noise_texture = LoadTexture("assets\\simple-noise.png");
    Texture fire_trail_wave_texture = LoadTexture("assets\\wave.png");

    // RenderTexture2D fire_trail_target_texture = LoadRenderTexture(512, 512);
    Shader fire_trail_shader = LoadShader(0, "shaders\\fire_trail.fs"); // NOTE: use default vs

    FireTrail fire_trail = {
        .material = (FireTrailMaterial) {
            .shader = fire_trail_shader,
            .time = 0,
            .noise_speed = 1.0,
            .wave_speed = 0.6,
            .noise_texture = simple_noise_texture,
            .wave_texture = fire_trail_wave_texture,
        },
        .fire_trail_target_texture = LoadRenderTexture(512, 512),
        .paused = false,
        .base = {0, 0},
        .direction = {1, 0},
        .length = 200,
        .girth = 50,
    };

    FireTrail fire_trail_big_left = {
        .material = (FireTrailMaterial) {
            .shader = fire_trail_shader,
            .time = 0,
            .noise_speed = 0.8,
            .wave_speed = 0.6,
            .noise_texture = simple_noise_texture,
            .wave_texture = fire_trail_wave_texture,
        },
        .fire_trail_target_texture = LoadRenderTexture(512, 512),
        .paused = false,
        .base = {-500, 500},
        .direction = {0, -1},
        .length = 1000,
        .girth = 500,
    };

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
        fire_trail_process_input(&fire_trail_big_left);

        // UPDATE
        tick_sprite_sheet_animation_timer(&circle_vfx_anim, delta_time);
        player_update(&player, camera, delta_time);
        fire_trail_update(&fire_trail, delta_time);
        fire_trail_update_player_relative(&fire_trail, camera, &player);
        fire_trail_update(&fire_trail_big_left, delta_time);
        camera.target = player.position;

        // DRAW
        //fire_trail_material_set_shader_values(&fire_trail.material);
        BeginTextureMode(fire_trail.fire_trail_target_texture);
            ClearBackground(BLANK);
            fire_trail_material_draw(&fire_trail.material, fire_trail.fire_trail_target_texture);
        EndTextureMode();

        //fire_trail_material_set_shader_values(&fire_trail_big_left.material);
        BeginTextureMode(fire_trail_big_left.fire_trail_target_texture);
            ClearBackground(BLANK);
            fire_trail_material_draw(&fire_trail_big_left.material, fire_trail_big_left.fire_trail_target_texture);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(camera);

                DrawCircle(0, 0, 5, RED); // ORIGIN

                player_draw(&player);

                // DrawTexture(fire_trail_target_texture.texture, -256, -256, WHITE);
                // DrawTextureRec(
                //     fire_trail_target_texture.texture,
                //     (Rectangle){ 0, 0, (float)fire_trail_target_texture.texture.width, (float)-fire_trail_target_texture.texture.height },
                //     (Vector2){ -256, -256 },
                //     WHITE
                // );
                // DrawTexturePro(
                //     fire_trail_target_texture.texture,
                //     (Rectangle) { 0, 0, fire_trail_target_texture.texture.width, fire_trail_target_texture.texture.height },
                //     (Rectangle) {-500, -250, 1000, 500},
                //     (Vector2) {fire_trail_target_texture.texture.width/2.0, fire_trail_target_texture.texture.height/2.0},
                //     90,
                //     WHITE
                // );
                fire_trail_draw(&fire_trail_big_left);

                DrawTexturePro(
                    heart_texture,
                    (Rectangle) { 0, 0, heart_texture.width, heart_texture.height },
                    (Rectangle) {
                        .x = player.position.x,
                        .y = player.position.y, //  - fire_trail->girth/2.0 + fire_trail->girth/2.0,
                        .width = heart_texture.width,
                        .height = heart_texture.height,
                    },
                    (Vector2) { 0, heart_texture.height/2.0 },
                    RAD2DEG * Vector2Angle((Vector2) {1, 0}, player.aim_direction),
                    WHITE
                );

                float ellipse_radius_w = fire_trail.girth/3.0;// /8.0;
                float ellipse_radius_h = fire_trail.girth * 1.5;

                // DrawEllipse(
                //     fire_trail.base.x - fire_trail.girth/16.0, fire_trail.base.y,
                //     ellipse_radius_w, ellipse_radius_h,
                //     (Color) { 255, 36, 0, 255 }
                // ); // FIREBALL

                Texture ellipse_texture = circle_texture;
                DrawTexturePro(
                    ellipse_texture,
                    (Rectangle) { 0, 0, ellipse_texture.width, ellipse_texture.height },
                    (Rectangle) {
                        .x = fire_trail.base.x, // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
                        .y = fire_trail.base.y, // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
                        .width = ellipse_radius_w,
                        .height = ellipse_radius_h
                    },
                    (Vector2) {ellipse_radius_w/2.0, ellipse_radius_h/2.0},
                    RAD2DEG * Vector2Angle((Vector2) {1, 0}, fire_trail.direction),
                    WHITE
                );

                float effect_radius_w = 2.0 * ellipse_radius_w;
                float effect_radius_h = 2.0 * ellipse_radius_h;
                float rotation = RAD2DEG * Vector2Angle((Vector2) {1, 0}, fire_trail.direction);
                // DrawTexturePro(
                //     circle_just_effect_texture,
                //     (Rectangle) { 0, 0, circle_just_effect_texture.width, circle_just_effect_texture.height },
                //     (Rectangle) {
                //         .x = fire_trail.base.x, // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
                //         .y = fire_trail.base.y, // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
                //         .width = effect_radius_w,
                //         .height = effect_radius_h
                //     },
                //     (Vector2) {effect_radius_w/2.0, effect_radius_h/2.0},
                //     rotation,
                //     WHITE
                // );
                // DrawTexturePro(
                //     circle_just_effect_texture,
                //     (Rectangle) { 0, 0, circle_just_effect_texture.width, circle_just_effect_texture.height },
                //     (Rectangle) {
                //         .x = fire_trail.base.x - cosf(rotation) * (5.0 * sinf(time*4.0)), // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
                //         .y = fire_trail.base.y + sinf(rotation) * (5.0 * sinf(time*4.0)), // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
                //         .width = effect_radius_w,
                //         .height = effect_radius_h
                //     },
                //     (Vector2) {effect_radius_w/2.0, effect_radius_h/2.0},
                //     rotation,
                //     WHITE
                // );

                SpriteSheetSprite circle_vfx_frame = sprite_sheet_get_current_sprite(&circle_vfx_anim);
                Texture* circle_vfx_texture = texture_assets_get_texture_unchecked(&GLOBAL.TEXTURE_ASSETS, circle_vfx_frame.texture_handle);
                DrawTexturePro(
                    *circle_vfx_texture,
                    circle_vfx_frame.sprite,
                    (Rectangle) {
                        .x = fire_trail.base.x, // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
                        .y = fire_trail.base.y, // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
                        .width = effect_radius_w,
                        .height = effect_radius_h
                    },
                    (Vector2) {effect_radius_w/2.0, effect_radius_h/2.0},
                    rotation,
                    WHITE
                );

                fire_trail_draw(&fire_trail);

            EndMode2D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Speed: %d", (int)player.speed), 10, 10 + 1*50, 20, MAGENTA);
            DrawText(TextFormat("Acc  : %d", (int)player.accelaration), 10, 10 + 2*50, 20, MAGENTA);
            DrawText(TextFormat("Drag : %0.2f", player.__drag), 10, 10 + 3*50, 20, MAGENTA);

        EndDrawing();
    }

    return 0;
}