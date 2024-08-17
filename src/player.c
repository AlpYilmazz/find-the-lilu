
#include "raylib.h"
#include "raymath.h"

#include "global.h"
#include "util.h"
#include "controls.h"
#include "shapes.h"
#include "asset.h"
#include "animation.h"
#include "bullet.h"
#include "firetrail.h"

#include "player.h"

// PlayerSkill_ShowWay

void player_skill_show_way_process_input(PlayerSkill_ShowWay* skill, bool skill_can_activate) {
    fire_trail_process_input(&skill->fire_trail);
    skill->active = skill_can_activate && IsKeyDown(BINDING_KEY_PLAYER_SHOW_WAY);
}

void player_skill_show_way_update(PlayerSkill_ShowWay* skill, float delta_time) {
    if (!skill->active) return;

    tick_sprite_sheet_animation_timer(&skill->heart_beat_anim, delta_time);
    fire_trail_update(&skill->fire_trail, delta_time);
    tick_sprite_sheet_animation_timer(&skill->portal_vfx_anim, delta_time);
}

void player_skill_show_way_predraw(PlayerSkill_ShowWay* skill) {
    if (!skill->active) return;
    
    BeginTextureMode(skill->fire_trail.fire_trail_target_texture);
        ClearBackground(BLANK);
        fire_trail_material_draw(&skill->fire_trail.material, skill->fire_trail.fire_trail_target_texture);
    EndTextureMode();
}

void player_skill_show_way_draw(
    GlobalResources* GLOBAL,
    PlayerSkill_ShowWay* skill,
    Vector2 position,
    Vector2 direction
) {  
    if (!skill->active) return;
              
    // heart
    SpriteSheetSprite heart_beat = sprite_sheet_get_current_sprite(&skill->heart_beat_anim);
    Texture* heart_beat_texture = texture_assets_get_texture_unchecked(&GLOBAL->TEXTURE_ASSETS, heart_beat.texture_handle);
    DrawTexturePro(
        *heart_beat_texture,
        heart_beat.sprite,
        (Rectangle) {
            .x = position.x,
            .y = position.y,
            .width = heart_beat.sprite.width,
            .height = heart_beat.sprite.height,
        },
        (Vector2) { 0, heart_beat.sprite.height/2.0 },
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, direction),
        WHITE
    );

    const float heart_texture_length = heart_beat.sprite.width;
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
    Texture* portal_vfx_texture = texture_assets_get_texture_unchecked(&GLOBAL->TEXTURE_ASSETS, portal_vfx_frame.texture_handle);
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

// PlayerGraphics

// Player

Player player_create_default() {
    return (Player) {
        .graphics = { LAZY_INIT },
        .audio = { LAZY_INIT },
        .player_skill_show_way = { LAZY_INIT },
        .heart_origin_relative = {28 - (64/2), 74 - (128/2)}, // @HARDCODED
        .collider = {
            .center = {0, 0},
            .radius = 16,
        },
        .health = 100,
        .position = {0, 0},
        .direction = {0, -1},
        .speed = 0,
        .accelaration = 0,
        .__drag = 0,
        .rotation_direction = 0,
        .rotation_speed_deg = 100,
        .aim_origin_relative = {19 - (64/2), 74 - (128/2)}, // @HARDCODED
        .aim_cursor_point = {0, 0},
        .aim_direction = {1, 0},
        .gun_ready = true,
        .do_shoot_this_frame = false,
        .shoot_cooldown_timer = new_timer(1.2, Timer_NonRepeating),
        .bullet_damage = 50,
        .in_shooting_stance = false,
        .in_shoot_blast = false,
        .reverse_active = false,
    };
}

