#pragma once

#include "raylib.h"

#include "collider.h"

typedef struct {
    CircleCollider collider;
    Vector2 start_position;
    float lifetime;
    Vector2 prev_position;
    Vector2 position;
    Vector2 velocity;
} Bullet;

Bullet new_bullet(Vector2 position, Vector2 velocity, float lifetime, float radius);
bool bullet_update(Bullet* bullet, float delta_time);
void bullet_draw(Bullet* bullet);

typedef struct {
    unsigned int count;
    unsigned int capacity;
    Bullet* bullets;
} BulletManager;

BulletManager new_bullet_manager();
void bullet_manager_spawn_bullet(BulletManager* bullet_manager, Bullet bullet);
void bullet_manager_update_bullets(BulletManager* bullet_manager, float delta_time);
int bullet_manager_check_bullet_hits(BulletManager* bullet_manager, CircleCollider enemy_collider);
void bullet_manager_draw_bullets(BulletManager* bullet_manager);