#pragma once

#include "raylib.h"

#include "global.h"
#include "animation.h"
#include "bullet.h"
#include "firetrail.h"

typedef struct {
    FireTrail fire_trail;
    Texture heart_texture;
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
} PlayerGraphics;

typedef struct {
    PlayerGraphics graphics;
    PlayerSkill_ShowWay player_skill_show_way;
    //
    Vector2 heart_origin_relative;
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
    Vector2 aim_direction;
    int bullet_damage;
    //
    bool in_shooting_stance;
    bool reverse_active;
} Player;

Player player_create_default();
void player_process_input(GlobalResources* GLOBAL, Player* player, Camera2D camera);
void player_update(Player* player, float delta_time);
void player_draw(GlobalResources* GLOBAL, Player* player);