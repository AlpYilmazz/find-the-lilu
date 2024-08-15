#pragma once

#define LAZY_INIT 0
#define LATER_INIT 0
#define UNINIT 0

static float signof_f(float x) {
    return (x == 0.0) ? 0.0
        : (x > 0.0) ? 1.0
        : -1.0;
}
