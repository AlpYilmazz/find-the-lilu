#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#include "collider.h"

float collider_dist_circle_circle(CircleCollider c1, CircleCollider c2) {
    return Vector2Distance(c1.center, c2.center) - c1.radius - c2.radius;
}

bool collide_circle_circle(CircleCollider c1, CircleCollider c2) {
    float radius_sum = c1.radius + c2.radius;
    return Vector2DistanceSqr(c1.center, c2.center) <= radius_sum * radius_sum + EPSILON;
}