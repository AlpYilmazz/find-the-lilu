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

GlobalResources GLOBAL = { LAZY_INIT };

void global_set_screen_size(unsigned int width, unsigned int height) {
    GLOBAL.SCREEN_WIDTH = width;
    GLOBAL.SCREEN_HEIGHT = height;
}

typedef struct {
    CircleCollider collider;
    int health;
} Enemy;

void enemy_draw(Enemy* enemy) {
    Color color = (enemy->health > 0) ? GREEN : RED;
    DrawCircleV(enemy->collider.center, enemy->collider.radius, color);
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

        GLOBAL.BULLET_MANAGER = new_bullet_manager();
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

    Enemy enemy = {
        .collider = {
            .center = {2*64 + 32, 2*64 + 32},
            .radius = 20,
        },
        .health = 150,
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
        player_process_input(&GLOBAL, &player, camera);

        fire_trail_process_input(&fire_trail_big_left);
        // -- INPUT END --

        // -- UPDATE START --
        bullet_manager_update_bullets(&GLOBAL.BULLET_MANAGER, delta_time);

        int enemy_hit_count = bullet_manager_check_bullet_hits(&GLOBAL.BULLET_MANAGER, enemy.collider);
        if (enemy_hit_count > 0) {
            int health = enemy.health;
            int damage = enemy_hit_count * player.bullet_damage;
            if (enemy.health > 0) {
                enemy.health -= damage;
            }
            printf("Enemy hit: count: %d, damage: %d, health: %d -> %d\n",
                        enemy_hit_count, damage, health, enemy.health);
        }

        player_update(&player, delta_time);
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
                enemy_draw(&enemy);

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