void player_process_input(
    Player* player,
    Camera2D camera
) {
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    player->aim_cursor_point = mouse;

    player->in_shooting_stance = IsMouseButtonDown(BINDING_MOUSE_BUTTON_PLAYER_SHOOTING_STANCE);
    player->reverse_active = IsKeyDown(BINDING_KEY_PLAYER_REVERSE);

    if (player->in_shooting_stance && IsMouseButtonPressed(BINDING_MOUSE_BUTTON_PLAYER_SHOOT)) {
        player->do_shoot_this_frame = true;
    }

    if (IsKeyDown(BINDING_KEY_PLAYER_UP) && IsKeyDown(BINDING_KEY_PLAYER_DOWN)) {
        player->accelaration = 0;
    }
    else if (IsKeyDown(BINDING_KEY_PLAYER_UP)) {
        player->accelaration = 1000;
    }
    else if (IsKeyDown(BINDING_KEY_PLAYER_DOWN)) {
        player->accelaration = -700;
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

    bool skill_can_activate = (player->speed == 0.0);
    player_skill_show_way_process_input(&player->player_skill_show_way, skill_can_activate);
}

void player_update(GlobalResources* GLOBAL, Player* player, float delta_time) {
    const float DRAG_COEFF = 1.0 / 800;
    const float CUT_SECTION = 1;
    float drag = DRAG_COEFF * CUT_SECTION * ((player->speed * player->speed)/2);
    drag *= -signof_f(player->speed);
    player->__drag = drag;

    float accelaration = player->accelaration + drag;

    const float SHOOT_AND_MOVE_SPEED_HARD_LIMIT = 800.0;
    if (player->in_shooting_stance && player->speed >= SHOOT_AND_MOVE_SPEED_HARD_LIMIT) {
        accelaration = drag;
    }

    float prev_speed = player->speed;
    player->speed += accelaration * delta_time;

    const float SPEED_HARD_LIMIT = 10000.0;
    const float MAX_REVERSE_SPEED = 100.0;
    player->speed = Clamp(
        player->speed,
        (player->reverse_active) ? -MAX_REVERSE_SPEED : 0.0, // TODO: smooth reverse to sudden stop
        SPEED_HARD_LIMIT
    );

    if (abs(player->speed) > EPSILON) {
        player->direction = Vector2Rotate(
            player->direction,
            player->rotation_direction * DEG2RAD * player->rotation_speed_deg * delta_time
        );
    }

    player->position = Vector2Add(
        player->position,
        Vector2Scale(player->direction, player->speed * delta_time)
    );
    player->collider.center = player->position;

    const float player_rotation_rad = PI/2.0 + Vector2Angle((Vector2) {1, 0}, player->direction);
    const Vector2 aim_origin_relative = Vector2Rotate(player->aim_origin_relative, player_rotation_rad);
    const Vector2 aim_origin = Vector2Add(player->position, aim_origin_relative);

    player->aim_direction = Vector2Normalize(Vector2Subtract(player->aim_cursor_point, aim_origin));

    if (prev_speed < EPSILON) {
        if (player->speed >= EPSILON) {
            // started moving
            StopMusicStream(player->audio.motorcycle_idle_audio);
            PlayMusicStream(player->audio.motorcycle_move_audio);
        }
        else { // if (player->speed < EPSILON)
            // still idle
            UpdateMusicStream(player->audio.motorcycle_idle_audio);
        }
    }
    else { // if (prev_speed >= EPSILON)
        if (player->speed >= EPSILON) {
            // still moving
            UpdateMusicStream(player->audio.motorcycle_move_audio);
        }
        else { // if (player->speed < EPSILON)
            // started idle
            StopMusicStream(player->audio.motorcycle_move_audio);
            PlayMusicStream(player->audio.motorcycle_idle_audio);
        }
    }

    if (!player->gun_ready) {
        tick_timer(&player->shoot_cooldown_timer, delta_time);
    }
    if (timer_is_finished(&player->shoot_cooldown_timer)) {
        player->gun_ready = true;
    }

    const bool do_shoot_this_frame = player->do_shoot_this_frame;
    if (player->do_shoot_this_frame) {
        player->do_shoot_this_frame = false;
    }
    
    if (IsMusicStreamPlaying(player->audio.shutgun_blast_audio)) {
        UpdateMusicStream(player->audio.shutgun_blast_audio);
    }
    if (!IsMusicStreamPlaying(player->audio.shutgun_pump_audio)
        && !player->gun_ready
        && player->shoot_cooldown_timer.time_elapsed >= player->shoot_cooldown_timer.time_setup/3.0
    ) {
        PlayMusicStream(player->audio.shutgun_pump_audio);
    }
    if (IsMusicStreamPlaying(player->audio.shutgun_pump_audio)) {
        UpdateMusicStream(player->audio.shutgun_pump_audio);
    }

    if (player->in_shoot_blast) {
        tick_sprite_sheet_animation_timer(&player->graphics.shotgun_blast_anim, delta_time);
    }

    if (player->gun_ready && do_shoot_this_frame) {
        player->gun_ready = false;
        player->in_shoot_blast = true;
        reset_sprite_sheet_animation(&player->graphics.shotgun_blast_anim);
        reset_timer(&player->shoot_cooldown_timer);

        StopMusicStream(player->audio.shutgun_blast_audio);
        PlayMusicStream(player->audio.shutgun_blast_audio);

        int bullet_spawn_offset = player->graphics.left_arm_with_gun_texture.width;
        Vector2 bullet_spawn_pos = Vector2Add(
            aim_origin,
            Vector2Scale(player->aim_direction, (float) bullet_spawn_offset)
        );
        float bullet_radius = 4;
        float bullet_lifetime = 0.5;
        float bullet_speed = 1600.0;
        Vector2 bullet_velocity_player_move_contribution = Vector2Scale(player->direction, player->speed/2.0);
        Vector2 bullet_velocity_gun_contribution = Vector2Scale(player->aim_direction, bullet_speed);

        #define BULLET_COUNT 7
        float bullet_angles_deg[BULLET_COUNT] = {-9, -6, -3, 0, 3, 6, 9};
        for (int i = 0; i < BULLET_COUNT; i++) {
            Vector2 bullet_velocity = Vector2Add(
                bullet_velocity_player_move_contribution,
                Vector2Rotate(bullet_velocity_gun_contribution, DEG2RAD * bullet_angles_deg[i])
            );
            Bullet bullet = new_bullet(
                bullet_spawn_pos,
                bullet_velocity,
                bullet_lifetime,
                bullet_radius
            );
            bullet_manager_spawn_bullet(&GLOBAL->BULLET_MANAGER, bullet);
        }
        #undef BULLET_COUNT

        // printf("Bullet Spawned\n");
        // printf("Player pos: %0.2f %0.2f\n", player->position.x, player->position.y);
        // printf("Bullet pos: %0.2f %0.2f\n", bullet_spawn_pos.x, bullet_spawn_pos.y);
    }

    if (sequence_timer_is_finished(&player->graphics.shotgun_blast_anim.timer)) {
        player->in_shoot_blast = false;
    }
    
    tick_sprite_sheet_animation_timer(&player->graphics.hair_move_anim, delta_time);
    player_skill_show_way_update(&player->player_skill_show_way, delta_time);
}

void player_draw(GlobalResources* GLOBAL, Player* player) {
    float player_radius = 50;

    // arrow_draw(
    //     (Arrow) {
    //         .head_radius = 30,
    //         .base = player->position,
    //         .direction = player->direction,
    //         .length = player_radius + 10*10,
    //     },
    //     5,
    //     RED
    // );
    // DrawCircleV(player->position, player_radius, WHITE);

    // typedef struct {
    //     Texture motorcycle_texture;
    //     Texture motorcycle_with_gun_texture;
    //     Texture lower_body_texture;
    //     Texture left_arm_with_gun_texture;
    //     Texture upper_body_texture;
    //     Texture upper_body_full_texture;
    //     Texture hair_idle_texture;
    //     // Image hair_move_image;
    //     SpriteSheetAnimation hair_move_anim;
    // } PlayerGraphics;

    bool in_shooting_stance = player->in_shooting_stance;
    PlayerGraphics* graphics = &player->graphics;
    Rectangle source = {
        0, 0,
        graphics->motorcycle_texture.width, graphics->motorcycle_texture.height,
    };
    Rectangle destination = {
        .x = player->position.x, //  - source.width/2.0,
        .y = player->position.y, // - source.height/2.0,
        .width = source.width,
        .height = source.height,
    };
    Vector2 origin = {source.width/2.0, source.height/2.0};
    float rotation = 90 + (RAD2DEG * Vector2Angle((Vector2) {1, 0}, player->direction));
    Color tint = WHITE;

    // player ground
    // DrawRectanglePro(
    //     destination,
    //     origin,
    //     rotation,
    //     tint
    // );

    // motorcycle
    DrawTexturePro(
        in_shooting_stance ? graphics->motorcycle_texture : graphics->motorcycle_with_gun_texture,
        source,
        destination,
        origin,
        rotation,
        tint
    );

    // lower body
    DrawTexturePro(
        graphics->lower_body_texture,
        source,
        destination,
        origin,
        rotation,
        tint
    );

    // left arm with gun
    if (in_shooting_stance) {
        Vector2 aim_origin_relative = Vector2Rotate(player->aim_origin_relative, DEG2RAD * rotation);
        Vector2 aim_origin = Vector2Add(player->position, aim_origin_relative);

        Texture left_arm_with_gun_texture = graphics->left_arm_with_gun_texture;
        DrawTexturePro(
            left_arm_with_gun_texture,
            (Rectangle) { 0, 0, left_arm_with_gun_texture.width, left_arm_with_gun_texture.height },
            (Rectangle) {
                .x = aim_origin.x,
                .y = aim_origin.y,
                .width = left_arm_with_gun_texture.width,
                .height = left_arm_with_gun_texture.height,
            },
            (Vector2) { 0, left_arm_with_gun_texture.height/2.0 },
            RAD2DEG * Vector2Angle((Vector2) {1, 0}, player->aim_direction),
            tint
        );

        if (player->in_shoot_blast) {
            SpriteSheetSprite shutgun_blast = sprite_sheet_get_current_sprite(&graphics->shotgun_blast_anim);
            Texture* shotgun_blast_texture = texture_assets_get_texture_unchecked(
                &GLOBAL->TEXTURE_ASSETS,
                shutgun_blast.texture_handle
            );

            Vector2 blast_pos = Vector2Add(
                aim_origin,
                Vector2Scale(player->aim_direction, left_arm_with_gun_texture.width)
            );
            // blast_pos = aim_origin;

            // DrawRectanglePro(
            //     (Rectangle) {
            //         .x = blast_pos.x,
            //         .y = blast_pos.y,// - shutgun_blast.sprite.height/2.0,
            //         .width = shutgun_blast.sprite.width,
            //         .height = shutgun_blast.sprite.height,
            //     },
            //     (Vector2) { shutgun_blast.sprite.width/2.0, 0 },
            //     90 + (RAD2DEG * Vector2Angle((Vector2) {1, 0}, player->aim_direction)),
            //     tint
            // );
            // arrow_draw(
            //     (Arrow) {
            //         .base = blast_pos,
            //         .direction = player->aim_direction,
            //         .length = 128,
            //         .head_radius = 20,
            //     },
            //     5,
            //     GREEN
            // );
            DrawTexturePro(
                *shotgun_blast_texture,
                shutgun_blast.sprite,
                (Rectangle) {
                    .x = blast_pos.x,
                    .y = blast_pos.y,
                    .width = shutgun_blast.sprite.width,
                    .height = shutgun_blast.sprite.height,
                },
                (Vector2) { 0, shutgun_blast.sprite.height/2.0 },
                RAD2DEG * Vector2Angle((Vector2) {1, 0}, player->aim_direction),
                tint
            );
        }
    }

    Vector2 heart_origin_relative = Vector2Rotate(player->heart_origin_relative, DEG2RAD * rotation);
    Vector2 heart_origin = Vector2Add(player->position, heart_origin_relative);
    Vector2 skill_direction = Vector2Normalize(Vector2Subtract(GLOBAL->LILU_POSITION, player->position));
    player_skill_show_way_draw(
        GLOBAL,
        &player->player_skill_show_way,
        heart_origin,
        skill_direction
    );

    // upper body
    DrawTexturePro(
        in_shooting_stance ? graphics->upper_body_texture : graphics->upper_body_full_texture,
        source,
        destination,
        origin,
        rotation,
        tint
    );

    // hair
    const float hair_move_speed_threshold = 100.0;
    if (player->speed >= hair_move_speed_threshold) {
        // move
        SpriteSheetSprite hair_anim_frame = sprite_sheet_get_current_sprite(&graphics->hair_move_anim);
        Texture* hair_anim_frame_texture = texture_assets_get_texture_unchecked(&GLOBAL->TEXTURE_ASSETS, hair_anim_frame.texture_handle);
        DrawTexturePro(
            *hair_anim_frame_texture,
            hair_anim_frame.sprite,
            destination,
            origin,
            rotation,
            tint
        );
    }
    else {
        // idle
        DrawTexturePro(
            graphics->hair_idle_texture,
            source,
            destination,
            origin,
            rotation,
            tint
        );
    }

    // DrawCircleLinesV(player->collider.center, player->collider.radius, GREEN);

    // --

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