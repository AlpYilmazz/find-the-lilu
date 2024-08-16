#pragma once

#include "asset.h"
#include "bullet.h"

// typedef struct EnemySpawner EnemySpawner;

typedef struct {
    unsigned int SCREEN_WIDTH;
    unsigned int SCREEN_HEIGHT;
    TextureAssets TEXTURE_ASSETS;
    TextureHandle NOISE_TEXTURE_HANDLE;
    BulletManager BULLET_MANAGER;
//    EnemySpawner ENEMY_SPAWNER;
} GlobalResources;