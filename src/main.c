#include <stdlib.h>
#include <stdio.h>

#define GLSL_VERSION 450

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "util.h"
#include "global.h"
#include "controls.h"
#include "asset.h"
#include "animation.h"
#include "collider.h"
#include "bullet.h"
#include "firetrail.h"
#include "player.h"
#include "enemy.h"

GlobalResources GLOBAL = { LAZY_INIT };

void global_set_screen_size(unsigned int width, unsigned int height) {
    GLOBAL.SCREEN_WIDTH = width;
    GLOBAL.SCREEN_HEIGHT = height;
}

int rectangle_position_quarter_offset(int a) {
    return (a < 0) ? -1 : 0;
}

typedef struct {
    Texture background_texture;
} Background;

void background_instance_draw(Background* bg, int pos_x, int pos_y) {
    Texture tex = bg->background_texture;
    DrawTexturePro(
        tex,
        (Rectangle) { 0, 0, tex.width, tex.height },
        (Rectangle) {
            .x = pos_x,
            .y = pos_y,
            .width = GLOBAL.SCREEN_WIDTH,
            .height = GLOBAL.SCREEN_HEIGHT,
        },
        (Vector2) {0, 0},
        0,
        PURPLE
    );
}

void background_draw(Background* background, Vector2 player_position) {
    const int DIRECTIONS[9][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0}, {0,  0}, {1,  0},
        {-1,  1}, {0,  1}, {1,  1},
    };

    int grid_x = (int)GLOBAL.SCREEN_WIDTH * ((int)(player_position.x) / (int)GLOBAL.SCREEN_WIDTH);
    int grid_y = (int)GLOBAL.SCREEN_HEIGHT * ((int)(player_position.y) / (int)GLOBAL.SCREEN_HEIGHT);
    Vector2 player_grid = {
        .x = grid_x + (rectangle_position_quarter_offset((int)(player_position.x)) * (int)GLOBAL.SCREEN_WIDTH),
        .y = grid_y + (rectangle_position_quarter_offset((int)(player_position.y)) * (int)GLOBAL.SCREEN_HEIGHT),
    };
    AABBCollider player_camera_collider = {
        .x_left = player_position.x - (float)GLOBAL.SCREEN_WIDTH/2.0,
        .x_right = player_position.x + (float)GLOBAL.SCREEN_WIDTH/2.0,
        .y_top = player_position.y - (float)GLOBAL.SCREEN_HEIGHT/2.0,
        .y_bottom = player_position.y + (float)GLOBAL.SCREEN_HEIGHT/2.0,
    };

    // printf("-----\n");
    // printf("Player pos: %d, %d\n", (int)player_position.x, (int)player_position.y);
    // printf("Player grid: %d, %d ===>\n", grid_x, grid_y);
    // printf("Player grid: %d, %d\n", (int)player_grid.x, (int)player_grid.y);
    // printf("-----\n");
    for (int i = 0; i < 9; i++) {
        int x_offset = DIRECTIONS[i][0];
        int y_offset = DIRECTIONS[i][1];
        Vector2 grid = Vector2Add(player_grid, (Vector2) {x_offset * (int)GLOBAL.SCREEN_WIDTH, y_offset * (int)GLOBAL.SCREEN_HEIGHT});
        AABBCollider grid_collider = {
            .x_left = grid.x,
            .x_right = grid.x + (float)GLOBAL.SCREEN_WIDTH,
            .y_top = grid.y,
            .y_bottom = grid.y + (float)GLOBAL.SCREEN_HEIGHT,
        };

        // printf("check grid: %d, %d\n", (int)grid.x, (int)grid.y);
        if (collide_aabb_aabb(player_camera_collider, grid_collider)) {
            // printf("Collides: %d\n", i);
            background_instance_draw(background, grid.x, grid.y);
        }
    }
}

