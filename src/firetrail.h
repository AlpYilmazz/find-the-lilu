#pragma once

#include "raylib.h"

typedef struct {
    Shader shader;
    float time;
    float noise_speed;
    float wave_speed;
    Texture noise_texture;
    Texture wave_texture;
} FireTrailMaterial;

void fire_trail_material_set_shader_values(FireTrailMaterial* ftm);
void fire_trail_material_draw(FireTrailMaterial* ftm, RenderTexture2D target);

typedef struct {
    FireTrailMaterial material;
    RenderTexture2D fire_trail_target_texture;
    bool paused;
    float length;
    float girth;
} FireTrail;

void fire_trail_process_input(FireTrail* fire_trail);
void fire_trail_update(FireTrail* fire_trail, float delta_time);
void fire_trail_draw(FireTrail* fire_trail, Vector2 base, Vector2 direction);