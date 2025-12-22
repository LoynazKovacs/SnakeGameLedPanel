#pragma once
#include <Arduino.h>

/**
 * 4x4 sprites (1 = pixel on).
 * Drawn directly into a 4x4 tile.
 */

// Powerups
inline constexpr uint8_t PU_BOOT_4[4][4] = {
    {0,1,1,0},
    {1,1,1,1},
    {0,1,1,0},
    {0,1,1,0},
};

inline constexpr uint8_t PU_BOMB_4[4][4] = {
    {0,1,1,0},
    {1,1,1,1},
    {1,1,1,1},
    {0,1,1,0},
};

inline constexpr uint8_t PU_FLAME_4[4][4] = {
    {0,1,0,0},
    {1,1,1,0},
    {0,1,1,1},
    {0,0,1,0},
};

inline constexpr uint8_t PU_SHIELD_4[4][4] = {
    {0,1,1,0},
    {1,0,0,1},
    {1,1,1,1},
    {0,1,1,0},
};

// Enemies
inline constexpr uint8_t ENEMY_RANDOM_4[4][4] = {
    {1,0,0,1},
    {0,1,1,0},
    {0,1,1,0},
    {1,0,0,1},
};

inline constexpr uint8_t ENEMY_CHASER_4[4][4] = {
    {0,1,1,0},
    {1,1,1,1},
    {1,0,0,1},
    {0,1,1,0},
};


