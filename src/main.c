#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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

// LILU

bool LILU_SPAWNED = false;
Vector2 LILU_SIZE = { LAZY_INIT };
SpriteSheetAnimation LILU_SPAWN_ANIM = { LAZY_INIT };
SpriteSheetAnimation LILU_ANIM = { LAZY_INIT };

void init_goddess_lilu() {
    // TODO
    LILU_SIZE = (Vector2) { UNINIT, UNINIT };

    {
        TextureHandle lilu_spawn_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        Image lilu_spawn_anim_image = LoadImage("assets\\lilu-spawn.png");
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, lilu_spawn_ss_texture_handle, lilu_spawn_anim_image);
        
        // TODO
        LILU_SPAWN_ANIM = new_sprite_sheet_animation_single_row_even_timer(
            lilu_spawn_ss_texture_handle,
            (Vector2) { UNINIT, UNINIT },
            UNINIT,
            UNINIT,
            Timer_Repeating
        );
    }
    {
        TextureHandle lilu_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        Image lilu_anim_image = LoadImage("assets\\lilu.png");
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, lilu_ss_texture_handle, lilu_anim_image);
        
        // TODO
        LILU_ANIM = new_sprite_sheet_animation_single_row_even_timer(
            lilu_ss_texture_handle,
            (Vector2) { UNINIT, UNINIT },
            UNINIT,
            UNINIT,
            Timer_Repeating
        );
    }
}

void goddess_lilu_update(float delta_time) {
    if (LILU_SPAWNED) {
        tick_sprite_sheet_animation_timer(&LILU_ANIM, delta_time);
    }
    else {
        tick_sprite_sheet_animation_timer(&LILU_SPAWN_ANIM, delta_time);
        if (sequence_timer_is_finished(&LILU_SPAWN_ANIM.timer)) {
            LILU_SPAWNED = true;
        }
    }
}

void goddess_lilu_draw() {
    SpriteSheetSprite lilu;
    if (LILU_SPAWNED) {
        lilu = sprite_sheet_get_current_sprite(&LILU_ANIM);
    }
    else {
        lilu = sprite_sheet_get_current_sprite(&LILU_SPAWN_ANIM);
    }

    Texture* lilu_texture = texture_assets_get_texture_unchecked(&GLOBAL.TEXTURE_ASSETS, lilu.texture_handle);

    DrawTexturePro(
        *lilu_texture,
        lilu.sprite,
        (Rectangle) {
            .x = GLOBAL.LILU_POSITION.x - LILU_SIZE.x/2.0,
            .y = GLOBAL.LILU_POSITION.y - LILU_SIZE.y/2.0,
            .width = LILU_SIZE.x,
            .height = LILU_SIZE.y,
        },
        (Vector2) { 0, 0 },
        0,
        WHITE
    );
}

