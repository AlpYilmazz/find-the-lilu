#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#include "collider.h"

#include "bullet.h"

// Bullet

Bullet new_bullet(Vector2 position, Vector2 velocity, float lifetime, float radius) {
    return (Bullet) {
        .collider = {
            .center = position,
            .radius = radius,
        },
        .start_position = position,
        .lifetime = lifetime,
        .prev_position = position,
        .position = position,
        .velocity = velocity,
    };
}

bool bullet_update(Bullet* bullet, float delta_time) {
    bullet->lifetime -= delta_time;
    if (bullet->lifetime <= 0.0) {
        return false;
    }

    bullet->prev_position = bullet->position;
    bullet->position = Vector2Add(
        bullet->position,
        Vector2Scale(bullet->velocity, delta_time)
    );
    bullet->collider.center = bullet->position;

    return true;
}

void bullet_draw(Bullet* bullet) {
    Color bullet_trail_color = { 255, 236, 145, 100 };
    DrawLineEx(bullet->start_position, bullet->position, bullet->collider.radius/2.0, bullet_trail_color);
    DrawCircleV(bullet->position, bullet->collider.radius, BLACK);
}

// BulletManager

BulletManager new_bullet_manager() {
    return (BulletManager) {0};
}

void bullet_manager_spawn_bullet(BulletManager* bullet_manager, Bullet bullet) {
    #define INITIAL_CAPACITY 32
    #define GROWTH_FACTOR 2

    if (bullet_manager->capacity == 0) {
        bullet_manager->capacity = INITIAL_CAPACITY;
        bullet_manager->bullets = malloc(bullet_manager->capacity * sizeof(Bullet));
    }
    else if (bullet_manager->count == bullet_manager->capacity) {
        bullet_manager->capacity = GROWTH_FACTOR * bullet_manager->capacity;
        bullet_manager->bullets = realloc(bullet_manager->bullets, bullet_manager->capacity * sizeof(Bullet));
    }

    bullet_manager->bullets[bullet_manager->count] = bullet;
    bullet_manager->count++;

    #undef INITIAL_CAPACITY
    #undef GROWTH_FACTOR
}

void bullet_manager_update_bullets(BulletManager* bullet_manager, float delta_time) {
    for (int i = 0; i < bullet_manager->count; i++) {
        //printf("Bullet updated: %d\n", i);
        bool bullet_lives = bullet_update(&bullet_manager->bullets[i], delta_time);
        if (!bullet_lives) {
            //printf("Bullet despawn\n");
            bullet_manager->count--;
            bullet_manager->bullets[i] = bullet_manager->bullets[bullet_manager->count];
            i--; // handle this index again
        }
    }
}

int bullet_manager_check_bullet_hits(BulletManager* bullet_manager, CircleCollider enemy_collider) {
    bool __debug_raymarching_method_flag = !IsKeyDown(KEY_F);

    int hit_count = 0;

    Bullet* bullet;
    bool bullet_hit;
    Vector2 bullet_direction;
    Vector2 bullet_position;
    float prev_collider_dist;
    CircleCollider bullet_collider;

    const int MAX_ITER = 100;
    float collider_dist;
    float position_dist;
    float move_amount;

    for (int i = 0; i < bullet_manager->count; i++) {
        bullet = &bullet_manager->bullets[i];
        bullet_hit = false;

        if (!__debug_raymarching_method_flag) {
            bullet_collider = bullet->collider;
            bullet_hit = collide_circle_circle(bullet_collider, enemy_collider);
        }
        else {
            bullet_direction = Vector2Normalize(Vector2Subtract(bullet->position, bullet->prev_position));
            bullet_position = bullet->prev_position;
            prev_collider_dist = INFINITY;
            bullet_collider = (CircleCollider) {
                .center = bullet_position,
                .radius = bullet->collider.radius,
            };

            for (int __iter = 0; __iter < MAX_ITER; __iter++) {
                bullet_collider.center = bullet_position;
                collider_dist = collider_dist_circle_circle(bullet_collider, enemy_collider);
                position_dist = Vector2Distance(bullet_position, bullet->position);
                move_amount = __min(collider_dist, position_dist);

                if (collider_dist <= EPSILON) {
                    // collision
                    bullet_hit = true;
                    break;
                }
                else if (position_dist <= EPSILON) {
                    // end position reached
                    break;
                }
                else if (collider_dist > prev_collider_dist) {
                    // started moving away
                    break;
                }

                bullet_position = Vector2Add(
                    bullet_position,
                    Vector2Scale(bullet_direction, move_amount)
                );
                bullet_collider.center = bullet_position;
                
                if (__iter == MAX_ITER-1) {
                    printf("MAX ITER REACHED\n");
                }
            }
        }

        if (bullet_hit) {
            hit_count++;
            bullet_manager->count--;
            bullet_manager->bullets[i] = bullet_manager->bullets[bullet_manager->count];
            i--; // handle this index again
        }
    }
    return hit_count;
}

void bullet_manager_draw_bullets(BulletManager* bullet_manager) {
    for (int i = 0; i < bullet_manager->count; i++) {
        bullet_draw(&bullet_manager->bullets[i]);
    }
}