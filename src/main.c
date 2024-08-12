#include <stdlib.h>
#include <stdio.h>

#define GLSL_VERSION 450

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "asset.h"
#include "animation.h"
#include "controls.h"

#define LAZY_INIT 0

struct {
    unsigned int SCREEN_WIDTH;
    unsigned int SCREEN_HEIGHT;
    TextureAssets TEXTURE_ASSETS;
    TextureHandle NOISE_TEXTURE_HANDLE;
} GLOBAL = { LAZY_INIT };

void global_set_screen_size(unsigned int width, unsigned int height) {
    GLOBAL.SCREEN_WIDTH = width;
    GLOBAL.SCREEN_HEIGHT = height;
}

// UTIL

float signof_f(float x) {
    return (x == 0.0) ? 0.0
        : (x > 0.0) ? 1.0
        : -1.0;
}

// UTIL END

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
    // Vector2 base;
    // Vector2 direction;
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

// void fire_trail_update_player_relative(FireTrail* fire_trail, Camera2D camera, Player* player) {
//     fire_trail->base = Vector2Add(
//         player->position,
//         Vector2Scale(player->aim_direction, 128.0 /* hearth texture length */ - 10.0)
//     );

//     Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
//     fire_trail->direction = Vector2Normalize(Vector2Subtract(mouse, player->position));
// }

void fire_trail_draw(FireTrail* fire_trail, Vector2 base, Vector2 direction) {
    Texture tex = fire_trail->fire_trail_target_texture.texture;
    DrawTexturePro(
        tex,
        (Rectangle) { 0, 0, -tex.width, tex.height },
        (Rectangle) {
            .x = base.x,
            .y = base.y, //  - fire_trail->girth/2.0 + fire_trail->girth/2.0,
            .width = fire_trail->length,
            .height = fire_trail->girth,
        },
        (Vector2) { 0, fire_trail->girth/2.0 },
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, direction),
        WHITE
    );
}

typedef struct {
    FireTrail fire_trail;
    Texture heart_texture;
    Texture portal_texture;
    SpriteSheetAnimation portal_vfx_anim;
} PlayerSkill_ShowWay;

void player_skill_show_way_process_input(PlayerSkill_ShowWay* skill) {
    fire_trail_process_input(&skill->fire_trail);
}

void player_skill_show_way_update(PlayerSkill_ShowWay* skill, float delta_time) {
    fire_trail_update(&skill->fire_trail, delta_time);
    tick_sprite_sheet_animation_timer(&skill->portal_vfx_anim, delta_time);
}

void player_skill_show_way_predraw(PlayerSkill_ShowWay* skill) {
    BeginTextureMode(skill->fire_trail.fire_trail_target_texture);
        ClearBackground(BLANK);
        fire_trail_material_draw(&skill->fire_trail.material, skill->fire_trail.fire_trail_target_texture);
    EndTextureMode();
}

void player_skill_show_way_draw(PlayerSkill_ShowWay* skill, Vector2 position, Vector2 direction) {            
    // heart
    Texture heart_texture = skill->heart_texture;
    DrawTexturePro(
        heart_texture,
        (Rectangle) { 0, 0, heart_texture.width, heart_texture.height },
        (Rectangle) {
            .x = position.x,
            .y = position.y,
            .width = heart_texture.width,
            .height = heart_texture.height,
        },
        (Vector2) { 0, heart_texture.height/2.0 },
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, direction),
        WHITE
    );

    const float heart_texture_length = heart_texture.width;
    const float heart_texture_end_margin = 10.0;

    FireTrail* fire_trail_ref = &skill->fire_trail;
    Vector2 fire_trail_base = Vector2Add(
        position,
        Vector2Scale(direction, heart_texture_length - heart_texture_end_margin)
    );

    // portal
    float ellipse_radius_w = fire_trail_ref->girth/3.0;// /8.0;
    float ellipse_radius_h = fire_trail_ref->girth * 1.5;

    Texture ellipse_texture = skill->portal_texture;
    DrawTexturePro(
        ellipse_texture,
        (Rectangle) { 0, 0, ellipse_texture.width, ellipse_texture.height },
        (Rectangle) {
            .x = fire_trail_base.x, // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
            .y = fire_trail_base.y, // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
            .width = ellipse_radius_w,
            .height = ellipse_radius_h
        },
        (Vector2) {ellipse_radius_w/2.0, ellipse_radius_h/2.0},
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, direction),
        WHITE
    );

    // vfx
    float effect_radius_w = 2.0 * ellipse_radius_w;
    float effect_radius_h = 2.0 * ellipse_radius_h;
    float rotation = RAD2DEG * Vector2Angle((Vector2) {1, 0}, direction);

    SpriteSheetSprite portal_vfx_frame = sprite_sheet_get_current_sprite(&skill->portal_vfx_anim);
    Texture* portal_vfx_texture = texture_assets_get_texture_unchecked(&GLOBAL.TEXTURE_ASSETS, portal_vfx_frame.texture_handle);
    DrawTexturePro(
        *portal_vfx_texture,
        portal_vfx_frame.sprite,
        (Rectangle) {
            .x = fire_trail_base.x, // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
            .y = fire_trail_base.y, // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
            .width = effect_radius_w,
            .height = effect_radius_h
        },
        (Vector2) {effect_radius_w/2.0, effect_radius_h/2.0},
        rotation,
        WHITE
    );

    // fire trail
    fire_trail_draw(fire_trail_ref, fire_trail_base, direction);
}

