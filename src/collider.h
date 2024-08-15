#pragma once

#include "raylib.h"

typedef struct {
    Vector2 center;
    float radius;
} CircleCollider;

float collider_dist_circle_circle(CircleCollider c1, CircleCollider c2);
bool collide_circle_circle(CircleCollider c1, CircleCollider c2);