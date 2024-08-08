#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define LAZY_INIT 0

struct {
    unsigned int SCREEN_WIDTH;
    unsigned int SCREEN_HEIGHT;
} GLOBAL = {
    .SCREEN_WIDTH = LAZY_INIT,
    .SCREEN_HEIGHT = LAZY_INIT,
};

void global_set_screen_size(unsigned int width, unsigned int height) {
    GLOBAL.SCREEN_WIDTH = width;
    GLOBAL.SCREEN_HEIGHT = height;
}

typedef struct {
    float head_radius;
    Vector2 base;
    Vector2 direction; // normalized
    float length;
} Arrow;

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

typedef struct {
    Vector2 position;
    Vector2 direction;
    float speed;
    float accelaration;
    float __drag;
    //
    int rotation_direction;
    float rotation_speed; // DEG
    //
    Vector2 aim_direction;
} Player;

Player player_create_default() {
    return (Player) {
        .position = 0,
        .direction = {0, -1},
        .speed = 100,
        .accelaration = 0,
        .__drag = 0,
        .rotation_direction = 0,
        .rotation_speed = 100,
        .aim_direction = {1, 0},
    };
}

void player_process_input(Player* player) {
    if (IsKeyDown(KEY_UP) && IsKeyDown(KEY_DOWN)) {
        player->accelaration = 0;
    }
    else if (IsKeyDown(KEY_UP)) {
        player->accelaration = 1000;
    }
    else if (IsKeyDown(KEY_DOWN)) {
        player->accelaration = -100;
    }
    else {
        player->accelaration = 0;
    }

    if (IsKeyDown(KEY_RIGHT) && IsKeyDown(KEY_LEFT)) {
        player->rotation_direction = 0;
    }
    else if (IsKeyDown(KEY_RIGHT)) {
        player->rotation_direction = 1;
    }
    else if (IsKeyDown(KEY_LEFT)) {
        player->rotation_direction = -1;
    }
    else {
        player->rotation_direction = 0;
    }
}

void player_update(Player* player, Camera2D camera, float delta_time) {
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

    const float DRAG_COEFF = 1.0 / 800;
    const float CUT_SECTION = 1;
    float drag = DRAG_COEFF * CUT_SECTION * ((player->speed * player->speed)/2);
    player->__drag = drag;

    player->speed += (player->accelaration - drag) * delta_time;
    player->speed = Clamp(player->speed, 0.0, player->speed);

    player->direction = Vector2Rotate(
        player->direction,
        player->rotation_direction * DEG2RAD * player->rotation_speed * delta_time
    );

    player->position = Vector2Add(
        player->position,
        Vector2Scale(player->direction, player->speed * delta_time)
    );

    player->aim_direction = Vector2Normalize(Vector2Subtract(mouse, player->position));
}

void player_draw(Player* player) {
    float player_radius = 50;

    arrow_draw(
        (Arrow) {
            .head_radius = 30,
            .base = player->position,
            .direction = player->direction,
            .length = player_radius + 10*10,
        },
        5,
        RED
    );
    DrawCircleV(player->position, player_radius, WHITE);

    arrow_draw(
        (Arrow) {
            .head_radius = 30,
            .base = player->position,
            .direction = player->aim_direction,
            .length = player_radius + 10*10,
        },
        5,
        GREEN
    );
}

int main() {

    {
        global_set_screen_size(1920, 1080);
    }

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");
    SetTargetFPS(60);

    {
        int monitor_id = GetMonitorCount() - 1;
        // Vector2 monitor_pos = GetMonitorPosition(monitor_id);
        // printf("Monitor Pos: %d %d\n", (int)monitor_pos.x, (int)monitor_pos.y);

        SetWindowMonitor(monitor_id);
        // SetWindowPosition(monitor_pos.x, monitor_pos.y);
        SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_BORDERLESS_WINDOWED_MODE);
    }

    Camera2D camera = {0};
    camera.zoom = 1;
    camera.offset = (Vector2) {
        .x = GLOBAL.SCREEN_WIDTH/2.0,
        .y = GLOBAL.SCREEN_HEIGHT/2.0,
    };

    Player player = player_create_default();

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();

        if (IsWindowResized()) {
            global_set_screen_size(GetScreenWidth(), GetScreenHeight());
            camera.offset = (Vector2) {
                .x = GLOBAL.SCREEN_WIDTH/2.0,
                .y = GLOBAL.SCREEN_HEIGHT/2.0,
            };
        }

        if (IsKeyPressed(KEY_P)) {
            Vector2 window_pos = GetWindowPosition();
            printf("Window Pos: %0.1f %0.1f\n", window_pos.x, window_pos.y);
            printf("Window Size: %d %d\n", GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT);
        }

        // INPUT
        player_process_input(&player);

        // UPDATE
        player_update(&player, camera, delta_time);
        camera.target = player.position;

        // DRAW
        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(camera);

                DrawCircle(0, 0, 5, RED); // ORIGIN

                player_draw(&player);

            EndMode2D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Speed: %d", (int)player.speed), 10, 10 + 1*50, 20, MAGENTA);
            DrawText(TextFormat("Acc  : %d", (int)player.accelaration), 10, 10 + 2*50, 20, MAGENTA);
            DrawText(TextFormat("Drag : %0.2f", player.__drag), 10, 10 + 3*50, 20, MAGENTA);

        EndDrawing();
    }

    return 0;
}