typedef struct {
    Vector2 position;
    Vector2 direction;
    float speed;
    float accelaration;
    float __drag;
    //
    int rotation_direction;
    float rotation_speed_deg;
    //
    Vector2 aim_direction;
    //
    bool reverse_active;
    //
    PlayerSkill_ShowWay player_skill_show_way;
} Player;

Player player_create_default() {
    return (Player) {
        .position = 0,
        .direction = {0, -1},
        .speed = 100,
        .accelaration = 0,
        .__drag = 0,
        .rotation_direction = 0,
        .rotation_speed_deg = 100,
        .aim_direction = {1, 0},
        .reverse_active = false,
        .player_skill_show_way = {0},
    };
}

void player_process_input(Player* player) {
    player->reverse_active = IsKeyDown(BINDING_KEY_PLAYER_REVERSE);

    if (IsKeyDown(BINDING_KEY_PLAYER_UP) && IsKeyDown(BINDING_KEY_PLAYER_DOWN)) {
        player->accelaration = 0;
    }
    else if (IsKeyDown(BINDING_KEY_PLAYER_UP)) {
        player->accelaration = 1000;
    }
    else if (IsKeyDown(BINDING_KEY_PLAYER_DOWN)) {
        player->accelaration = -500;
    }
    else {
        player->accelaration = 0;
    }

    if (IsKeyDown(BINDING_KEY_PLAYER_RIGHT) && IsKeyDown(BINDING_KEY_PLAYER_LEFT)) {
        player->rotation_direction = 0;
    }
    else if (IsKeyDown(BINDING_KEY_PLAYER_RIGHT)) {
        player->rotation_direction = 1;
    }
    else if (IsKeyDown(BINDING_KEY_PLAYER_LEFT)) {
        player->rotation_direction = -1;
    }
    else {
        player->rotation_direction = 0;
    }

    player_skill_show_way_process_input(&player->player_skill_show_way);
}

