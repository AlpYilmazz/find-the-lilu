#include "stdio.h"

#include "raylib.h"

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

int main() {

    {
        global_set_screen_size(1920, 1080);
    }

    InitWindow(GLOBAL.SCREEN_WIDTH, GLOBAL.SCREEN_HEIGHT, "Find The LILU");
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

    while (!WindowShouldClose()) {

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

        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(camera);

                DrawCircle(0, 0, 5, RED); // ORIGIN

            EndMode2D();
        EndDrawing();
    }

    return 0;
}