int main() {

    SetRandomSeed(time(NULL));

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitAudioDevice();

    // Setup Global Variables
    {
        global_set_screen_size(1920, 1080);
        
        GLOBAL.LILU_POSITION = (Vector2) {
            ((GetRandomValue(0, 1) == 0) ? -1 : 1) * ((float)GLOBAL.SCREEN_WIDTH), // * 100.0,
            ((GetRandomValue(0, 1) == 0) ? -1 : 1) * ((float)GLOBAL.SCREEN_HEIGHT), //  * 100.0,
        };

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

    init_goddess_lilu();

    Camera2D camera = {0};
    camera.zoom = 1.5;
    camera.offset = (Vector2) {
        .x = GLOBAL.SCREEN_WIDTH/2.0,
        .y = GLOBAL.SCREEN_HEIGHT/2.0,
    };

    Texture portal_texture = LoadTexture("assets\\circle.png");
    SetTextureFilter(portal_texture, TEXTURE_FILTER_POINT);

    TextureHandle circle_vfx_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    Image circle_vfx_ss = LoadImage("assets\\circle-vfx.png");
    texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, circle_vfx_ss_texture_handle, circle_vfx_ss);

    Texture simple_noise_texture = LoadTexture("assets\\simple-noise.png");
    Texture fire_trail_wave_texture = LoadTexture("assets\\wave.png");

    Shader fire_trail_shader = LoadShader(0, "shaders\\fire_trail.fs"); // use default vs

    // >> Heart Beat
    TextureHandle heart_beat_ss_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
    Image heart_beat_ss = LoadImage("assets\\heart-beat.png");
    texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, heart_beat_ss_texture_handle, heart_beat_ss);
    // << Heart Beat

    Background bg = {
        .background_texture = LoadTexture("assets\\background.png"),
    };
    SetTextureFilter(bg.background_texture, TEXTURE_FILTER_POINT);

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
        .heart_beat_anim = new_sprite_sheet_animation_single_row_even_timer(
            heart_beat_ss_texture_handle,
            (Vector2) {128, 64},
            8,      // frame count
            0.1,    // time between frames
            Timer_Repeating
        ),
        .portal_texture = portal_texture,
        .portal_vfx_anim = new_sprite_sheet_animation_single_row_even_timer(
            circle_vfx_ss_texture_handle,
            (Vector2) {64, 64},
            8,      // frame count
            0.15,    // time between frames
            Timer_Repeating
        ),
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

        SpriteSheetAnimation hair_move_anim = new_sprite_sheet_animation_single_row_even_timer(
            hair_move_texture_handle,
            (Vector2) {64, 128},
            5,      // frame count
            0.1,    // time between frames
            Timer_Repeating
        );

        Image shotgun_blast_image = LoadImage("assets\\shotgun-blast.png");
        TextureHandle shotgun_blast_texture_handle = texture_assets_reserve_texture_slot(&GLOBAL.TEXTURE_ASSETS);
        texture_assets_put_image_and_create_texture(&GLOBAL.TEXTURE_ASSETS, shotgun_blast_texture_handle, shotgun_blast_image);

        SpriteSheetAnimation shotgun_blast_anim = new_sprite_sheet_animation_single_row_even_timer(
            shotgun_blast_texture_handle,
            (Vector2) {128, 128},
            12,      // frame count
            0.1,     // time between frames
            Timer_NonRepeating
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
            //
            .shotgun_blast_anim = shotgun_blast_anim,
        };
    }
    {
        player.audio = (PlayerAudio) {
            .motorcycle_idle_audio = LoadMusicStream("assets\\audio\\motorcycle-engine-idle.mp3"),
            .motorcycle_move_audio = LoadMusicStream("assets\\audio\\motorcycle-engine-move.mp3"),
            .shutgun_blast_audio = LoadMusicStream("assets\\audio\\shotgun-blast.wav"),
            .shutgun_pump_audio = LoadMusicStream("assets\\audio\\shotgun-pump.wav"),
        };

        SetMusicVolume(player.audio.motorcycle_idle_audio, 1.0);
        SetMusicVolume(player.audio.motorcycle_move_audio, 1.0);
        SetMusicVolume(player.audio.shutgun_blast_audio, 0.5);
        SetMusicVolume(player.audio.shutgun_pump_audio, 1.0);

        player.audio.shutgun_blast_audio.looping = false;
        player.audio.shutgun_pump_audio.looping = false;
    }

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
        .length = 1080 - 100 - 10,
        .girth = 500,
    };
    Vector2 fire_trail_big_left_position = {500, GLOBAL.SCREEN_HEIGHT};
    Vector2 fire_trail_big_left_direction = {0, -1};

    Timer enemy_spawn_timer = new_timer(5, Timer_Repeating);
    enemy_spawner_spawn_at_position(
        &enemy_spawner,
        Vector2Add(
            player.position,
            Vector2Scale(Vector2Rotate(player.direction, DEG2RAD * 90), 600)
        )
    );
    enemy_spawner_spawn_at_position(
        &enemy_spawner,
        Vector2Add(
            player.position,
            Vector2Scale(Vector2Rotate(player.direction, DEG2RAD * 270), 600)
        )
    );
    enemy_spawner.enemies[0].direction = (Vector2) { -1, 0 };

    bool paused = true;
    bool lilu_found = false;

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

        if (player.health <= 0) {
            paused = true;
        }
        if (Vector2Distance(player.position, GLOBAL.LILU_POSITION) <= 100) {
            paused = true;
            lilu_found = true;
            camera.target = GLOBAL.LILU_POSITION;
        }

        PAUSE_CHECK:
        if (paused) {
            if (!lilu_found && IsKeyPressed(KEY_ENTER)) {
                paused = false;
                camera.zoom = 1.5;
                for (int i = 0; i < enemy_spawner.count; i++) {
                    enemy_spawner.enemies[i].moving = true;
                }
                goto UNPAUSE;
            }
            camera.zoom = 1.0;

            // fire_trail_process_input(&fire_trail_big_left);

            if (lilu_found) {
                goddess_lilu_update(delta_time);
            }

            fire_trail_update(&fire_trail_big_left, delta_time);
            fire_trail_big_left_position = Vector2Add(
                camera.target,
                (Vector2) {-((int)GLOBAL.SCREEN_WIDTH/4), -100 + (int)GLOBAL.SCREEN_HEIGHT/2}
            );

            BeginTextureMode(fire_trail_big_left.fire_trail_target_texture);
                ClearBackground(BLANK);
                fire_trail_material_draw(&fire_trail_big_left.material, fire_trail_big_left.fire_trail_target_texture);
            EndTextureMode();

            BeginDrawing();
                ClearBackground(LIGHTGRAY);
                BeginMode2D(camera);
                    background_draw(&bg, player.position);

                    // Enemy
                    // enemy_draw(&GLOBAL, &enemy);
                    enemy_spawner_draw_enemies(&GLOBAL, &enemy_spawner);

                    // Player
                    player_draw(&GLOBAL, &player);

                    // Bullets
                    bullet_manager_draw_bullets(&GLOBAL.BULLET_MANAGER);

                    // LILU
                    if (lilu_found) {
                        goddess_lilu_draw();
                    }

                    // portal
                    float ellipse_radius_w = fire_trail_big_left.girth/3.0;// /8.0;
                    float ellipse_radius_h = fire_trail_big_left.girth * 1.2;

                    Texture ellipse_texture = portal_texture;
                    DrawTexturePro(
                        ellipse_texture,
                        (Rectangle) { 0, 0, ellipse_texture.width, ellipse_texture.height },
                        (Rectangle) {
                            .x = fire_trail_big_left_position.x, // - ellipse_radius_w/2.0 + ellipse_radius_w/2.0,
                            .y = fire_trail_big_left_position.y, // - ellipse_radius_h/2.0 + ellipse_radius_h/2.0,
                            .width = ellipse_radius_w,
                            .height = ellipse_radius_h
                        },
                        (Vector2) {ellipse_radius_w/2.0, ellipse_radius_h/2.0},
                        RAD2DEG * Vector2Angle((Vector2) {1, 0}, fire_trail_big_left_direction),
                        WHITE
                    );
                    fire_trail_draw(&fire_trail_big_left, fire_trail_big_left_position, fire_trail_big_left_direction);
                EndMode2D();

                    #define TITLE_PARTS 8
                    const char* title[TITLE_PARTS] = {
                        "ADAMIN",
                        "Kalbindeki yanan tutkuyla",
                        "Etrafindaki kimseyi gormeden",
                        "Ve fark etmeden",
                        "Dunyanin neresinde",
                        "Olursa olsun",
                        "LILUSUNU",
                        "Bulma oyunu",
                    };
                    Font font = GetFontDefault();
                    for (int i = 0; i < TITLE_PARTS; i++) {
                        const char* text = title[i];
                        float font_size = 80;
                        float spacing = 5;
                        Vector2 size = MeasureTextEx(font, text, font_size, spacing);
                        DrawTextEx(
                            font,
                            text,
                            (Vector2) {
                                -200 + GLOBAL.SCREEN_WIDTH/2,
                                100 + i*(size.y + size.y/4.0),
                            },
                            font_size,
                            spacing,
                            (i == 0) ? RED
                                : (i == TITLE_PARTS-1 - 1) ? YELLOW
                                : RAYWHITE
                        );
                    }
            EndDrawing();

            continue;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            paused = true;
            for (int i = 0; i < enemy_spawner.count; i++) {
                enemy_spawner.enemies[i].moving = false;
            }
            BeginDrawing();
            EndDrawing();
            goto PAUSE_CHECK;
        }
        UNPAUSE:

        // -- INPUT START --
        player_process_input(&player, camera);

        // fire_trail_process_input(&fire_trail_big_left);
        // -- INPUT END --

        // -- UPDATE START --
        tick_timer(&enemy_spawn_timer, delta_time);
        if (timer_is_finished(&enemy_spawn_timer)) {
            enemy_spawner_spawn_at_position(
                &enemy_spawner,
                Vector2Add(player.position, Vector2Scale(player.direction, 400))
            );
        }

        bullet_manager_update_bullets(&GLOBAL.BULLET_MANAGER, delta_time);
        enemy_spawner_update_enemies(&GLOBAL, &enemy_spawner, &player, delta_time);
        if (enemy_spawner.count == 0) {
            for (int angle = 0; angle < 360; angle += 45) {
                enemy_spawner_spawn_at_position(
                    &enemy_spawner,
                    Vector2Add(
                        player.position,
                        Vector2Scale(Vector2Rotate(player.direction, DEG2RAD * angle), 400)
                    )
                );
            }
        }

        player_update(&GLOBAL, &player, delta_time);
        // enemy_update(&enemy, &player, delta_time);
        // fire_trail_update(&fire_trail_big_left, delta_time);
        camera.target = player.position;
        // -- UPDATE END --

        // ==== DRAW ====

        // -- PREDRAW START --
        player_skill_show_way_predraw(&player.player_skill_show_way);

        // BeginTextureMode(fire_trail_big_left.fire_trail_target_texture);
        //     ClearBackground(BLANK);
        //     fire_trail_material_draw(&fire_trail_big_left.material, fire_trail_big_left.fire_trail_target_texture);
        // EndTextureMode();
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
                // DrawCircle(0, 0, 5, RED);

                // Enemy
                // enemy_draw(&GLOBAL, &enemy);
                enemy_spawner_draw_enemies(&GLOBAL, &enemy_spawner);

                // Player
                player_draw(&GLOBAL, &player);

                // Bullets
                bullet_manager_draw_bullets(&GLOBAL.BULLET_MANAGER);

                // Fire Trail on the left as big
                // fire_trail_draw(&fire_trail_big_left, fire_trail_big_left_position, fire_trail_big_left_direction);

            EndMode2D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Speed: %d", (int)player.speed), 10, 10 + 3*50, 20, MAGENTA);
            DrawText(TextFormat("Acc  : %d", (int)player.accelaration), 10, 10 + 4*50, 20, MAGENTA);
            DrawText(TextFormat("Drag : %0.2f", player.__drag), 10, 10 + 5*50, 20, MAGENTA);

            Rectangle healthbar_full = {
                .x = 10,
                .y = 10 + 1*50,
                .width = 200,
                .height = 50,
            };
            float h = (float)player.health / 100.0;
            Rectangle healthbar = healthbar_full;
            healthbar.width = healthbar_full.width * h;
            DrawRectangleRec(
                healthbar, (h < 0.5) ? RED : GREEN
            );
            DrawRectangleLinesEx(
                healthbar_full, 5, BLACK
            );

        EndDrawing();
        // -- DRAW END --
    }

    return 0;
}