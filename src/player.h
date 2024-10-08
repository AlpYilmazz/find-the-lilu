#pragma once

#include "raylib.h"

#include "global.h"
#include "collider.h"
#include "animation.h"
#include "bullet.h"
#include "firetrail.h"

typedef struct {
    FireTrail fire_trail;
    SpriteSheetAnimation heart_beat_anim;
    Texture portal_texture;
    SpriteSheetAnimation portal_vfx_anim;
    //
    bool active;
} PlayerSkill_ShowWay;

void player_skill_show_way_process_input(PlayerSkill_ShowWay* skill, bool skill_can_activate);
void player_skill_show_way_update(PlayerSkill_ShowWay* skill, float delta_time);
void player_skill_show_way_predraw(PlayerSkill_ShowWay* skill);
void player_skill_show_way_draw(GlobalResources* GLOBAL, PlayerSkill_ShowWay* skill, Vector2 position, Vector2 direction);

typedef struct {
    Texture motorcycle_texture;
    Texture motorcycle_with_gun_texture;
    Texture lower_body_texture;
    Texture left_arm_with_gun_texture;
    Texture upper_body_texture;
    Texture upper_body_full_texture;
    Texture hair_idle_texture;
    // Image hair_move_image;
    SpriteSheetAnimation hair_move_anim;
    SpriteSheetAnimation shotgun_blast_anim;
} PlayerGraphics;

typedef struct {
    Music motorcycle_idle_audio;
    Music motorcycle_move_audio;
    Music shutgun_blast_audio;
    Music shutgun_pump_audio;
} PlayerAudio;

typedef struct {
    PlayerGraphics graphics;
    PlayerAudio audio;
    PlayerSkill_ShowWay player_skill_show_way;
    //
    Vector2 heart_origin_relative;
    //
    CircleCollider collider;
    int health;
    //
    Vector2 position;
    Vector2 direction;
    float speed;
    float accelaration;
    float __drag;
    //
    int rotation_direction;
    float rotation_speed_deg;
    //
    Vector2 aim_origin_relative;
    Vector2 aim_cursor_point;
    Vector2 aim_direction;
    bool gun_ready;
    bool do_shoot_this_frame;
    Timer shoot_cooldown_timer;
    int bullet_damage;
    //
    bool in_shooting_stance;
    bool in_shoot_blast;
    bool reverse_active;
} Player;

Player player_create_default();
void player_process_input(Player* player, Camera2D camera);
void player_update(GlobalResources* GLOBAL, Player* player, float delta_time);
void player_draw(GlobalResources* GLOBAL, Player* player);