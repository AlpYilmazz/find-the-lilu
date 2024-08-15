#pragma once

#include "raylib.h"

typedef struct {
    float head_radius;
    Vector2 base;
    Vector2 direction; // normalized
    float length;
} Arrow;

Vector2 arrow_get_head(Arrow arrow);
void arrow_draw(Arrow arrow, float thick, Color color);