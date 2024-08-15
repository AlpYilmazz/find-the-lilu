
#include "raylib.h"
#include "raymath.h"

#include "firetrail.h"

// FireTrailMaterial

void fire_trail_material_set_shader_values(FireTrailMaterial* ftm) {
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "time"),
        &ftm->time,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "noiseSpeed"),
        &ftm->noise_speed,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValue(
        ftm->shader,
        GetShaderLocation(ftm->shader, "waveSpeed"),
        &ftm->wave_speed,
        SHADER_UNIFORM_FLOAT
    );
    SetShaderValueTexture(
        ftm->shader,
        GetShaderLocation(ftm->shader, "noiseTexture"),
        ftm->noise_texture
    );
}

void fire_trail_material_draw(FireTrailMaterial* ftm, RenderTexture2D target) {
    BeginShaderMode(ftm->shader);
        fire_trail_material_set_shader_values(ftm);
        DrawTexturePro(
            ftm->wave_texture,
            (Rectangle) { 0, 0, ftm->wave_texture.width, ftm->wave_texture.height },
            (Rectangle) {0, 0, target.texture.width, target.texture.height},
            (Vector2) {0, 0},
            0,
            WHITE
        );
    EndShaderMode();
}

// FireTrail

void fire_trail_process_input(FireTrail* fire_trail) {
    if (IsKeyPressed(KEY_T)) {
        fire_trail->paused ^= 1;
    }
}

void fire_trail_update(FireTrail* fire_trail, float delta_time) {
    if (!fire_trail->paused) {
        fire_trail->material.time += delta_time;
    }
}

void fire_trail_draw(FireTrail* fire_trail, Vector2 base, Vector2 direction) {
    Texture tex = fire_trail->fire_trail_target_texture.texture;
    DrawTexturePro(
        tex,
        (Rectangle) { 0, 0, -tex.width, tex.height },
        (Rectangle) {
            .x = base.x,
            .y = base.y, //  - fire_trail->girth/2.0 + fire_trail->girth/2.0,
            .width = fire_trail->length,
            .height = fire_trail->girth,
        },
        (Vector2) { 0, fire_trail->girth/2.0 },
        RAD2DEG * Vector2Angle((Vector2) {1, 0}, direction),
        WHITE
    );
}