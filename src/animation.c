#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "asset.h"

#include "animation.h"

// Timer

Timer new_timer(float setup_secs, TimerMode mode) {
    return (Timer) {
        .mode = mode,
        .time_setup = setup_secs,
        .time_elapsed = 0.0,
        .finished = false,
    };
}

void reset_timer(Timer* timer) {
    timer->time_elapsed = 0.0;
    timer->finished = false;
}

void tick_timer(Timer* timer, float delta_time_seconds) {
    switch (timer->mode) {
    case Timer_NonRepeating:
        if (!timer->finished) {
            timer->time_elapsed += delta_time_seconds;
            if (timer->time_elapsed >= timer->time_setup) {
                timer->finished = true;
            }
        }
        break;
    case Timer_Repeating:
        timer->time_elapsed += delta_time_seconds;
        if (timer->time_elapsed >= timer->time_setup) {
            timer->time_elapsed = 0.0;
            timer->finished = true;
        }
        break;
    }
}

bool timer_is_finished(Timer* timer) {
    switch (timer->mode) {
    case Timer_NonRepeating:
        return timer->finished;
    case Timer_Repeating:
        if (timer->finished) {
            timer->finished = false;
            return true;
        }
        return false;
    }
}

// SequenceTimer

SequenceTimer new_sequence_timer(float* checkpoints, int count, TimerMode mode) {
    return (SequenceTimer) {
        .mode = mode,
        .checkpoint_count = count,
        .index = 0,
        .checkpoints = checkpoints,
        .time_elapsed = 0.0,
        .pulsed = false,
        .finished = false,
    };
}

// allocates
SequenceTimer new_sequence_timer_evenly_spaced(float time_between, int count, TimerMode mode) {
    float* checkpoints = malloc(count * sizeof(float));
    for (int i = 0; i < count; i++) {
        checkpoints[i] = time_between * (i+1);
    }
    return (SequenceTimer) {
        .mode = mode,
        .checkpoint_count = count,
        .index = 0,
        .checkpoints = checkpoints,
        .time_elapsed = 0.0,
        .pulsed = false,
        .finished = false,
    };
}

void reset_sequence_timer(SequenceTimer* stimer) {
    stimer->index = 0;
    stimer->time_elapsed = 0.0;
    stimer->pulsed = false;
    stimer->finished = false;
}

void tick_sequence_timer(SequenceTimer* stimer, float delta_time_seconds) {
    if (stimer->checkpoint_count == 0) {
        stimer->finished = true;
        return;
    }

    switch (stimer->mode) {
    case Timer_NonRepeating:
        if (!stimer->finished) {
            float checkpoint = stimer->checkpoints[stimer->index];
            stimer->time_elapsed += delta_time_seconds;
            if (stimer->time_elapsed >= checkpoint) {
                stimer->pulsed = true;
                stimer->index++;
                stimer->index %= stimer->checkpoint_count;
                if (stimer->index == 0) {
                    stimer->finished = true;
                }
            }
        }
        break;
    case Timer_Repeating:
        float checkpoint = stimer->checkpoints[stimer->index];
        stimer->time_elapsed += delta_time_seconds;
        if (stimer->time_elapsed >= checkpoint) {
            stimer->pulsed = true;
            stimer->index++;
            stimer->index %= stimer->checkpoint_count;
            if (stimer->index == 0) {
                stimer->time_elapsed = 0.0;
            }
        }
        break;
    }
}

bool sequence_timer_has_pulsed(SequenceTimer* stimer) {
    if (stimer->pulsed) {
        stimer->pulsed = false;
        return true;
    }
    return false;
}

bool sequence_timer_is_finished(SequenceTimer* stimer) {
    switch (stimer->mode) {
    case Timer_NonRepeating:
        return stimer->finished;
    case Timer_Repeating:
        if (stimer->finished) {
            stimer->finished = false;
            return true;
        }
        return false;
    }
}

// SpriteAnimation

SpriteAnimation new_sprite_animation(SequenceTimer timer, TextureHandle* textures, int texture_count) {
    return (SpriteAnimation) {
        .timer = timer,
        .textures = textures,
        .texture_count = texture_count,
        .current_texture_ind = 0,
    };
}

void tick_animation_timer(SpriteAnimation* anim, float delta_time_seconds) {
    if (anim->texture_count == 0) return;

    tick_sequence_timer(&anim->timer, delta_time_seconds);
    if (sequence_timer_has_pulsed(&anim->timer)) {
        anim->current_texture_ind++;
        anim->current_texture_ind %= anim->texture_count;
    }
}

TextureHandle get_current_texture(SpriteAnimation* anim) {
    if (anim->texture_count == 0) return primary_texture_handle();
    return anim->textures[anim->current_texture_ind];
}

// SpriteSheetAnimation

SpriteSheetAnimation new_sprite_sheet_animation(
    SequenceTimer timer,
    TextureHandle sprite_sheet_texture,
    Vector2 sprite_size,
    int rows,
    int cols,
    int count
) {
    return (SpriteSheetAnimation) {
        .timer = timer,
        .sprite_sheet_texture = sprite_sheet_texture,
        .sprite_size = sprite_size,
        .rows = rows,
        .cols = cols,
        .count = count,
        .current_sprite_ind = 0,
    };
}

SpriteSheetAnimation new_sprite_sheet_animation_single_row(
    SequenceTimer timer,
    TextureHandle sprite_sheet_texture,
    Vector2 sprite_size,
    int count
) {
    return new_sprite_sheet_animation(
        timer,
        sprite_sheet_texture,
        sprite_size,
        1, count, count
    );
}

SpriteSheetAnimation new_sprite_sheet_animation_single_row_even_timer(
    TextureHandle sprite_sheet_texture,
    Vector2 sprite_size,
    int count,
    float time_between,
    TimerMode mode
) {
    return new_sprite_sheet_animation(
        new_sequence_timer_evenly_spaced(time_between, count, mode),
        sprite_sheet_texture,
        sprite_size,
        1, count, count
    );
}

void reset_sprite_sheet_animation(SpriteSheetAnimation* anim) {
    reset_sequence_timer(&anim->timer);
    anim->current_sprite_ind = 0;
}

void tick_sprite_sheet_animation_timer(SpriteSheetAnimation* anim, float delta_time_seconds) {
    if (anim->count == 0) return;

    tick_sequence_timer(&anim->timer, delta_time_seconds);
    if (sequence_timer_has_pulsed(&anim->timer)) {
        anim->current_sprite_ind++;
        anim->current_sprite_ind %= anim->count;
    }
}

SpriteSheetSprite sprite_sheet_get_current_sprite(SpriteSheetAnimation* anim) {
    if (anim->count == 0) {
        return (SpriteSheetSprite) {
            .texture_handle = primary_texture_handle(),
            .sprite = {0, 0, 1, 1},
        };
    }

    int r = anim->current_sprite_ind / anim->cols;
    int c = anim->current_sprite_ind % anim->cols;

    return (SpriteSheetSprite) {
        .texture_handle = anim->sprite_sheet_texture,
        .sprite = (Rectangle) {
            .x = c * anim->sprite_size.x,
            .y = r * anim->sprite_size.y,
            .width = anim->sprite_size.x,
            .height = anim->sprite_size.y,
        },
    };
}