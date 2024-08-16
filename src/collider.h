#pragma once

#include "raylib.h"

typedef struct {
    Vector2 center;
    float radius;
} CircleCollider;

typedef struct {
    float x_left;
    float x_right;
    float y_top;
    float y_bottom;
} AABBCollider;

float collider_dist_circle_circle(CircleCollider c1, CircleCollider c2);
bool collide_circle_circle(CircleCollider c1, CircleCollider c2);

bool collide_aabb_aabb(AABBCollider r1, AABBCollider r2);