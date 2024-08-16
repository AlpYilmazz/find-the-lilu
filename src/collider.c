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

bool collide_aabb_aabb(AABBCollider r1, AABBCollider r2) {
    return r1.x_left <= r2.x_right &&
          r2.x_left <= r1.x_right &&
          r1.y_top <= r2.y_bottom &&
          r2.y_top <= r1.y_bottom;
}