int main() {

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);

    // Setup Global Variables
    {
        global_set_screen_size(1920, 1080);

        GLOBAL.TEXTURE_ASSETS = new_texture_assets();

        GLOBAL.NOISE_TEXTURE_HANDLE = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        Image noise_texture_image = LoadImage("assets\\simple-noise.png");
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, GLOBAL.NOISE_TEXTURE_HANDLE, noise_texture_image);

        GLOBAL.BULLET_MANAGER = new_bullet_manager();
        // GLOBAL.ENEMY_SPAWNER = new_enemy_spawner();
    }

    // Setup Monitor
    {
        int monitor_id = GetMonitorCount() - 1;

        SetWindowMonitor(monitor_id);
        SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_BORDERLESS_WINDOWED_MODE);
    }

    Camera2D camera = {0};
    camera.zoom = 1.5;
    camera.offset = (Vector2) {
        .x = GLOBAL.SCREEN_WIDTH/2.0,
        .y = GLOBAL.SCREEN_HEIGHT/2.0,
    };

    TextureHandle circle_vfx_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    Image circle_vfx_ss = LoadImage("assets\\circle-vfx.png");
    texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, circle_vfx_ss_texture_handle, circle_vfx_ss);

    float circle_vfx_anim_frame_count = 8;
    SpriteSheetAnimation circle_vfx_anim = new_sprite_sheet_animation(
        new_sequence_timer_evenly_spaced(0.15, circle_vfx_anim_frame_count, Timer_Repeating),
        circle_vfx_ss_texture_handle,
        (Vector2) {64, 64},
        1, circle_vfx_anim_frame_count, circle_vfx_anim_frame_count
    );

    Texture circle_texture = LoadTexture("assets\\circle.png");
    Texture circle_effect_texture = LoadTexture("assets\\circle-effect.png");
    Texture circle_just_effect_texture = LoadTexture("assets\\circle-just-effect.png");
    SetTextureFilter(circle_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(circle_effect_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(circle_just_effect_texture, TEXTURE_FILTER_POINT);

    Texture simple_noise_texture = LoadTexture("assets\\simple-noise.png");
    Texture fire_trail_wave_texture = LoadTexture("assets\\wave.png");

    Shader fire_trail_shader = LoadShader(0, "shaders\\fire_trail.fs"); // use default vs

    Background bg = {
        .background_texture = LoadTexture("assets\\background.png"),
    };
    SetTextureFilter(bg.background_texture, TEXTURE_FILTER_TRILINEAR);

    EnemySpawner enemy_spawner = new_enemy_spawner();
    {
        Image idle_image = LoadImage("assets\\enemy\\enemy-idle.png");
        TextureHandle idle_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, idle_texture_handle, idle_image);

        Image move_image = LoadImage("assets\\enemy\\enemy-move.png");
        TextureHandle move_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, move_texture_handle, move_image);

        enemy_spawner.enemy_idle_anim_sprite_sheet_texture_handle = idle_texture_handle;
        enemy_spawner.enemy_move_anim_sprite_sheet_texture_handle = move_texture_handle;
    }

    TextureHandle heart_beat_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    Image heart_beat_ss = LoadImage("assets\\heart-beat.png");
    texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, heart_beat_ss_texture_handle, heart_beat_ss);

    float heart_beat_anim_frame_count = 8;
    SpriteSheetAnimation heart_beat_anim = new_sprite_sheet_animation(
        new_sequence_timer_evenly_spaced(0.1, heart_beat_anim_frame_count, Timer_Repeating),
        heart_beat_ss_texture_handle,
        (Vector2) {128, 64},
        1, heart_beat_anim_frame_count, heart_beat_anim_frame_count
    );

    Player player = player_create_default();
    player.player_skill_show_way = (PlayerSkill_ShowWay) {
        .fire_trail = {
            .material = (FireTrailMaterial) {
                .shader = fire_trail_shader,
                .time = 0,
                .noise_speed = 0.7,
                .wave_speed = 0.7,
                .noise_texture = simple_noise_texture,
                .wave_texture = fire_trail_wave_texture,
            },
            .fire_trail_target_texture = LoadRenderTexture(512, 512),
            .paused = false,
            .length = 300,
            .girth = 100, // 50,
        },
        .heart_beat_anim = heart_beat_anim,
        .portal_texture = circle_texture,
        .portal_vfx_anim = circle_vfx_anim,
        .active = false,
    };
    {
        Texture motorcycle_texture = LoadTexture("assets\\player\\motorcycle.png");
        Texture motorcycle_with_gun_texture = LoadTexture("assets\\player\\motorcycle-with-gun.png");
        Texture lower_body_texture = LoadTexture("assets\\player\\lower-body.png");
        Texture left_arm_with_gun_texture = LoadTexture("assets\\player\\left-arm-with-gun.png");
        Texture upper_body_texture = LoadTexture("assets\\player\\upper-body.png");
        Texture upper_body_full_texture = LoadTexture("assets\\player\\upper-body-full.png");
        Texture hair_idle_texture = LoadTexture("assets\\player\\hair-idle.png");
        Image hair_move_image = LoadImage("assets\\player\\hair-move.png");

        TextureHandle hair_move_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, hair_move_texture_handle, hair_move_image);

        float hair_move_anim_frame_count = 5;
        SpriteSheetAnimation hair_move_anim = new_sprite_sheet_animation(
            new_sequence_timer_evenly_spaced(0.1, hair_move_anim_frame_count, Timer_Repeating),
            hair_move_texture_handle,
            (Vector2) {64, 128},
            1, hair_move_anim_frame_count, hair_move_anim_frame_count
        );

        player.graphics = (PlayerGraphics) {
            .motorcycle_texture = motorcycle_texture,
            .motorcycle_with_gun_texture = motorcycle_with_gun_texture,
            .lower_body_texture = lower_body_texture,
            .left_arm_with_gun_texture = left_arm_with_gun_texture,
            .upper_body_texture = upper_body_texture,
            .upper_body_full_texture = upper_body_full_texture,
            .hair_idle_texture = hair_idle_texture,
            .hair_move_anim = hair_move_anim,
        };
    }

    // Vector2 enemy_pos = {2*64 + 32, 2*64 + 32};
    // Enemy enemy = enemy_create_default();
    // {
    //     Image idle_image = LoadImage("assets\\enemy\\enemy-idle.png");
    //     TextureHandle idle_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    //     texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, idle_texture_handle, idle_image);

    //     Image move_image = LoadImage("assets\\enemy\\enemy-move.png");
    //     TextureHandle move_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    //     texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, move_texture_handle, move_image);

    //     float idle_anim_frame_count = 7;
    //     SpriteSheetAnimation idle_anim = new_sprite_sheet_animation(
    //         new_sequence_timer_evenly_spaced(0.1, idle_anim_frame_count, Timer_Repeating),
    //         idle_texture_handle,
    //         (Vector2) {128, 64},
    //         1, idle_anim_frame_count, idle_anim_frame_count
    //     );

    //     float move_anim_frame_count = 4;
    //     SpriteSheetAnimation move_anim = new_sprite_sheet_animation(
    //         new_sequence_timer_evenly_spaced(0.1, move_anim_frame_count, Timer_Repeating),
    //         move_texture_handle,
    //         (Vector2) {128, 64},
    //         1, move_anim_frame_count, move_anim_frame_count
    //     );

    //     enemy.graphics = (EnemyGraphics) {
    //         .idle_anim = idle_anim,
    //         .move_anim = move_anim,
    //     };
    // }

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
        player_process_input(&player, camera);

        fire_trail_process_input(&fire_trail_big_left);
        // -- INPUT END --

        // -- UPDATE START --
        bullet_manager_update_bullets(&GLOBAL.BULLET_MANAGER, delta_time);
        enemy_spawner_update_enemies(&GLOBAL, &enemy_spawner, &player, delta_time);
        if (enemy_spawner.count == 0) {
            enemy_spawner_spawn_at_position(
                &GLOBAL,
                &enemy_spawner,
                Vector2Add(player.position, Vector2Scale(player.direction, 400))
            );
        }

        player_update(&GLOBAL, &player, delta_time);
        // enemy_update(&enemy, &player, delta_time);
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
            ClearBackground(LIGHTGRAY);
            
            BeginMode2D(camera);

                background_draw(&bg, player.position);

                const int step = 64;
                const int count_x = (GLOBAL.SCREEN_WIDTH / step) - 1;
                const int count_y = (GLOBAL.SCREEN_HEIGHT / step) - 1;
                for (int i = 0; i < count_x; i++) {
                    int x = step * (i+1);
                    x -= camera.offset.x;
                    int yStart = -camera.offset.y;
                    DrawLine(x, yStart, x, yStart + 2.0*camera.offset.y, BLACK);
                }
                for (int i = 0; i < count_y; i++) {
                    int y = step * (i+1);
                    y -= camera.offset.y;
                    int xStart = -camera.offset.x;
                    DrawLine(xStart, y, xStart + 2.0*camera.offset.x, y, BLACK);
                }

                // Origin
                DrawCircle(0, 0, 5, RED);

                // Player
                player_draw(&GLOBAL, &player);

                // Enemy
                // enemy_draw(&GLOBAL, &enemy);
                enemy_spawner_draw_enemies(&GLOBAL, &enemy_spawner);

                // Bullets
                bullet_manager_draw_bullets(&GLOBAL.BULLET_MANAGER);

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