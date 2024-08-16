#pragma once

#include "global.h"
#include "asset.h"
#include "animation.h"
#include "collider.h"
#include "player.h"

typedef struct {
    SpriteSheetAnimation idle_anim;
    SpriteSheetAnimation move_anim;
} EnemyGraphics;

typedef struct {
    EnemyGraphics graphics;
    //
    CircleCollider collider;
    int health;
    //
    bool moving;
    Vector2 position;
    Vector2 direction;
    float speed;
} Enemy;

Enemy enemy_create_default();
void __enemy_delete();
void enemy_update(Enemy* enemy, Player* player, float delta_time);
void enemy_draw(GlobalResources* GLOBAL, Enemy* enemy);

typedef struct {
    TextureHandle enemy_idle_anim_sprite_sheet_texture_handle;
    TextureHandle enemy_move_anim_sprite_sheet_texture_handle;
    //
    unsigned int count;
    unsigned int capacity;
    Enemy* enemies;
} EnemySpawner;

EnemySpawner new_enemy_spawner();
void enemy_spawner_spawn_enemy(EnemySpawner* dyn_array, Enemy item);
void enemy_spawner_swap_remove_enemy(EnemySpawner* dyn_array, int index);
void enemy_spawner_spawn_at_position(GlobalResources* GLOBAL, EnemySpawner* enemy_spawner, Vector2 position);
void enemy_spawner_update_enemies(GlobalResources* GLOBAL, EnemySpawner* enemy_spawner, Player* player, float delta_time);
void enemy_spawner_draw_enemies(GlobalResources* GLOBAL, EnemySpawner* enemy_spawner);