void player_update(Player* player, Camera2D camera, float delta_time) {
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

    const float DRAG_COEFF = 1.0 / 800;
    const float CUT_SECTION = 1;
    float drag = DRAG_COEFF * CUT_SECTION * ((player->speed * player->speed)/2);
    drag *= -signof_f(player->speed);
    player->__drag = drag;

    player->speed += (player->accelaration + drag) * delta_time;

    const float SPEED_HARD_LIMIT = 10000.0;
    const float MAX_REVERSE_SPEED = 100.0;
    player->speed = Clamp(
        player->speed,
        (player->reverse_active) ? -MAX_REVERSE_SPEED : 0.0, // TODO: smooth reverse to sudden stop
        SPEED_HARD_LIMIT
    );

    player->direction = Vector2Rotate(
        player->direction,
        player->rotation_direction * DEG2RAD * player->rotation_speed_deg * delta_time
    );

    player->position = Vector2Add(
        player->position,
        Vector2Scale(player->direction, player->speed * delta_time)
    );

    player->aim_direction = Vector2Normalize(Vector2Subtract(mouse, player->position));
    
    player_skill_show_way_update(&player->player_skill_show_way, delta_time);
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

    Vector2 skill_direction = Vector2Normalize(Vector2Subtract(Vector2Zero(), player->position));
    player_skill_show_way_draw(&player->player_skill_show_way, player->position, skill_direction);

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

int main() {

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    // Setup Global Variables
    {
        global_set_screen_size(1920, 1080);
        GLOBAL.TEXTURE_ASSETS = new_texture_assets();

        GLOBAL.NOISE_TEXTURE_HANDLE = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        Image noise_texture_image = LoadImage("assets\\simple-noise.png");
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, GLOBAL.NOISE_TEXTURE_HANDLE, noise_texture_image);
    }

    // Setup Monitor
    {
        int monitor_id = GetMonitorCount() - 1;

        SetWindowMonitor(monitor_id);
        SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_BORDERLESS_WINDOWED_MODE);
    }

    Camera2D camera = {0};
    camera.zoom = 1;
    camera.offset = (Vector2) {
        .x = GLOBAL.SCREEN_WIDTH/2.0,
        .y = GLOBAL.SCREEN_HEIGHT/2.0,
    };

    TextureHandle circle_vfx_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    Image circle_vfx_ss = LoadImage("assets\\circle-vfx.png");
    texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, circle_vfx_ss_texture_handle, circle_vfx_ss);

    SpriteSheetAnimation circle_vfx_anim = {0};
    {
        int anim_frame_count = 8;
        float* checkpoints = malloc(anim_frame_count * sizeof(float));
        for (int i = 0; i < anim_frame_count; i++) {
            checkpoints[i] = 0.15 * (i+1);
        }
        SequenceTimer stimer = new_sequence_timer(checkpoints, anim_frame_count, Timer_Repeating);
        circle_vfx_anim = new_sprite_sheet_animation(
            stimer,
            circle_vfx_ss_texture_handle,
            (Vector2) {64, 64},
            1, anim_frame_count, anim_frame_count
        );
    }

    Texture circle_texture = LoadTexture("assets\\circle.png");
    Texture circle_effect_texture = LoadTexture("assets\\circle-effect.png");
    Texture circle_just_effect_texture = LoadTexture("assets\\circle-just-effect.png");
    SetTextureFilter(circle_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(circle_effect_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(circle_just_effect_texture, TEXTURE_FILTER_POINT);

    Texture heart_texture = LoadTexture("assets\\heart.png");

    Texture simple_noise_texture = LoadTexture("assets\\simple-noise.png");
    Texture fire_trail_wave_texture = LoadTexture("assets\\wave.png");

    Shader fire_trail_shader = LoadShader(0, "shaders\\fire_trail.fs"); // use default vs

    Player player = player_create_default();
    player.player_skill_show_way = (PlayerSkill_ShowWay) {
        .fire_trail = {
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
            .length = 300,
            .girth = 100, // 50,
        },
        .heart_texture = heart_texture,
        .portal_texture = circle_texture,
        .portal_vfx_anim = circle_vfx_anim,
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
        // .base = {-500, 500},
        // .direction = {0, -1},
        .length = 1000,
        .girth = 500,
    };
    Vector2 fire_trail_big_left_position = {-500, 500};
    Vector2 fire_trail_big_left_direction = {0, -1};

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

        // -- INPUT START --
        player_process_input(&player);

        fire_trail_process_input(&fire_trail_big_left);
        // -- INPUT END --

        // -- UPDATE START --
        player_update(&player, camera, delta_time);
        fire_trail_update(&fire_trail_big_left, delta_time);
        camera.target = player.position;
        // -- UPDATE END --

        // ==== DRAW ====

        // -- PREDRAW START --
        player_skill_show_way_predraw(&player.player_skill_show_way);

        BeginTextureMode(fire_trail_big_left.fire_trail_target_texture);
            ClearBackground(BLANK);
            fire_trail_material_draw(&fire_trail_big_left.material, fire_trail_big_left.fire_trail_target_texture);
        EndTextureMode();
        // -- PREDRAW END --

        // -- DRAW START --
        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(camera);

                // Origin
                DrawCircle(0, 0, 5, RED);

                // Player
                player_draw(&player);

                // Fire Trail on the left as big
                fire_trail_draw(&fire_trail_big_left, fire_trail_big_left_position, fire_trail_big_left_direction);

            EndMode2D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Speed: %d", (int)player.speed), 10, 10 + 1*50, 20, MAGENTA);
            DrawText(TextFormat("Acc  : %d", (int)player.accelaration), 10, 10 + 2*50, 20, MAGENTA);
            DrawText(TextFormat("Drag : %0.2f", player.__drag), 10, 10 + 3*50, 20, MAGENTA);

        EndDrawing();
        // -- DRAW END --
    }

    return 0;
}