#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#include "global.h"
#include "util.h"
#include "asset.h"
#include "animation.h"
#include "collider.h"
#include "player.h"

#include "enemy.h"

// EnemyGraphics

// Enemy

Enemy enemy_create_default() {
    return (Enemy) {
        .graphics = { LAZY_INIT },
        .collider = {
            .center = {0, 0},
            .radius = 16,
        },
        .health = 150,
        .position = {0, 0},
        .speed = 300,
        .moving = false,
    };
}

void __enemy_delete(Enemy* enemy) {
    // TODO: add handle_destruction to Animation
    free(enemy->graphics.idle_anim.timer.checkpoints);
    free(enemy->graphics.move_anim.timer.checkpoints);
}

void enemy_update(EnemySpawner* enemy_spawner, Enemy* enemy, Player* player, float delta_time) {
    const float FOLLOW_RADIUS = 500;
    bool close_to_player = Vector2DistanceSqr(enemy->position, player->position) <= FOLLOW_RADIUS * FOLLOW_RADIUS;
    if (!enemy->moving && close_to_player) {
        enemy->moving = true;
        reset_sprite_sheet_animation(&enemy->graphics.idle_anim);
    }
    // else if (enemy->moving && !close_to_player) {
    //     enemy->moving = false;
    //     reset_sprite_sheet_animation(&enemy->graphics.move_anim);

    //     enemy_spawner_spawn_at_position(
    //         enemy_spawner,
    //         Vector2Add(
    //             player->position,
    //             Vector2Scale(Vector2Rotate(player->direction, DEG2RAD * 20), 600)
    //         )
    //     );
    // }

    if (enemy->moving) {
        enemy->direction = Vector2Normalize(Vector2Subtract(player->position, enemy->position));

        enemy->position = Vector2Add(
            enemy->position,
            Vector2Scale(enemy->direction, enemy->speed * delta_time)
        );

        enemy->collider.center = enemy->position;

        tick_sprite_sheet_animation_timer(&enemy->graphics.move_anim, delta_time);
    }
    else {
        tick_sprite_sheet_animation_timer(&enemy->graphics.idle_anim, delta_time);
    }
}

void enemy_draw(GlobalResources* GLOBAL, Enemy* enemy) {
    SpriteSheetSprite sp;
    if (enemy->moving) {
        sp = sprite_sheet_get_current_sprite(&enemy->graphics.move_anim);
    }
    else {
        sp = sprite_sheet_get_current_sprite(&enemy->graphics.idle_anim);
    }

    Texture* tex = texture_assets_get_texture_unchecked(&GLOBAL->TEXTURE_ASSETS, sp.texture_handle);
    DrawTexturePro(
        *tex,
        sp.sprite,
        (Rectangle) {
            .x = enemy->position.x,
            .y = enemy->position.y,
            .width = sp.sprite.width,
            .height = sp.sprite.height,
        },
        (Vector2) {sp.sprite.width/2.0, sp.sprite.height/2.0},
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, enemy->direction),
        WHITE
    );

    // Color color = (enemy->health > 0) ? GREEN : RED;
    // DrawCircleLinesV(enemy->collider.center, enemy->collider.radius, color);
}

// EnemySpawner

EnemySpawner new_enemy_spawner() {
    return (EnemySpawner) {0};
}

void enemy_spawner_spawn_enemy(EnemySpawner* dyn_array, Enemy item) {
    Enemy* items = dyn_array->enemies;

    #define INITIAL_CAPACITY 32
    #define GROWTH_FACTOR 2

    if (dyn_array->capacity == 0) {
        dyn_array->capacity = INITIAL_CAPACITY;
        items = malloc(dyn_array->capacity * sizeof(Enemy));
    }
    else if (dyn_array->count == dyn_array->capacity) {
        dyn_array->capacity = GROWTH_FACTOR * dyn_array->capacity;
        items = realloc(items, dyn_array->capacity * sizeof(Enemy));
    }

    items[dyn_array->count] = item;
    dyn_array->count++;

    #undef INITIAL_CAPACITY
    #undef GROWTH_FACTOR

    dyn_array->enemies = items;
}

void enemy_spawner_swap_remove_enemy(EnemySpawner* dyn_array, int index) {
    Enemy* items = dyn_array->enemies;

    dyn_array->count--;
    items[index] = items[dyn_array->count];
}

void enemy_spawner_spawn_at_position(EnemySpawner* enemy_spawner, Vector2 position) {
    Enemy enemy = {
        .graphics = {
            .idle_anim = new_sprite_sheet_animation_single_row_even_timer(
                enemy_spawner->enemy_idle_anim_sprite_sheet_texture_handle,
                (Vector2) {128, 64},
                7,      // frame count
                0.1,    // time between frames
                Timer_Repeating
            ),
            .move_anim = new_sprite_sheet_animation_single_row_even_timer(
                enemy_spawner->enemy_move_anim_sprite_sheet_texture_handle,
                (Vector2) {128, 64},
                4,      // frame count
                0.1,    // time between frames
                Timer_Repeating
            ),
        },
        .collider = {
            .center = position,
            .radius = 16,
        },
        .health = 150,
        .position = position,
        .speed = 300,
        .moving = false,
    };

    enemy_spawner_spawn_enemy(enemy_spawner, enemy);
}

void enemy_spawner_update_enemies(GlobalResources* GLOBAL, EnemySpawner* enemy_spawner, Player* player, float delta_time) {
    for (int i = 0; i < enemy_spawner->count; i++) {
        Enemy* enemy = &enemy_spawner->enemies[i];
        int hit_count = bullet_manager_check_bullet_hits(&GLOBAL->BULLET_MANAGER, enemy->collider);
        enemy->health -= hit_count * player->bullet_damage;
    }

    for (int i = 0; i < enemy_spawner->count; i++) {
        Enemy* enemy = &enemy_spawner->enemies[i];
        if (enemy->health <= 0) {
            // ENEMY DEAD
            __enemy_delete(enemy);
            enemy_spawner_swap_remove_enemy(enemy_spawner, i);
            i--; // handle this index again
        }
        enemy_update(enemy_spawner, enemy, player, delta_time);
    }

    for (int i = 0; i < enemy_spawner->count; i++) {
        Enemy* enemy = &enemy_spawner->enemies[i];
        bool enemy_hit = collide_circle_circle(player->collider, enemy->collider);
        if (enemy_hit) {
            int enemy_damage = 10;
            player->health -= enemy_damage;
            enemy->position = Vector2Add(
                enemy->position,
                Vector2Scale(Vector2Negate(enemy->direction), 10 * enemy->collider.radius)
            );
        }
    }
}

void enemy_spawner_draw_enemies(GlobalResources* GLOBAL, EnemySpawner* enemy_spawner) {
    for (int i = 0; i < enemy_spawner->count; i++) {
        Enemy* enemy = &enemy_spawner->enemies[i];
        enemy_draw(GLOBAL, enemy);
    }
}

// Enemy Spawn Strategies

void enemy_spawn_strategy_line_formation(GlobalResources* GLOBAL, EnemySpawner* enemy_spawner) {
    // TODO
}