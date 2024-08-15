
#include "raylib.h"
#include "raymath.h"

#include "shapes.h"

// Arrow

Vector2 arrow_get_head(Arrow arrow) {
    return Vector2Add(
        arrow.base,
        Vector2Scale(arrow.direction, arrow.length)
    );
}

void arrow_draw(Arrow arrow, float thick, Color color) {
    Vector2 b = Vector2Zero();
    Vector2 h = Vector2Scale(arrow.direction, arrow.length);
    Vector2 hu = Vector2Add(h, Vector2Scale(Vector2Rotate(arrow.direction, DEG2RAD * -150), arrow.head_radius));
    Vector2 hd = Vector2Add(h, Vector2Scale(Vector2Rotate(arrow.direction, DEG2RAD * -210), arrow.head_radius));

    b = Vector2Add(arrow.base, b);
    h = Vector2Add(arrow.base, h);
    hu = Vector2Add(arrow.base, hu);
    hd = Vector2Add(arrow.base, hd);

    DrawLineEx(b, h, thick, color);
    DrawLineEx(h, hu, thick, color);
    DrawLineEx(h, hd, thick, color);
}